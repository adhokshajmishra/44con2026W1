#!/usr/bin/env bash
set -euo pipefail

export TEAMTNT_EVASION_LAB="${TEAMTNT_EVASION_LAB:-1}"
export EVASION_STAGING="/tmp/teamtnt-evasion"
export MOCK_MINER_NAME="${MOCK_MINER_NAME:-k8s-guardian}"
export MOCK_MINER_PATH="${MOCK_MINER_PATH:-/tmp/.dockerd}"

mkdir -p "${EVASION_STAGING}"

log()  { printf '[evasion] %s\n' "$*"; }
warn() { printf '[evasion][WARN] %s\n' "$*" >&2; }
die()  { printf '[evasion][ERROR] %s\n' "$*" >&2; exit 1; }

require_lab() {
  [[ "${TEAMTNT_EVASION_LAB:-}" == "1" ]] || die "Set TEAMTNT_EVASION_LAB=1 for lab mode."
}

require_root() {
  [[ "$(id -u)" -eq 0 ]] || die "This step requires root (use sudo)."
}
