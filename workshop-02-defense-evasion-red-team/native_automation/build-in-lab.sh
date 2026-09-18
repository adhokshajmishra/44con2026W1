#!/usr/bin/env bash
# Build inside the attacker container — uses build-docker/ to avoid clashing with host build/
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${ROOT}/build-docker"

if [[ -f "${ROOT}/build/CMakeCache.txt" ]] && grep -q "/Users/" "${ROOT}/build/CMakeCache.txt" 2>/dev/null; then
  echo "[build-in-lab] Ignoring host build/ cache (macOS paths)"
fi

[[ -f "${ROOT}/CMakeLists.txt" ]] || { echo "CMakeLists.txt not found in ${ROOT}"; exit 1; }

rm -rf "${BUILD_DIR}"
cmake -S "${ROOT}" -B "${BUILD_DIR}" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_COMPILER=g++
cmake --build "${BUILD_DIR}" -j
echo "Built: ${BUILD_DIR}/evasion_native"
