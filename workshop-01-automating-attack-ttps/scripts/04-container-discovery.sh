#!/usr/bin/env bash
# T1613 / T1610 — container enumeration and malicious image pull
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=lib/common.sh
source "${SCRIPT_DIR}/lib/common.sh"
require_lab

export DOCKER_HOST="${DOCKER_API_URL}"

log "Container inventory (T1613)"
docker ps -a --no-trunc > "${STAGING_DIR}/containers-full.txt"
docker images > "${STAGING_DIR}/images.txt"

log "Pulling public image (T1204.003 — malicious image simulation)"
# Uses a minimal public image; in real TeamTNT this would be a backdoored Docker Hub image
timeout 30 docker pull busybox:1.36 2>&1 | tee "${STAGING_DIR}/pull.log" || warn "pull skipped (timeout/offline)"

log "Running pulled image (T1204.003 — user execution)"
docker rm -f teamtnt-pulled-lab 2>/dev/null || true
docker run --name teamtnt-pulled-lab busybox:1.36 \
  sh -c 'echo "[T1204] malicious image executed (lab)"; sleep 2' \
  2>&1 | tee "${STAGING_DIR}/image-run.log" || warn "image run skipped"

log "Inspecting running workloads for sensitive mounts"
docker inspect "$(docker ps -q | head -1)" --format '{{json .HostConfig}}' 2>/dev/null \
  | jq . 2>/dev/null > "${STAGING_DIR}/inspect-sample.json" || true

log "Discovery complete."
