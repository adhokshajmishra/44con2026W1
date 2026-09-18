FROM debian:bookworm-slim

RUN apt-get update && apt-get install -y --no-install-recommends \
    bash curl wget jq nmap masscan docker.io openssh-client sshpass ca-certificates \
    build-essential cmake libcurl4-openssl-dev pkg-config \
    && rm -rf /var/lib/apt/lists/*

ENV TEAMTNT_LAB=1
ENV STAGING_DIR=/tmp/teamtnt-staging
ENV DOCKER_API_URL=tcp://docker-api:2375
ENV METADATA_URL=http://metadata-sim/latest/meta-data
ENV SSH_LATERAL_TARGET=lateral-ssh-target
ENV TEAMTNT_EVASION_LAB=1
ENV EVASION_STAGING=/tmp/teamtnt-evasion
ENV MOCK_MINER_PATH=/tmp/.dockerd
ENV MOCK_MINER_NAME=k8s-guardian
ENV PRELOAD_SOURCE=/opt/teamtnt/evasion/lib/libprocesshider-demo.c
WORKDIR /opt/teamtnt
