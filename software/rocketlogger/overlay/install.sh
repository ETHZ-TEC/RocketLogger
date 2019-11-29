#!/bin/bash
# Compile and install the RocketLogger device tree overlay

# clone beaglebone device tree repo
git clone https://github.com/beagleboard/bb.org-overlays.git

# copy RocketLogger device tree overlay and compile binary
cp -f BB-FLOCKLAB2.dts bb.org-overlays/src/arm
(cd bb.org-overlays && make src/arm/BB-FLOCKLAB2.dtbo)

# deploy device tree binary
sudo cp -f bb.org-overlays/src/arm/BB-FLOCKLAB2.dtbo /lib/firmware
