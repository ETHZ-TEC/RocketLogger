RocketLogger Python Support Library
===================================

This package provides RocketLogger data file handling support, basic data
processing and plotting of the data.


Data Import
-----------

To import a RocketLogger data (.rld) file, simply do::

    >>> from rocketlogger.data import RocketLoggerData
    >>> rld = RocketLoggerData('data.rld')

To merge channels with auto-ranging, i.e. the current channels::

    >>> rld.merge_channels()

To get the loaded channel data (by name) and corresponding timestamps::

    >>> rld.get_data(['V1', 'I1'])
    >>> rld.get_time()

A list of the channels existing in the loaded file is provided by:

    >>> rld.get_channel_names()

To plot a preview of the data::

    >>> rld.plot()

You can also plot the (merged) file data a single command, e.g. for preview::

    >>> RocketLoggerData('data.rld').merge_channels().plot()

For more details about the indicidual functions, please refer to the
documentation.

