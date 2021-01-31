#!/bin/bash

# register ARM executables
docker run --rm --privileged docker/binfmt:a7996909642ee92942dcd6cff44b9b95f08dad64

# inspect existing builder
docker buildx inspect --bootstrap

# build minimal armv7 debian image with compiler tools
docker buildx build --platform linux/arm/v7 -t beaglebone_builder .

# build rocketlogger binary
docker run --platform linux/arm/v7 --user "$(id -u):$(id -g)" --mount type=bind,source="$(pwd)/rocketlogger",target=/home/rocketlogger \
    -t beaglebone_builder /bin/bash -c "cd rocketlogger && meson builddir && cd builddir && ninja"
