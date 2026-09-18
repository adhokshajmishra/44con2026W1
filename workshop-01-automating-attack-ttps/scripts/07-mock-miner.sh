#!/usr/bin/env bash
# T1496.001 — deploy mock cryptominer (no real pool traffic)
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=lib/common.sh
source "${SCRIPT_DIR}/lib/common.sh"
require_lab

export DOCKER_HOST="${DOCKER_API_URL}"
MINER_NAME="k8s-guardian-miner"

log "Deploying mock XMRig container (T1496.001)"
docker rm -f "${MINER_NAME}" 2>/dev/null || true

docker run -d --name "${MINER_NAME}" \
  --cpus=0.25 \
  busybox:1.36 \
  sh -c 'while true; do dd if=/dev/zero of=/dev/null bs=1M count=2 2>/dev/null; sleep 1; done'

log "Miner container started: ${MINER_NAME}"
docker stats --no-stream "${MINER_NAME}" 2>/dev/null || docker ps --filter "name=${MINER_NAME}"
