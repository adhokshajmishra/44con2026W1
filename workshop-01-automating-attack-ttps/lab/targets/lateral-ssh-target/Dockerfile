FROM debian:bookworm-slim

RUN apt-get update && apt-get install -y --no-install-recommends openssh-server \
    && rm -rf /var/lib/apt/lists/*

RUN useradd -m devops \
    && echo 'devops:TeamTNT-Lab-SSH!' | chpasswd \
    && mkdir -p /home/devops/.ssh \
    && chmod 700 /home/devops/.ssh

COPY shared-ssh-keys/lab_devops.pub /home/devops/.ssh/authorized_keys
RUN chown -R devops:devops /home/devops/.ssh \
    && chmod 700 /home/devops/.ssh \
    && chmod 600 /home/devops/.ssh/authorized_keys

# Allow password auth for T1021.004 lab demo (training network only)
RUN sed -i 's/#PasswordAuthentication yes/PasswordAuthentication yes/' /etc/ssh/sshd_config \
    && sed -i 's/PasswordAuthentication no/PasswordAuthentication yes/' /etc/ssh/sshd_config

RUN mkdir -p /var/run/sshd

EXPOSE 22
CMD ["/usr/sbin/sshd", "-D", "-e"]
