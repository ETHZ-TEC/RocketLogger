RocketLogger Calibration
------------------------

To generate a calibration file from your calibration measurements use the RocketLogger::

    >>> from rocketlogger.data import RocketLoggerData
    >>> from rocketlogger.calibration import RocketLoggerCalibration, CALIBRATION_SETUP_SMU2450
    >>> filename_prefix = '20200101_'
    >>> data_i1l = RocketLoggerData('{}calibration_i1l.rld'
    ...                             .format(filename_prefix))
    >>> data_i2l = RocketLoggerData('{}calibration_i2l.rld'
    ...                             .format(filename_prefix))
    >>> data_ih = RocketLoggerData('{}calibration_ih.rld'
    ...                            .format(filename_prefix))
    >>> data_v = RocketLoggerData('{}calibration_v.rld'
    ...                           .format(filename_prefix))
    >>>
    >>> cal = RocketLoggerCalibration()
    >>> cal.load_measurement_data(
    ...     data_v,
    ...     data_i1l,
    ...     data_ih,
    ...     data_i2l,
    ...     data_ih)

or using the shortcut with passing the trace filenames::

    >>> filename_prefix = '20200101_'
    >>> cal = RocketLoggerCalibration(
    ...     '{}calibration_v.rld'.format(filename_prefix),
    ...     '{}calibration_i1l.rld'.format(filename_prefix),
    ...     '{}calibration_ih.rld'.format(filename_prefix),
    ...     '{}calibration_i2l.rld'.format(filename_prefix),
    ...     '{}calibration_ih.rld'.format(filename_prefix))

to calculate new calibration parameters and store the files use::

    >>> cal.recalibrate(CALIBRATION_SETUP_SMU2450)
    >>> cal.write_calibration_file('calibration.dat')

it is recommended to print the errors within the calibration data to verify
that the provided calibration measurements were properly recorded::

    >>> cal.print_statistics()

the statistics output can also be save as log file (it is  recommend to always
keep a log file along with the calibration)::

    >>> cal.write_log_file('calibration.log')

For a ready made script that can be used for performing the calibration
calculations is available in `calibrate.py` distributed with the package.

For more details on the individual functions and its optional parameters,
please we refer to the API documentation of the
:class:`rocketlogger.calibration` module.
