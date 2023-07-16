# RocketLogger System Configuration

This folder includes operating system and RocketLogger default configuration files
* uEnv configuration patch `uEnv.txt.patch` to deploy RocketLogger device tree overlay and activate PRU uio interface
* uio_pruss kernel module parameter configuration file `rocketlogger.conf`
* systemd service configuration for RocketLogger daemon `rocketlogger.service`
* design data based default calibration `calibration.dat`


## Documentation

### PRU Resources
* uEnv configuration to switch from rproc to uio PRU interface: <https://gist.github.com/jonlidgard/1d9e0e92b4f219f3f40edfed260b851e>
