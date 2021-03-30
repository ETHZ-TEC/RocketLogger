# RocketLogger image build environment
FROM arm32v7/debian:buster-slim
WORKDIR /opt

# set up buster-backports for meson and ninja
RUN echo "deb http://deb.debian.org/debian buster-backports main" >> /etc/apt/sources.list.d/buster-backports.list

# Debian package dependencies
RUN apt-get update && apt-get install --assume-yes  \
    device-tree-compiler            \
    gcc                             \
    git                             \
    meson/buster-backports          \
    ninja-build/buster-backports    \
    pkg-config                      \
    wget                            \
    libi2c-dev                      \
    libncurses5-dev                 \
    libzmq3-dev

# TI Code Generation Tools for PRU
ARG CGT_PRU_VERSION=2.3.3
RUN wget http://software-dl.ti.com/codegen/esd/cgt_public_sw/PRU/${CGT_PRU_VERSION}/ti_cgt_pru_${CGT_PRU_VERSION}_armlinuxa8hf_busybox_installer.sh  &&  \
    chmod +x ti_cgt_pru_${CGT_PRU_VERSION}_armlinuxa8hf_busybox_installer.sh &&  \
    ./ti_cgt_pru_${CGT_PRU_VERSION}_armlinuxa8hf_busybox_installer.sh &&  \
    rm ti_cgt_pru_${CGT_PRU_VERSION}_armlinuxa8hf_busybox_installer.sh

# default shell config
WORKDIR /home
CMD ["bash"]
