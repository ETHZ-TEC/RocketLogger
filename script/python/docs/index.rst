RocketLogger Python Support
===========================

This package provides RocketLogger data file handling support, basic data
processing and plotting of the data.


To import a RocketLogger Data (.rld) file, simply do::

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

For more details about the indicidual functions, please refer to the API
documentation.



Contents
--------

.. toctree::
    :maxdepth: 2

    modules


Features
--------

Python support for RocketLogger Data (\*.rld) files

- Import
- Channel merging
- Data extraction
- Plotting of file data


Installation
------------

Install the RocketLogger Python Support from the PiPy using:

    `pip install rocketlogger`


Contribute
----------

- Issue Tracker: https://git.ee.ethz.ch/sigristl/rocketlogger/issues/
- Source Code: https://git.ee.ethz.ch/sigristl/rocketlogger/


License
-------

The project is licensed under the BSD license. See LICESNSE.txt for more details.
