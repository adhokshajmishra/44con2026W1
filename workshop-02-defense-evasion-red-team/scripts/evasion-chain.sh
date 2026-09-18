#!/usr/bin/env bash
# Master defense evasion chain — post-compromise TeamTNT playbook (lab only)
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

export TEAMTNT_EVASION_LAB=1

steps=(
  "01-host-recon.sh"
  "02-discover-security-agents.sh"
  "03-masquerade-miner.sh"
  "04-ld-preload-hide.sh"
  "05-anti-forensics.sh"
  "06-chattr-immutable.sh"
)

echo "=============================================="
echo " TeamTNT Defense Evasion Chain — 44CON Lab"
echo "=============================================="

for step in "${steps[@]}"; do
  echo ""
  echo ">>> Running ${step}"
  bash "${SCRIPT_DIR}/${step}"
  sleep 1
done

echo ""
echo "=============================================="
echo " Evasion chain complete."
echo " Blue team exercises: ps aux, lsattr, journalctl, ld.so.preload audit"
echo "=============================================="
