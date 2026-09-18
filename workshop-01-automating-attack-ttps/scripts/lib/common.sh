#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CONFIG_DIR="$(cd "${SCRIPT_DIR}/../../config" && pwd)"

# shellcheck source=/dev/null
source "${CONFIG_DIR}/targets.env"

mkdir -p "${STAGING_DIR}"

log()  { printf '[teamtnt] %s\n' "$*"; }
warn() { printf '[teamtnt][WARN] %s\n' "$*" >&2; }
die()  { printf '[teamtnt][ERROR] %s\n' "$*" >&2; exit 1; }

require_lab() {
  [[ "${TEAMTNT_LAB:-}" == "1" ]] || die "Set TEAMTNT_LAB=1 or run inside the attacker container."
}

stage_file() {
  local src="$1"
  local name="$2"
  cp -f "$src" "${STAGING_DIR}/${name}"
  log "Staged: ${STAGING_DIR}/${name}"
}
