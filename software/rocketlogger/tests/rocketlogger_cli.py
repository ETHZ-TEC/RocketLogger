"""
RocketLogger CLI helpers
"""

import os
import math
import shutil
import subprocess


_ROCKETLOGGER_BINARY = "rocketlogger"
_DEFAULT_SAMPLE_RATE = 1000
_DEFAULT_SAMPLE_COUNT = 5 * _DEFAULT_SAMPLE_RATE
_DEFAULT_FILE_COMMENT = "RocketLogger system test"
_DEFAULT_FILE_SIZE = 100 * 10**6

sample_rates = [1000, 2000, 4000, 8000, 16000, 32000, 64000]
update_rates = [1, 2, 5, 10]
channels_voltage = ["V1", "V2", "V3", "V4"]
channels_current = ["I1L", "I1H", "I2L", "I2H"]
channels_analog = channels_voltage + channels_current
channels_digital = ["DI1", "DI2", "DI3", "DI4", "DI5", "DI6"]


def getBinary(system_binary=False):
    """
    Get the RocketLogger CLI binary.

    :param system_binary: True to return system installed binary, False to return local built binary
    """
    if system_binary:
        binary = shutil.which(_ROCKETLOGGER_BINARY)
    elif os.path.exists("builddir"):
        binary = os.path.join("builddir", _ROCKETLOGGER_BINARY)
    else:
        binary = os.path.join(os.path.pardir, "builddir", _ROCKETLOGGER_BINARY)

    if not os.path.exists(binary):
        raise FileNotFoundError(
            f"Could not find requested RocketLogger CLI binary! [{binary}]"
        )
    return os.path.abspath(binary)


def getDefaultConfig():
    """
    Get the default RocketLogger CLI configuration
    """
    config = {
        "samples": _DEFAULT_SAMPLE_COUNT,
        "channel": "all",
        "rate": _DEFAULT_SAMPLE_RATE,
        "update": 1,
        "output": "data.rld",
        "format": "rld",
        "size": _DEFAULT_FILE_SIZE,
        "comment": _DEFAULT_FILE_COMMENT,
        "digital": True,
        "ambient": False,
        "aggregate": "downsample",
        "high-range": [],
        "web": False,
    }
    return config


def configToCliArguments(config):
    """
    Get CLI arguments for configuration.

    :param config: The configuration to process

    :return: The configuration arguments as list
    """
    if not isinstance(config, dict):
        raise TypeError("Expected dict for config")

    args = []
    for key, value in config.items():
        if value == None:
            args.append(f"--{key}")
            continue

        if isinstance(value, list):
            value = ",".join(value)
        args.append(f"--{key}={value}")

    return args


def runMeasurement(config, system_binary=False):
    """
    Run RocketLogger Measurement.

    :param config: The configuration to process

    :param system_binary: True to use system installed instead of local built binary

    :return: The subprocess result
    """
    if config["samples"] <= 0:
        raise ValueError("Cannot start measurement without limited number of samples")

    timeout = (
        3
        + math.log2(config["rate"] / 1000)
        + 1.05 * (config["samples"] / config["rate"])
    )
    binary = getBinary(system_binary)
    args = configToCliArguments(config)

    result = subprocess.run(
        [binary, "start"] + args, timeout=timeout, check=True, capture_output=True
    )

    return result
