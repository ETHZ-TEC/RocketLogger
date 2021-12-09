Welcome to the RocketLogger Python Support documentation!
=========================================================

.. toctree::
    :maxdepth: 2
    :titlesonly:
    :glob:
    :hidden:

    data
    calibration
    rocketlogger


The ``rocketlogger`` package provides support for handling RocketLogger data files and for
generation of RocketLogger device calibration file from measurements.


Features
--------

Python support for RocketLogger Data (``*.rld``) files by the :mod:`rocketlogger.data` module.
Supports any officially specified RocketLogger data file version (versions 2-4).

- Import
- Channel merging
- Data extraction
- Plotting of file data

Support for RocketLogger Device Calibration by the :mod:`rocketlogger.calibration` module.

- Generate new calibrations from measurements
- Read/write calibration files
- Validate the accuracy of new and existing calibrations


Installation
------------

Install the RocketLogger Python Support from the `Python Package Index <https://pypi.org/project/rocketlogger/>`_ using:

    ``pip install rocketlogger``


Testing
-------

The source installation comes with automated test cases and the relevant sample data.
For more details see the test documentation of :doc:`tests`.


Contribute
----------

- Source code:   https://github.com/ETHZ-TEC/RocketLogger
- Issue tracker: https://github.com/ETHZ-TEC/RocketLogger/issues
- Documentation: https://github.com/ETHZ-TEC/RocketLogger/wiki/python


License
-------

The project is licensed under the 3-Clause BSD License. See the package's ``LICENSE`` file for more details.


Indices and Tables
------------------

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`
