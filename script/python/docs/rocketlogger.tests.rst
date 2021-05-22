Package Tests
=============

The :doc:`rocketlogger.tests` module provides the package tests for the
:mod:`rocketlogger.data` and :mod:`rocketlogger.calibration` modules.

The tests are to be executed using the ``nosetests`` and ``coverage`` utilities,
e.g. using the ``test_coverage.sh`` bash script distributed with the package.
Further, the package includes a configuration for ``tox`` to test against all supported Python installations,
and a helper script ``test_performance.sh`` to test the performance of various file import options on your target machine


Data Tests
----------

.. automodule:: rocketlogger.tests.test_data
    :members:
    :undoc-members:
    :show-inheritance:


Calibration Tests
-----------------

.. automodule:: rocketlogger.tests.test_calibration
    :members:
    :undoc-members:
    :show-inheritance:
