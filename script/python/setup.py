""""
RocketLogger Python Library.

Copyright (c) 2016-2017, ETH Zurich, Computer Engineering Group

"""

from setuptools import setup, find_packages


setup(name='rocketlogger',
      version='1.1',
      description='RocketLogger Python Support',
      url='https://rocketlogger.ethz.ch/',
      author='Computer Engineering Group, ETH Zurich',
      author_email='lukas.sigrist@tik.ee.ethz.ch',
      license='BSD 3-Clause',
      classifiers=[
          'Development Status :: 5 - Production/Stable',
          'Intended Audience :: Science/Research',
          'Topic :: Scientific/Engineering :: Information Analysis',
          'License :: OSI Approved :: BSD License',
          'Programming Language :: Python :: 3 :: Only',
          'Programming Language :: Python :: 3',
          'Programming Language :: Python :: 3.2',
          'Programming Language :: Python :: 3.3',
          'Programming Language :: Python :: 3.4',
          'Programming Language :: Python :: 3.5',
      ],
      keywords='rocketlogger data analysis',
      packages=find_packages(exclude=['contrib', 'docs', 'tests']),
      python_requires='>=3.2, <4',
      install_requires=[
          'matplotlib',
          'numpy',
      ],
      test_suite='nose.collector',
      tests_require=[
          'nose'
      ],
      include_package_data=True,
      zip_safe=False)
