#!/bin/bash
# Cross compile rocketlogger binaries using docker buildx container

REPO_ROOT_PATH=`git rev-parse --show-toplevel`
REPO_REL_PATH=`git rev-parse --show-prefix`

# register ARM executables
docker run --rm --privileged docker/binfmt:a7996909642ee92942dcd6cff44b9b95f08dad64

# inspect existing builder
docker buildx inspect --bootstrap

# build minimal armv7 debian image with compiler tools
docker buildx build --platform linux/arm/v7 --tag rocketlogger_stretch_build .

# build rocketlogger binary
docker run --platform linux/arm/v7 --user "$(id -u):$(id -g)" --mount type=bind,source="$(pwd)",target=/home/rocketlogger \
    --tty rocketlogger_stretch_build /bin/bash -c "cd rocketlogger && meson builddir && cd builddir && ninja"


# for interactive docker shell use:
#docker run --platform linux/arm/v7 --user "$(id -u):$(id -g)" --mount type=bind,source="$(pwd)/rocketlogger",target=/home/rocketlogger --tty --interactive rocketlogger_stretch_build
