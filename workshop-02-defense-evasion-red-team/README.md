# Workshop 2 — Defense Evasion for Red Team

**44CON 2026** · Friday 10:00–10:50 · Red team operators  
**Threat actor:** [TeamTNT (G0139)](https://attack.mitre.org/groups/G0139/) / [Hildegard (S0601)](https://attack.mitre.org/software/S0601/)

Post-compromise evasion on Linux: hide the mock miner deployed in Workshop 1, clear forensic artifacts, and hinder remediation.

---

## Prerequisites

- Completed Workshop 1 **or** run `03-mock-miner.sh` / `07-mock-miner.sh` first
- `gcc` for ld.so.preload demo (optional; dry-run without it)
- Root/sudo for live chattr and preload demos

---

## Quick start

```bash
chmod +x workshop-02-defense-evasion-red-team/scripts/*.sh

# Shell automation
export TEAMTNT_EVASION_LAB=1
cd workshop-02-defense-evasion-red-team/scripts
./evasion-chain.sh

# C++ native automation (inside attacker container — see native_automation/README.md)
docker compose -f workshop-01-automating-attack-ttps/lab/docker-compose.yml exec attacker \
  bash /opt/teamtnt/evasion/native_automation/build-in-lab.sh
docker compose -f workshop-01-automating-attack-ttps/lab/docker-compose.yml exec attacker \
  /opt/teamtnt/evasion/native_automation/build-docker/evasion_native
```

Run individual steps for deeper discussion:

```bash
./03-masquerade-miner.sh      # T1036 — name payload .dockerd
sudo ./04-ld-preload-hide.sh  # T1574.006 — hide from ps
sudo ./06-chattr-immutable.sh # T1222.002
```

---

## 50-minute agenda

| Time | Module | Script | MITRE |
|------|--------|--------|-------|
| 0:00–0:05 | Recap WS1 foothold | — | Kill chain bridge |
| 0:05–0:12 | Host + agent recon | `01-host-recon.sh`, `02-discover-security-agents.sh` | T1082, T1518, T1685 |
| 0:12–0:22 | Masquerade + obfuscation | `03-masquerade-miner.sh` | T1036, T1027, T1140 |
| 0:22–0:32 | Process hiding | `04-ld-preload-hide.sh` | T1574.006, T1014 |
| 0:32–0:42 | Anti-forensics | `05-anti-forensics.sh` | T1070, T1074 |
| 0:42–0:50 | Tamper protection | `06-chattr-immutable.sh` | T1222.002 |

---

## Blue team detection exercises

After running the evasion chain, participants switch hats:

| Technique | Detection command / signal |
|-----------|---------------------------|
| Masqueraded miner | `ls -la /tmp/.dockerd`; compare `ps` vs `/proc` |
| ld.so.preload | `cat /etc/ld.so.preload`; auditd on write |
| chattr +i | `lsattr /tmp/.dockerd` |
| Log gaps | Compare local syslog vs centralized SIEM |

Cross-reference: `diagrams/` in this repo covers **kernel-level** hiding (ftrace rootkit talk) — complement to userspace `ld.so.preload` shown here.

---

## Kernel rootkit note (T1014)

TeamTNT deployed **Diamorphine** and similar LKMs. This workshop uses **userspace** `ld.so.preload` only (safer for conference VMs). For Ring-0 evasion depth, see the **Lord of the Ring(0)** talk materials in `diagrams/`.

---

## Safety

- Do not run against production hosts.
- `chattr +i` changes require cleanup: `chattr -i /tmp/.dockerd`.
- Preload cleanup: `sudo rm -f /etc/ld.so.preload`
