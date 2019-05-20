RocketLogger Data File Support
------------------------------

To import binary RocketLogger Data (.rld) files, use the `RocketLoggerData`
class. Loading a file is as easy as calling the class with the filename::

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

For more details on the individual functions and its optional parameters,
please we refer to the API documentation of the
:class:`rocketlogger.calibration` module.
