# RocketLogger Python Support Library

This package provides RocketLogger data file handling support, as well as
basic processing and plotting of the data. Further, it provides the necessary
support to generate calibration data from measurements.

**Dependencies**
* Python 3: version 3.6-3.11
* NumPy: version 1.13-1.25

**Optional dependencies**
* Matplotlib: for plotting data overview
* pandas: for pandas DataFrame export

**Compatibility**
* Data processing: supports all officially specified RLD file version (versions 2-4)
* Calibration: compatible with RocketLogger calibration file version 2


## Installation

The package is available from the [Python Package Index](https://pypi.org/project/rocketlogger/)
Install the package using pip:
```bash
python -m pip install rocketlogger
```


## Getting Started

### RocketLogger Data Processing

To import a RocketLogger data (`*.rld`) file, use the `RocketLoggerData` class:
```py
>>> from rocketlogger.data import RocketLoggerData
>>> rld = RocketLoggerData('data.rld')
```

To merge channels with auto-ranging, i.e. the current channels:
```py
>>> rld.merge_channels()
```

To get the loaded channel data (by name) and corresponding timestamps:
```py
>>> d = rld.get_data(['V1', 'I1'])
>>> t = rld.get_time()
```

For more details about the individual functions and their parameters, refer to
the documentation available at <https://github.com/ETHZ-TEC/RocketLogger/wiki/python>.


### RocketLogger Device Calibration

The `RocketLoggerCalibration` class from the `rocketlogger.calibration` module
provides the necessary support for generating RocketLogger device calibration.
See the [Calibration](https://github.com/ETHZ-TEC/RocketLogger/wiki/calibration) wiki section for more details on the calibration
procedure.


## Documentation

The documentation for the RocketLogger is found in the wiki pages at
<https://github.com/ETHZ-TEC/RocketLogger/wiki>.


## License

The RocketLogger Project is released under [3-clause BSD license](https://opensource.org/licenses/BSD-3-Clause).
For more details, refer the the [LICENSE](LICENSE) file.
