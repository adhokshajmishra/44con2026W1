# Workshop 1 — Automating Attack TTPs for Fun

**44CON 2026** · Friday 09:10–10:00 · Penetration testers  
**Threat actor:** [TeamTNT (G0139)](https://attack.mitre.org/groups/G0139/)

Hands-on automation of a Linux cloud/container attack chain: scan → exposed Docker API → credential harvest → escape → lateral movement → mock cryptomining.

---

## Prerequisites

- Docker Engine + Docker Compose v2
- 4 GB free RAM
- Linux or macOS host (Apple Silicon supported)
---

## Quick start

```bash
# From repo root
chmod +x setup-lab.sh workshop-01-automating-attack-ttps/scripts/*.sh
./setup-lab.sh

# Shell automation
docker compose -f workshop-01-automating-attack-ttps/lab/docker-compose.yml exec attacker \
  bash /opt/teamtnt/scripts/teamtnt-chain.sh

# C++ native automation (see native_automation/README.md)
cd workshop-01-automating-attack-ttps/native_automation
cmake -B build && cmake --build build -j
docker compose -f ../lab/docker-compose.yml exec attacker ./native_automation/build/teamtnt_native
```

Or step through scripts manually from the attacker shell:

```bash
docker compose -f workshop-01-automating-attack-ttps/lab/docker-compose.yml exec attacker bash
cd /opt/teamtnt/scripts
./01-scan-targets.sh
./02-probe-docker-api.sh
# ... through 07-mock-miner.sh
```

---

## 50-minute agenda

| Time | Module | Script | MITRE |
|------|--------|--------|-------|
| 0:00–0:05 | Intro + lab orientation | — | G0139 overview |
| 0:05–0:15 | Automated recon | `01-scan-targets.sh` | T1595, T1046 |
| 0:15–0:25 | Docker API initial access | `02-probe-docker-api.sh` | T1133, T1613 |
| 0:25–0:35 | Credential harvest pipeline | `03-harvest-credentials.sh` | T1552, T1083 |
| 0:35–0:42 | Container escape | `05-escape-and-deploy.sh` | T1611, T1105 |
| 0:42–0:47 | Lateral movement | `06-lateral-movement.sh` | T1021.004, T1528 |
| 0:47–0:50 | Impact — mock miner | `07-mock-miner.sh` | T1496.001 |

---

## Lab topology

```
┌─────────────┐     scan/harvest      ┌──────────────┐
│  attacker   │ ────────────────────► │  docker-api  │ :2375 (no TLS)
│  container  │                       │  (dind)      │
└──────┬──────┘                       └──────────────┘
       │                                      │
       │                              manages │
       ▼                                      ▼
┌─────────────┐                       ┌──────────────┐
│ metadata-sim│ 169.254.169.254       │  victim-app  │ creds in $HOME
│  (IMDSv1)   │                       │  (nginx)     │
└─────────────┘                       └──────────────┘
```

---

## Misconfigurations exercised

| Misconfiguration | Detection / hardening hint |
|------------------|---------------------------|
| Docker API on TCP without auth | Bind to localhost; enable TLS + client auth |
| IMDSv1 reachable | Enforce IMDSv2 + hop limit |
| Credentials in container env/files | Secrets manager; never chmod keys world-readable |
| Privileged containers + host mounts | Pod Security Standards; deny `privileged` |
| No egress filtering | Egress proxy; block mining pool domains |

---

## Teardown

```bash
cd lab && docker compose down -v
```

---

## Safety

All miners are **mock** (CPU loop in busybox). Credentials are **fake**. Use only in the isolated lab network.
