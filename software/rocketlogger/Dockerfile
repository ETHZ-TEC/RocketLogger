# RocketLogger image build environment
FROM arm32v7/debian:stretch-slim
WORKDIR /opt

# set up stretch-backports for meson and ninja
RUN echo "deb http://deb.debian.org/debian stretch-backports main" >> /etc/apt/sources.list.d/stretch-backports.list

# Add BeagleBone stretch repo for updated device-tree-compiler
RUN apt-get update && apt-get install --assume-yes gnupg
RUN apt-key adv --keyserver keyserver.ubuntu.com --recv-key D284E608A4C46402
RUN echo "deb [arch=armhf] http://repos.rcn-ee.com/debian/ stretch main" >> /etc/apt/sources.list

# Debian package dependencies
RUN apt-get update && apt-get install --assume-yes  \
    curl                            \
    device-tree-compiler            \
    gcc                             \
    git                             \
    meson/stretch-backports         \
    ninja-build                     \
    pkg-config                      \
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
