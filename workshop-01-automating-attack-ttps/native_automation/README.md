# TeamTNT Native Automation (C++)

CMake-based C++ port of the Workshop 1 shell attack chain. Uses **libcurl** for HTTP/HTTPS (metadata, Docker Engine API, payload download) and **nlohmann/json** for Docker API responses.

## Build

```bash
# Debian/Ubuntu / attacker container
sudo apt-get install -y cmake g++ libcurl4-openssl-dev

cd native_automation
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

## Run

```bash
export TEAMTNT_LAB=1
export STAGING_DIR=/tmp/teamtnt-staging
export DOCKER_API_URL=tcp://docker-api:2375
export METADATA_URL=http://metadata-sim/latest/meta-data

./build/teamtnt_native          # full chain
./build/teamtnt_native 3        # single step (1-7)
```

## Run inside the Docker lab

From the **repo root** (`44con/`):

```bash
# 1. Start the lab
./setup-lab.sh

# 2. Build native binary inside the attacker container (first time only)
docker compose -f workshop-01-automating-attack-ttps/lab/docker-compose.yml exec attacker \
  bash /opt/teamtnt/native_automation/build-in-lab.sh

# 3. Run the full attack chain
docker compose -f workshop-01-automating-attack-ttps/lab/docker-compose.yml exec attacker \
  /opt/teamtnt/native_automation/build-docker/teamtnt_native

# Optional: interactive shell inside attacker, then run manually
docker compose -f workshop-01-automating-attack-ttps/lab/docker-compose.yml exec attacker bash
# inside container:
#   /opt/teamtnt/native_automation/build-in-lab.sh
#   /opt/teamtnt/native_automation/build-docker/teamtnt_native
#   /opt/teamtnt/native_automation/build-docker/teamtnt_native 3   # single step
```

Environment variables are pre-set in the attacker image (`TEAMTNT_LAB`, `DOCKER_API_URL`, `METADATA_URL`).

**Do not** run `teamtnt_native` on the macOS host without overriding hosts — `docker-api` only resolves inside the lab network. Use the commands above, or set:

```bash
export DOCKER_API_URL=tcp://127.0.0.1:2375
export METADATA_URL=http://127.0.0.1:8180/latest/meta-data
```

## Steps

| Step | MITRE | Implementation |
|------|-------|----------------|
| 1 | T1595, T1046 | POSIX TCP connect scan |
| 2 | T1133, T1613 | Docker API `/_ping`, `/containers/json` |
| 3 | T1552, T1083 | libcurl metadata + Docker exec |
| 4 | T1613, T1610 | Docker images/containers API |
| 5 | T1611, T1105 | Privileged container via API + HTTPS download |
| 6 | T1021, T1078 | SSH key extract, AWS cred stub |
| 7 | T1496 | Mock miner container via API |
