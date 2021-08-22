# RocketLogger image build environment
FROM arm32v7/debian:bullseye-slim
WORKDIR /opt

# Debian package dependencies
RUN apt-get update && apt-get install --assume-yes  \
    clang                           \
    curl                            \
    device-tree-compiler            \
    git                             \
    meson                           \
    ninja-build                     \
    pkg-config                      \
    libgpiod-dev                    \
    libi2c-dev                      \
    libncurses5-dev                 \
    libzmq3-dev

# update and rehash certificates (see also: https://blog.abgefaerbt.de/troubleshooting/docker/buildx/arm/curl/certificates/update-ca-certificates/docker-buildx-clamav-curl-unable-to-get-local-issuer-certificate/)
RUN update-ca-certificates && c_rehash

# TI Code Generation Tools for PRU
ARG CGT_PRU_VERSION=2.3.3
RUN curl --location --remote-name https://software-dl.ti.com/codegen/esd/cgt_public_sw/PRU/${CGT_PRU_VERSION}/ti_cgt_pru_${CGT_PRU_VERSION}_armlinuxa8hf_busybox_installer.sh &&  \
    chmod +x ti_cgt_pru_${CGT_PRU_VERSION}_armlinuxa8hf_busybox_installer.sh &&  \
    ./ti_cgt_pru_${CGT_PRU_VERSION}_armlinuxa8hf_busybox_installer.sh &&  \
    rm ti_cgt_pru_${CGT_PRU_VERSION}_armlinuxa8hf_busybox_installer.sh

# default shell config
WORKDIR /home
CMD ["bash"]
