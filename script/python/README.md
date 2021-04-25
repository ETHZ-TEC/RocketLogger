# RocketLogger Python Support Library

This package provides RocketLogger data file handling support, as well as
basic processing and plotting of the data. Further is provides the necessary
support to generate calibration data from measurements.

**Dependencies**
* Python 3: version 3.6-3.9
* NumPy: version 1.13-1.20

**Optional dependencies**
* Matplotlib: for plotting data overview
* pandas: for pandas DataFrame export


## Data Import

To import a RocketLogger data (.rld) file, use the `RocketLoggerData` class:
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

A list of the channels existing in the loaded file is provided by:
```py
>>> ch = rld.get_channel_names()
```

To plot a preview of the data (requires Matplotlib):
```py
>>> p = rld.plot()
```

You can also plot the (merged) file data a single command, e.g. for preview:
```py
>>> p = RocketLoggerData('data.rld').merge_channels().plot()
```

For analysis using pandas, the use the following shortcut to get a time-
indexed dataframe (requires pandas):
```py
>>> df = rld.get_dataframe()
```

For more details about the individual functions and thier parameters, refer to
the documentation available at <https://rocketlogger.ethz.ch/python/>.


## RocketLogger Device Calibration

The `RocketLoggerCalibration` class from the `calibration` module provides the support
necessary for generating RocketLogger device calibration. See the calibration section
in the package documentation <https://rocketlogger.ethz.ch/python/> and the
RocketLogger wiki at <https://rocketlogger.ethz.ch/wiki/> for more details on the
calibration procedure.
