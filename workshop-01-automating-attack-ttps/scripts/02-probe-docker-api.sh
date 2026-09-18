#!/usr/bin/env bash
# T1133 / T1059.013 — probe misconfigured Docker API
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=lib/common.sh
source "${SCRIPT_DIR}/lib/common.sh"
require_lab

export DOCKER_HOST="${DOCKER_API_URL}"

log "Probing Docker API at ${DOCKER_HOST}"
docker version >/dev/null 2>&1 || die "Docker API unreachable — is the lab running?"

log "Enumerating containers (T1613)"
docker ps -a --format 'table {{.ID}}\t{{.Image}}\t{{.Status}}\t{{.Names}}' | tee "${STAGING_DIR}/docker-ps.txt"

log "Checking for privileged escape vectors (T1611)"
docker info --format '{{.SecurityOptions}}' | tee "${STAGING_DIR}/docker-info.txt"

log "Docker API is unauthenticated — initial access confirmed."
