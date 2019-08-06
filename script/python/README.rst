RocketLogger Python Support Library
===================================

This package provides RocketLogger data file handling support, basic data
processing and plotting of the data.

Dependencies:

* Python 3: version 3.5-3.7
* NumPy: version 1.13-1.17

Optional dependencies:
* Matplotlib: for plotting data overview
* pandas: for DataFrame export


Data Import
-----------

To import a RocketLogger data (.rld) file, simply do::

    >>> from rocketlogger.data import RocketLoggerData
    >>> rld = RocketLoggerData('data.rld')

To merge channels with auto-ranging, i.e. the current channels::

    >>> rld.merge_channels()

To get the loaded channel data (by name) and corresponding timestamps::

    >>> d = rld.get_data(['V1', 'I1'])
    >>> t = rld.get_time()

A list of the channels existing in the loaded file is provided by::

    >>> ch = rld.get_channel_names()

To plot a preview of the data (requires Matplotlib)::

    >>> p = rld.plot()

You can also plot the (merged) file data a single command, e.g. for preview::

    >>> p = RocketLoggerData('data.rld').merge_channels().plot()

For analysis using pandas, the use the following shortcut to get a time indexed dataframe (requires pandas)::

    >>> df = rld.get_dataframe()

For more details about the individual functions and thier parameters, refer to the
documentation available at <https://rocketlogger.ethz.ch/python/>.


RocketLogger Device Calibration
-------------------------------

The :RocketLoggerCalibration: class provides the necessary support for generating
device calibration files. See the calibration section in the package documentation
<https://rocketlogger.ethz.ch/python/> and the RocketLogger wiki at
<https://rocketlogger.ethz.ch/wiki/> for more details on the calibration procedure.

