#!/usr/bin/env bash
# T1082 / T1016 — host enumeration for tailored evasion
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=lib/common.sh
source "${SCRIPT_DIR}/lib/common.sh"
require_lab

OUT="${EVASION_STAGING}/host-recon.txt"
{
  echo "=== hostname ==="
  hostname
  echo "=== uname ==="
  uname -a
  echo "=== os-release ==="
  cat /etc/os-release 2>/dev/null || true
  echo "=== ip ==="
  ip -4 addr show 2>/dev/null || ifconfig 2>/dev/null || true
  echo "=== mounts ==="
  mount | head -20
} > "${OUT}"

log "Host recon written to ${OUT}"
cat "${OUT}"
