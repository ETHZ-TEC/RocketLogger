#!/bin/bash
# Post install system configuration for RocketLogger software

# source dir
SRC=..

# user folder paths
USER_DATA_DIR="/home/rocketlogger/data/"
USER_CONFIG_DIR="/home/rocketlogger/.config/rocketlogger/"

# systemd service
SERVICE_NAME="rocketlogger.service"

# device tree configuration file to patch
UENV_PATCH="${SRC}/config/uEnv.txt.patch"
UENV_TARGET="/boot/uEnv.txt"


## create user folders
mkdir --parents USER_DATA_DIR
mkdir --parents USER_CONFIG_DIR

## enable and start systemd service
systemctl enable "${SERVICE_NAME}"
systemctl restart "${SERVICE_NAME}"

## patch device tree overlay configuration, with backup
patch --forward --backup --input="${UENV_PATCH}" "${UENV_TARGET}"
