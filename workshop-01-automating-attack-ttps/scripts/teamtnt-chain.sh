#!/usr/bin/env bash
# Master automation chain — TeamTNT-style worm pipeline (lab only)
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

export TEAMTNT_LAB=1
warn() { printf '[teamtnt][WARN] %s\n' "$*" >&2; }

steps=(
  "01-scan-targets.sh"
  "02-probe-docker-api.sh"
  "03-harvest-credentials.sh"
  "04-container-discovery.sh"
  "05-escape-and-deploy.sh"
  "06-lateral-movement.sh"
  "07-mock-miner.sh"
)

echo "=============================================="
echo " TeamTNT Automated Attack Chain — 44CON Lab"
echo "=============================================="

for step in "${steps[@]}"; do
  echo ""
  echo ">>> Running ${step}"
  timeout 120 bash "${SCRIPT_DIR}/${step}" || warn "Step ${step} timed out or failed — continue demo manually"
  sleep 1
done

echo ""
echo "=============================================="
echo " Chain complete. Staging dir: /tmp/teamtnt-staging"
echo " Next: Workshop 2 — defense evasion on compromised host"
echo "=============================================="
