Data Processing
===============

The :mod:`rocketlogger.data` module provides Python data processing support
for RocketLogger data files (\*.rld). This support is built around the class
:class:`rocketlogger.data.RocketLoggerData`.

To load a binary RocketLogger data file, create a new instance of these class
with passing the file name as argument:

.. code-block:: python

    >>> from rocketlogger.data import RocketLoggerData
    >>> rld = RocketLoggerData("data.rld")


To merge channels with auto-ranging, i.e. the current channels:

.. code-block:: python

    >>> rld.merge_channels()


To get the loaded channel data (by name) and corresponding timestamps:

.. code-block:: python

    >>> rld.get_data(["V1", "I1"])
    >>> rld.get_time()


A list of the channels existing in the loaded file is provided by:

.. code-block:: python

    >>> rld.get_channel_names()


To plot a preview of the data:

.. code-block:: python

    >>> rld.plot()


You can also plot the (merged) file data a single command, e.g. for preview:

.. code-block:: python

    >>> RocketLoggerData("data.rld").merge_channels().plot()


An example illustrating post processing of measurement data is provided in
``process_data.py`` that is distributed with the source package or
available in the package's repository.

For more details on the individual functions and its optional parameters,
please we refer to the API documentation of the
:mod:`rocketlogger.calibration` module.
