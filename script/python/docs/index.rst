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

Python support for RocketLogger Data (\*.rld) files by the 
:mod:`rocketlogger.data` module.

- Import
- Channel merging
- Data extraction
- Plotting of file data

Support for RocketLogger Device Calibration by the
:mod:`rocketlogger.calibration` module.

- Generate new calibrations from measurements
- Read/write calibration files
- Validate the accuracy of new and existing calibrations


Installation
------------

Install the RocketLogger Python Support from the PyPI using:

    ``pip install rocketlogger``


Contribute
----------

- Source Code:   https://gitlab.ethz.ch/tec/public/rocketlogger/
- Issue Tracker: https://gitlab.ethz.ch/tec/public/rocketlogger/issues/
- documentation: https://rocketlogger.ethz.ch/python/


License
-------

The project is licensed under the BSD license. See the packages' ``LICESNSE`` file for more details.


Indices and Tables
------------------

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`
