#!/bin/bash
# Post install system configuration for RocketLogger software

# source dir
SRC=..

# systemd service
SERVICE_NAME="rocketlogger.service"

# device tree configuration file to patch
UENV_PATCH="${SRC}/config/uEnv.txt.patch"
UENV_TARGET="/boot/uEnv.txt"


## enable and start systemd service
systemctl enable "${SERVICE_NAME}"
systemctl restart "${SERVICE_NAME}"

## patch device tree overlay configuration, with backup
patch --forward --backup --input="${UENV_PATCH}" "${UENV_TARGET}"

# succeed even if uEnv config patched already
exit 0
