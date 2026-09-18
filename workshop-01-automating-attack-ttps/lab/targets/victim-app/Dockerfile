FROM debian:bookworm-slim

RUN apt-get update && apt-get install -y --no-install-recommends \
    nginx openssh-client curl ca-certificates \
    && rm -rf /var/lib/apt/lists/*

RUN useradd -m devops && \
    mkdir -p /home/devops/.aws /home/devops/.ssh /home/devops/.docker

COPY shared-ssh-keys/lab_devops /home/devops/.ssh/id_rsa
COPY shared-ssh-keys/lab_devops.pub /home/devops/.ssh/id_rsa.pub
COPY victim-app/index.html /var/www/html/index.html

RUN printf '[default]\naws_access_key_id = AKIA44CONFAKEKEY01\naws_secret_access_key = wJalrXUtnFEMI/K7MDENG/bPxRfiCY44CONFKEY\n' \
      > /home/devops/.aws/credentials && \
    printf '{"auths":{"https://index.docker.io/v1/":{"auth":"dGVhbXRudDpsYWJwd2Q="}}}\n' \
      > /home/devops/.docker/config.json && \
    printf 'devops:TeamTNT-Lab-SSH!\n' > /home/devops/.lab_ssh_creds && \
    chmod 600 /home/devops/.ssh/id_rsa && \
    chmod 644 /home/devops/.ssh/id_rsa.pub /home/devops/.lab_ssh_creds && \
    chmod -R a+rX /home/devops/.aws /home/devops/.docker && \
    chown -R devops:devops /home/devops

EXPOSE 80
CMD ["nginx", "-g", "daemon off;"]
