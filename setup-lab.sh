#!/usr/bin/env bash
# 44CON TeamTNT workshops — bring up the shared lab environment
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LAB_DIR="${ROOT}/workshop-01-automating-attack-ttps/lab"

echo "==> 44CON TeamTNT lab setup"
echo "    Root: ${ROOT}"

if ! command -v docker >/dev/null 2>&1; then
  echo "ERROR: docker is required." >&2
  exit 1
fi

if ! docker compose version >/dev/null 2>&1; then
  echo "ERROR: docker compose plugin is required." >&2
  exit 1
fi

echo "==> Building and starting vulnerable targets..."
cd "${LAB_DIR}"
docker compose up -d --build

echo ""
echo "==> Lab endpoints (attack from host or attacker container):"
docker compose ps
echo ""
echo "  Docker API (misconfigured):  tcp://127.0.0.1:2375"
echo "  Metadata simulator (IMDSv1):   http://127.0.0.1:8180/latest/meta-data/ (in-lab: http://metadata-sim/...)"
echo "  Victim web app:                http://127.0.0.1:8080/"
echo "  Attacker shell:                docker compose exec attacker bash"
echo ""
echo "Workshop 1 shell:  docker compose -f workshop-01-automating-attack-ttps/lab/docker-compose.yml exec attacker /opt/teamtnt/scripts/teamtnt-chain.sh"
echo "Workshop 1 native: docker compose -f workshop-01-automating-attack-ttps/lab/docker-compose.yml exec attacker /opt/teamtnt/native_automation/build-docker/teamtnt_native"
echo "Workshop 2 shell:  docker compose -f workshop-01-automating-attack-ttps/lab/docker-compose.yml exec attacker /opt/teamtnt/evasion/scripts/evasion-chain.sh"
echo "Workshop 2 native: docker compose -f workshop-01-automating-attack-ttps/lab/docker-compose.yml exec attacker /opt/teamtnt/evasion/native_automation/build-docker/evasion_native"
echo ""
echo "Teardown: cd workshop-01-automating-attack-ttps/lab && docker compose down -v"
