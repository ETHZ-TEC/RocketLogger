#!/bin/sh
# Run tests and coverage analysis.
#
# Copyright (c) 2016-2017, ETH Zurich, Computer Engineering Group
#

nosetests \
  --verbosity=2 \
  --with-coverage \
  --cover-package=rocketlogger \
  --cover-inclusive \
  --cover-branches \
  --cover-html \
  --cover-html-dir="docs/htmlcov"
