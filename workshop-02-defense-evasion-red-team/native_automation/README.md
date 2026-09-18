# Defense Evasion Native Automation (C++)

CMake-based C++ port of the Workshop 2 shell evasion chain. Uses **libcurl** for HTTP/HTTPS exfil simulation (T1048).

## Build on host (optional)

```bash
sudo apt-get install -y cmake g++ make libcurl4-openssl-dev

cd native_automation
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

## Run inside the Docker lab (recommended)

Workshop 2 native automation is mounted into the shared **attacker** container at `/opt/teamtnt/evasion/`.

From the **repo root** (`44con/`):

```bash
# 1. Start the lab (shared with Workshop 1)
./setup-lab.sh

# 2. Build evasion_native inside the attacker container (first time only)
docker compose -f workshop-01-automating-attack-ttps/lab/docker-compose.yml exec attacker \
  bash /opt/teamtnt/evasion/native_automation/build-in-lab.sh

# 3. Run the full evasion chain
docker compose -f workshop-01-automating-attack-ttps/lab/docker-compose.yml exec attacker \
  /opt/teamtnt/evasion/native_automation/build-docker/evasion_native

# Optional: interactive shell
docker compose -f workshop-01-automating-attack-ttps/lab/docker-compose.yml exec attacker bash
# inside container:
#   /opt/teamtnt/evasion/native_automation/build-in-lab.sh
#   /opt/teamtnt/evasion/native_automation/build-docker/evasion_native
#   /opt/teamtnt/evasion/native_automation/build-docker/evasion_native 4   # single step
```

Environment variables are pre-set in the attacker image:

| Variable | In-container value |
|----------|-------------------|
| `TEAMTNT_EVASION_LAB` | `1` |
| `EVASION_STAGING` | `/tmp/teamtnt-evasion` |
| `MOCK_MINER_PATH` | `/tmp/.dockerd` |
| `PRELOAD_SOURCE` | `/opt/teamtnt/evasion/lib/libprocesshider-demo.c` |
| `STAGING_DIR` | `/tmp/teamtnt-staging` (WS1 harvest for exfil step) |

Run Workshop 1 first so step 5 can stage harvested creds from `/tmp/teamtnt-staging/`.

**Build directories:**
- `build/` — host builds (macOS/Linux on your machine)
- `build-docker/` — in-container builds (use this in the lab)

## Steps

| Step | MITRE | Implementation |
|------|-------|----------------|
| 1 | T1082, T1016 | uname, /etc/os-release, /proc/mounts |
| 2 | T1518, T1685 | /proc cmdline scan, systemctl |
| 3 | T1036, T1027, T1140 | Masquerade file + native base64 |
| 4 | T1574.006 | gcc builds libprocesshider-demo.c |
| 5 | T1070, T1048 | Log checks + libcurl POST exfil sim |
| 6 | T1222.002 | chattr +i |
