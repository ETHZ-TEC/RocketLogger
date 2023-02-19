#!/bin/bash
# Cross compile rocketlogger binaries using docker buildx container

REPO_ROOT_PATH=`git rev-parse --show-toplevel`
REPO_REL_PATH=`git rev-parse --show-prefix`

# register ARM executables
docker run --rm --privileged docker/binfmt:a7996909642ee92942dcd6cff44b9b95f08dad64

# inspect existing builder
docker buildx inspect --bootstrap

# build minimal armv7 debian image with compiler tools
docker buildx build --platform linux/arm/v7 --tag rocketlogger_build .

# build rocketlogger binary
docker run --platform linux/arm/v7 --user "$(id -u):$(id -g)" --mount type=bind,source="${REPO_ROOT_PATH}",target=/home/rocketlogger \
    --tty rocketlogger_build /bin/bash -c "cd /home/rocketlogger/${REPO_REL_PATH} && CC=clang CXX=clang++ meson builddir && ninja -C builddir"


# for interactive docker shell use:
#docker run --platform linux/arm/v7 --user "$(id -u):$(id -g)" --mount type=bind,source="${REPO_ROOT_PATH}",target=/home/rocketlogger --tty --interactive rocketlogger_build
