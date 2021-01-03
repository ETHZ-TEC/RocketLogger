# RocketLogger binary builder image
FROM arm32v7/debian:buster-slim
WORKDIR /opt

# Debian package dependencies
RUN apt-get update && apt-get install --assume-yes  \
    device-tree-compiler    \
    gcc                     \
    git                     \
    meson                   \
    ninja-build             \
    pkg-config              \
    wget                    \
    libi2c-dev              \
    libncurses5-dev         \
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
