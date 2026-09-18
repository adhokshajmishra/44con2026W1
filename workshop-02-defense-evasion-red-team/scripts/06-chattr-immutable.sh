#!/usr/bin/env bash
# T1222.002 — make miner binary immutable to hinder remediation
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=lib/common.sh
source "${SCRIPT_DIR}/lib/common.sh"
require_lab

TARGET="${MOCK_MINER_PATH}"

[[ -f "${TARGET}" ]] || die "Miner not found at ${TARGET} — run 03-masquerade-miner.sh first"

if [[ "$(id -u)" -eq 0 ]] && command -v chattr >/dev/null 2>&1; then
  chattr +i "${TARGET}"
  log "Applied chattr +i to ${TARGET} (T1222.002)"
  lsattr "${TARGET}"
  log "Remediation requires: chattr -i ${TARGET}"
else
  log "Dry-run: would run chattr +i ${TARGET}"
  log "Blue team detection: lsattr, auditd, FIM on immutable attribute changes"
fi
