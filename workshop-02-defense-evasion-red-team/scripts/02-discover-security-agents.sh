#!/usr/bin/env bash
# T1518.001 / T1007 / T1685 — discover and simulate disabling cloud security agents
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=lib/common.sh
source "${SCRIPT_DIR}/lib/common.sh"
require_lab

OUT="${EVASION_STAGING}/security-agents.txt"
PATTERNS='aliyun|aegis|qcloud|tencent|bmc-agent|falco|sentinel|crowdstrike|elastic-agent'

log "Process scan for security tooling (T1518.001)"
ps aux 2>/dev/null | grep -iE "${PATTERNS}" | grep -v grep > "${OUT}" || true

log "Service enumeration (T1007)"
if command -v systemctl >/dev/null 2>&1; then
  systemctl list-units --type=service --all 2>/dev/null \
    | grep -iE "${PATTERNS}" >> "${OUT}" || true
fi

if [[ ! -s "${OUT}" ]]; then
  echo "(no matching agents in lab — expected)" >> "${OUT}"
fi

log "Simulating agent disable (T1685) — lab dry-run only"
cat > "${EVASION_STAGING}/disable-agents.sh" <<'EOF'
#!/usr/bin/env bash
# TeamTNT would: systemctl stop/disable or uninstall vendor agents
# LAB: print only — do not run against production
for svc in aliyun.service aegis.service bmc-agent.service; do
  systemctl is-active "$svc" 2>/dev/null && echo "would disable: $svc"
done
EOF
chmod +x "${EVASION_STAGING}/disable-agents.sh"

cat "${OUT}"
