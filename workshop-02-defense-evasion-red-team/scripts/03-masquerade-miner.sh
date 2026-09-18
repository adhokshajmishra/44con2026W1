#!/usr/bin/env bash
# T1036 / T1036.005 / T1027.013 / T1140 — masquerade and obfuscate payload
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=lib/common.sh
source "${SCRIPT_DIR}/lib/common.sh"
require_lab

log "Creating masqueraded miner binary path (T1036.005)"
cat > "${MOCK_MINER_PATH}" <<'EOF'
#!/bin/sh
# Masqueraded as dockerd — mock CPU burn for lab
while true; do dd if=/dev/zero of=/dev/null bs=1M count=1 2>/dev/null; sleep 2; done
EOF
chmod +x "${MOCK_MINER_PATH}"

log "Base64 obfuscation layer (T1027.013)"
base64 < "${MOCK_MINER_PATH}" | tr -d '\n' > "${EVASION_STAGING}/payload.b64"
log "Encoded payload: ${EVASION_STAGING}/payload.b64"

log "Decode and execute (T1140)"
base64 -d -i "${EVASION_STAGING}/payload.b64" -o "${EVASION_STAGING}/decoded-miner.sh" 2>/dev/null \
  || base64 -d < "${EVASION_STAGING}/payload.b64" > "${EVASION_STAGING}/decoded-miner.sh"
chmod +x "${EVASION_STAGING}/decoded-miner.sh"

nohup "${MOCK_MINER_PATH}" >/dev/null 2>&1 &
echo $! > "${EVASION_STAGING}/miner.pid"
log "Mock miner running as '${MOCK_MINER_PATH}' (PID $(cat "${EVASION_STAGING}/miner.pid"))"
ps aux | grep -E '[.]dockerd|decoded-miner' | grep -v grep || true
