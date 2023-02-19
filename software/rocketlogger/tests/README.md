# RocketLogger System Tests

These system test are meant to be run on a RocketLogger to check for successful
sampling and post processing of sampled data using the Python support library.


## System Dependencies

The following system dependencies are required for generating a Python
environment and importing required packages.
```
python3
python3-venv
libatlas-base-dev
```
Instead of Python's `venv` one may prefer `virtualenvwrapper` for creating
virtual Python environments.


## Test Environment end Execution

To setup the environment on the RocketLogger for test execution use the
following commands to create and load a Python virtual environment and install
required package dependencies:
```
python -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

The tests itself are executed using pytest, i.e. using the following command
in the `tests/` or its parent directory:
```
pytest
``` 
