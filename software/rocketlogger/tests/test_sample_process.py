"""
RocketLogger system tests
* sample with parametrized measurement configurations
* basic measurement data import using the Python support library
"""

import math
import pytest
import rocketlogger_cli as cli
from rocketlogger.data import RocketLoggerData


@pytest.fixture
def measurement_config(tmpdir):
    config = cli.getDefaultConfig()
    config["output"] = tmpdir.join("test.rld")
    yield config


def sample_load_assert(config):
    result = cli.runMeasurement(config)
    assert result.returncode == 0

    data = RocketLoggerData(config["output"])
    header = data.get_header()
    assert header["sample_count"] == config["samples"]


@pytest.mark.parametrize("rate", cli.sample_rates)
def test_default(measurement_config, rate):
    measurement_config["rate"] = rate
    measurement_config["samples"] = 5 * rate
    sample_load_assert(measurement_config)


@pytest.mark.parametrize("rate", cli.sample_rates)
def test_voltage_only(measurement_config, rate):
    measurement_config["rate"] = rate
    measurement_config["samples"] = 5 * rate
    measurement_config["channel"] = cli.channels_voltage
    measurement_config["digital"] = False
    sample_load_assert(measurement_config)


@pytest.mark.parametrize("rate", cli.sample_rates)
def test_current_only(measurement_config, rate):
    measurement_config["rate"] = rate
    measurement_config["samples"] = 5 * rate
    measurement_config["channel"] = cli.channels_current
    measurement_config["digital"] = False
    sample_load_assert(measurement_config)


@pytest.mark.parametrize("rate", cli.sample_rates)
def tes_digital_only(measurement_config, rate):
    measurement_config["rate"] = rate
    measurement_config["samples"] = 5 * rate
    measurement_config["channel"] = []
    measurement_config["digital"] = True
    sample_load_assert(measurement_config)


@pytest.mark.parametrize("rate", cli.sample_rates)
def test_split_default(measurement_config, rate):
    measurement_config["rate"] = rate
    measurement_config["size"] = 5 * 10**6
    measurement_config["samples"] = (
        math.ceil(measurement_config["size"] / (8 * 4) / measurement_config["rate"])
        * measurement_config["rate"]
    )
    sample_load_assert(measurement_config)
