#!/bin/sh
set -eu

DIND_HOST="${DIND_HOST:-tcp://docker-api:2375}"
VICTIM_IMAGE="${VICTIM_IMAGE:-lab-victim-app}"

echo "[bootstrap] Waiting for Docker API at ${DIND_HOST}..."
until docker -H "${DIND_HOST}" info >/dev/null 2>&1; do sleep 2; done

echo "[bootstrap] Waiting for host image ${VICTIM_IMAGE}..."
until docker image inspect "${VICTIM_IMAGE}" >/dev/null 2>&1; do sleep 2; done

echo "[bootstrap] Loading ${VICTIM_IMAGE} into lab Docker API..."
docker save "${VICTIM_IMAGE}" | docker -H "${DIND_HOST}" load

TAG="$(docker image inspect "${VICTIM_IMAGE}" --format '{{.Id}}')"
echo "[bootstrap] Imported image ${TAG}"

docker -H "${DIND_HOST}" rm -f victim-app 2>/dev/null || true
docker -H "${DIND_HOST}" run -d --name victim-app --hostname victim-app "${VICTIM_IMAGE}"

echo "[bootstrap] Victim seeded inside Docker API."
docker -H "${DIND_HOST}" ps
