RocketLogger Calibration
========================

The :mod:`rocketlogger.calibration` module provides the support for RocketLogger
device calibration. This calibration data processing support is built around the class
:class:`rocketlogger.calibration.RocketLoggerCalibration`.

To generate a calibration file from your calibration measurements use the RocketLogger:

.. code-block:: python

    >>> from rocketlogger.data import RocketLoggerData
    >>> from rocketlogger.calibration import RocketLoggerCalibration, CALIBRATION_SETUP_SMU2450
    >>> filename_prefix = "20200101_"
    >>> data_i1l = RocketLoggerData(f"{filename_prefix}calibration_i1l.rld")
    >>> data_i2l = RocketLoggerData(f"{filename_prefix}calibration_i2l.rld")
    >>> data_ih = RocketLoggerData(f"{filename_prefix}calibration_ih.rld")
    >>> data_v = RocketLoggerData(f"{filename_prefix}calibration_v.rld")
    >>>
    >>> cal = RocketLoggerCalibration()
    >>> cal.load_measurement_data(
    ...     data_v,
    ...     data_i1l,
    ...     data_ih,
    ...     data_i2l,
    ...     data_ih,
    ... )


or using the shortcut with passing the trace filenames:

.. code-block:: python

    >>> filename_prefix = "20200101_"
    >>> cal = RocketLoggerCalibration(
    ...     f"{filename_prefix}calibration_v.rld",
    ...     f"{filename_prefix}calibration_i1l.rld",
    ...     f"{filename_prefix}calibration_ih.rld",
    ...     f"{filename_prefix}calibration_i2l.rld",
    ...     f"{filename_prefix}calibration_ih.rld",
    ... )


to calculate new calibration parameters and store the files use:

.. code-block:: python

    >>> cal.recalibrate(CALIBRATION_SETUP_SMU2450)
    >>> cal.write_calibration_file("calibration.dat")


it is recommended to print the errors within the calibration data to verify
that the provided calibration measurements were properly recorded:

.. code-block:: python

    >>> cal.print_statistics()


the statistics output can also be save as log file (it is recommend to always
keep a log file along with the calibration):

.. code-block:: python

    >>> cal.write_log_file("calibration.log")


An example script that can be used for performing the calibration
calculations is available in ``create_calibration.py`` distributed with the
source package or available in the package's repository.

Further, the ``convert_calibration.py`` script is available to convert legacy
calibration files used for RocketLogger versions `1.x` to the latest format.


For more details on the individual functions and its optional parameters,
please we refer to the API documentation of the
:class:`rocketlogger.calibration.RocketLoggerCalibration` class.
