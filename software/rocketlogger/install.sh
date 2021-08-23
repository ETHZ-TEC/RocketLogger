#!/bin/bash
# Post install system configuration for RocketLogger software

# source dir
SRC=..

# default folder paths
DEFAULT_LOG_DIR="/var/log/flocklab/"

# user folder paths
USER_DATA_DIR="/home/flocklab/data/"
USER_CONFIG_DIR="/home/flocklab/.config/rocketlogger/"

# systemd service
SERVICE_NAME="rocketlogger.service"

# device tree configuration file to patch
UENV_PATCH="${SRC}/config/uEnv.txt.patch"
UENV_TARGET="/boot/uEnv.txt"


## create user and log folders
mkdir --parents DEFAULT_LOG_DIR
mkdir --parents USER_CONFIG_DIR
mkdir --parents USER_DATA_DIR

## enable and start systemd service
systemctl enable "${SERVICE_NAME}"
systemctl restart "${SERVICE_NAME}"

## patch device tree overlay configuration, with backup
patch --forward --backup --input="${UENV_PATCH}" "${UENV_TARGET}"

# succeed even if uEnv config was patched already
exit 0
