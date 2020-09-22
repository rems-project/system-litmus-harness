FROM debian:10-slim

SHELL ["/bin/bash", "-o", "pipefail", "-c"]

ARG GUEST_UID=1000
ARG GUEST_GID=${GUEST_UID}
ARG GUEST_USER=rems
ARG GUEST_GROUP=${GUEST_USER}

# Add a user with sudo perms
RUN groupadd -g ${GUEST_GID} ${GUEST_GROUP} \
        && useradd --no-log-init -m -s /bin/bash -g ${GUEST_GROUP} -G sudo -p '' -u ${GUEST_UID} ${GUEST_USER} \
        && mkdir -p /home/${GUEST_USER}/bin \
        && chown ${GUEST_UID}:${GUEST_GID} /home/${GUEST_USER}/bin

# get packages
RUN apt-get update -y -q \
        && DEBIAN_FRONTEND=noninteractive apt-get install -y -q --no-install-recommends \
            git \
            make \
            sudo \
            gcc-aarch64-linux-gnu\
            qemu-system-aarch64 \
            python3 \
        && rm -rf /var/lib/apt/lists/*

# use GUEST_USER rather than root
USER ${GUEST_USER}
WORKDIR /home/${GUEST_USER}

# clone system-litmus-harness

# for this COPY to work the context must be system-litmus-harness/
# e.g. build with `docker build -f- path/to/system-litmus-harness < Dockerfile`
# we also need to own the dir
COPY --chown=${GUEST_UID}:${GUEST_GID} . /home/${GUEST_USER}/system-litmus-harness/

# build
RUN cd /home/${GUEST_USER}/system-litmus-harness/ \
    && make build