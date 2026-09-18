#!/usr/bin/env bash
# T1595.001 / T1595.002 / T1046 — automated network discovery (lab-fast)
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=lib/common.sh
source "${SCRIPT_DIR}/lib/common.sh"
require_lab

OUT="${STAGING_DIR}/scan-results.txt"

# In-lab service ports (host-published 8080/8180 map to container :80)
declare -A HOST_PORTS=(
  [docker-api]="2375 10250 10255"
  [victim-app]="80"
  [metadata-sim]="80"
)

log "Scanning lab targets (TeamTNT-style service discovery)"
: > "${OUT}"

for host in "${!HOST_PORTS[@]}"; do
  ip="$(getent hosts "${host}" 2>/dev/null | awk '{print $1}' | head -1 || true)"
  [[ -z "${ip}" ]] && { warn "Cannot resolve ${host} — skipping"; continue; }
  for port in ${HOST_PORTS[$host]}; do
    if timeout 2 bash -c "echo >/dev/tcp/${ip}/${port}" 2>/dev/null; then
      printf 'open tcp %s %s %s\n' "${port}" "${ip}" "$(date +%s)" >> "${OUT}"
      log "  OPEN ${host} (${ip}):${port}"
    fi
  done
done

if command -v nmap >/dev/null 2>&1; then
  nmap -Pn -p 80,2375,10250,10255 --open --host-timeout 5s -oN "${STAGING_DIR}/nmap-detail.txt" \
    docker-api victim-app metadata-sim 2>/dev/null || true
fi

# Debrief: real TeamTNT mass-scans cloud CIDRs (T1595.001) — not run here (too slow for 50 min)
if command -v masscan >/dev/null 2>&1; then
  log "masscan debrief: masscan <cloud-CIDR> -p2375,10250 --rate=10000"
fi

log "Results written to ${OUT}"
if [[ -s "${OUT}" ]]; then
  cat "${OUT}"
else
  warn "No open ports found — is the lab running? (./setup-lab.sh)"
  exit 1
fi
