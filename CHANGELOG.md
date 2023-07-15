# Changelog


## Version 2.1.1 (2023-07-15)

* [CHANGED] Update to latest Debian *buster* release 10.13 image for security fixes
* [CHANGED] Update web server dependencies for security fixes

_Notes:_

This bugfix release focuses on security updates for web server dependencies and the operating system base image.


## Version 2.1.0 (2023-02-19)

Base operating system:
* [CHANGED] update to Debian *buster* release 10.13 (#78)
* [CHANGED] update to Node.js version 18 LTS (#91)

RocketLogger software:
* [ADDED] System tests for software and data processing pipeline (#73)
* [ADDED] Server side caching of measurement data (#74)
* [ADDED] Activity aware binary channel down-sampling (#92)
* [ADDED] Document measurement status and data stream interface, provide `--stream` CLI alias
* [CHANGED] Improved PWM hardware configuration reliability (#68, #76)
* [CHANGED] Hardware module based inter sample time capturing (#67)
* [CHANGED] Update web interface dependencies to latest version (#71, #83)
* [CHANGED] Optimize and reduce web interface dependencies (#84)
* [CHANGED] Refactor web interface implementation and addition of unit tests (#70, #72)
* [CHANGED] Update development tool configurations (#93, #95)

Python support library:
* [ADDED] Support and test coverage for Python 3.10 and 3.11 (#82, #88)
* [CHANGED] Migrate from nose to pytest (#81)
* [FIX] More robust measurement set-point detection (#94)


_Notes:_

This release updates the base operating and web interface dependencies. New features include server side measurement data caching and "activity aware" down-sampling of digital signal for the web interface preview.
Other notable changes include more robust hardware configuration and initial automated test coverage of the measurement software and web interface components.
Finally, the measurement status and data streaming interfaces are now officially documented for use by third party software.


## Version 2.0.2 (2021-12-09)

RocketLogger software:
* [FIXED] Corrupt RLD files when logging no binary channels (#57)
* [FIXED] Corrupt channel info in split RLD files (#62)

Python support library:
* [ADDED] Standalone script to recover RLD files corrupted by #57
* [FIXED] Missing documentation of file revision changes (#61)
* [CHANGED] Cross validate file headers and tolerate channel info corruption of split files (relates to #62)


_Notes:_

This bugfix release resolves RLD file corruption related issues. Details on how to fully recover affected measurement files are given below.
A timely update is highly recommended to avoid generating corrupt data files.


_Recover Corrupt Data Files:_

Measurement files affected by bugs #57 or #62 can be recovered without data loss:
* To recover files affected by #57 use the `bug57_recover.py` script provided in the Python source package (or available directly from the repository).
* The updated Python library can tolerate channel info header corruption of split files due to bug #62 if all files of the same measurement are imported in one batch.


## Version 2.0.1 (2021-11-29)

RocketLogger software:
* [FIXED] RocketLogger reporting "uncalibrated" after restart (#34)
* [FIXED] Sample aggregation not working, resulting in segmentation fault (#35) by [@rdaforno](https://github.com/rdaforno)
* [FIXED] Sample aggregation "average" produces invalid results (#42) by [@rdaforno](https://github.com/rdaforno)
* [FIXED] Web interface: filename not adjusted when changing file format (#49)
* [FIXED] RocketLogger daemon not terminating on failure (#50)

Python support library:
* [FIXED] Python support calibration conversion script availability documentation (#33)
* [FIXED] Exclude tests from Python package installation (#40)
* [FIXED] Remove debug console output in Python support library (#46)
* [FIXED] Specify exact versions of Python support package dependencies (#48)


_Notes:_

This bugfix release resolves issues related to the sample aggregation for low data rates, the Python support package and the web interface. See the respective bug reports linked in the list above for more details.
Measurements with samples rates <1 kSps, and average aggregation mode (with CLI option `--aggregate=average`)performed with RocketLogger version 2.0.0 and earlier may be corrupted. Users relying on the web interface only and not setting this specific mode as default using the CLI are _not_ affected.


## Version 2.0.0 (2021-08-19)

The changes since version [1.1.6](#version-116-2019-07-31) include all changes of the the beta release [2.0.0-beta1](#version-200-beta1-2021-06-12) and the fixes and improvements listed below.

Base operating system:
* [CHANGED] update to Debian *buster* release 10.10
* [CHANGED] improve robustness and usability of system setup scripts
* [CHANGED] optimize the size of locally patched operating system image
* [FIXED] remote system installation script not working (#25)

RocketLogger software:
* [CHANGED] additional actions available via hardware button gestures
* [FIXED] startup artifacts after RocketLogger system startup or hardware reset (#19)
* [FIXED] segmentation fault in logging output (#29)
* [FIXED] no data plotted in web interface when measuring low current channel only (#26)
* [FIXED] incorrect plot selection shown when reloading web interface (#27)
* [FIXED] invalid configuration in web interface for low sample rates (#31)

Python support library:
* [FIXED] use effectively configured instead of theoretical ADC sample rate for the calculation of relative timestamps _(backward incompatible)_ (#13)
* [FIXED] invalid absolute timestamp calculation for last data block (#30)

Additionally, the documentation in the repository and the accompanying [wiki pages](https://github.com/ETHZ-TEC/RocketLogger/wiki) was updated and significantly extended. These changes address several documentation related issues (#20, #21, #22, #23, #32).


_Notes:_

This major release updates the base operating system to run the latest Debian version and includes numerous internal software and development tool changes. The most noticeable change for the user is the new web control interface that was implemented from scratch and a simplified, more robust command line interface.
The Python support library received new device calibration features and extended data processing functionality.

Due to the major upgrade of the base operating system from Debian version 7 to 10, and the change of the install location to the internal eMMC memory, the RocketLogger system needs to be reinstalled.


_Backward Incompatible Changes:_

* The updated calibration file format requires conversion of existing calibration data when upgrading from previous versions. Follow the [upgrade instructions](https://github.com/ETHZ-TEC/RocketLogger/wiki/upgrade-v2) when updating from RocketLogger version 1.x.
* The new RocketLogger command line interface uses different modes and arguments. The changes are summarized under [CLI update summary](https://github.com/ETHZ-TEC/RocketLogger/wiki/measurement-control#command-line-interface-changes-from-version-1x-to-200).
* The Python Support Library's `get_time()` API has changed. Remove the `absolute_time` argument and use only the updated `time_reference` argument for timestamp reference selection. For detailed API documentation refer to the [Python documentation](https://github.com/ETHZ-TEC/RocketLogger/wiki/python).
* The Python Support Library's `get_time()` implementation for relative timestamp calculation (i.e. using `time_reference="relative"`) has been fixed to use the exact used ADC sample rate instead of the theoretically targeted sample rate. Expect different timestamp values and derived analysis compared to versions 1.x when using `get_time()` with `time_reference="relative"` (corresponding to the default argument value)!
* The MATLAB calibration support has been removed in favor of the new calibration tools included in the Python Support Library.



## Version 2.0.0-beta1 (2021-06-12)

Base operating system:
* [ADDED] local operating system image patching to generate ready to use RocketLogger image (#4)
* [ADDED] switch to Debian *buster* release (#2)
* [CHANGED] install operating system to BeagleBone's embedded eMMC memory, use external SD card for configuration and data storage
* [CHANGED] simplify remote system setup procedure

RocketLogger software:
* [ADDED] use meson build framework for combined build and installation of all software components
* [ADDED] PRU measured ADC sample interval, available as optional `DT` channel (#17)
* [ADDED] switch to ZeroMQ messaging library for data streaming to the webserver
* [ADDED] Node.js based web interface with server side data streaming capability (#3, #5, #9)
* [CHANGED] command line interface update: improved argument consistency and more robust argument parsing _(backward incompatible)_
* [CHANGED] rework low level hardware interfacing for Debian *buster* compatibility (#2)
* [CHANGED] update and reorganize internal software API and headers for increased consistency
* [CHANGED] update to latest and official compiler tools (#18)
* [CHANGED] Binary file format: increment file version to `0x04`
* [REMOVED] legacy web control interface

Python support library:
* [ADDED] extended get data API to access relevant header fields and filenames, add pandas DataFrame generation
* [ADDED] add header-only import, recovery mode for truncated file import
* [ADDED] add calibration support (#10)
* [ADDED] sample data processing and calibration scripts (#7)
* [CHANGED] make plotting an optional feature to reduce package dependencies
* [CHANGED] `get_time()` API: use `time_reference` for timestamp reference selection (using updated parameters!), drop `absolute_time` argument _(backward incompatible)_
* [FIXED] deprecated NumPy API dependency (#12)

MATLAB support:
* [DEPRECATED] The MATLAB support for processing RocketLogger data is deprecated and will be removed in a future release. It is recommended to switch to the more feature-rich Python support library (#11)
* [REMOVED] The MATLAB calibration support has been dropped. Use the Python support library to generate new or convert existing calibration data


_Notes:_

This beta release for the next major version updates the base operating system to run the latest Debian version and includes numerous internal software and development tool changes. The most noticeable change for the user is the new web control interface that was implemented from scratch.
Calibration features were added to the Python support library and the data processing functionality was extended.

Due to the major upgrade of the base operating system from Debian version 7 to 10, and the change of the install location to the internal eMMC memory, the RocketLogger system needs to be reinstalled.


_Known Issues:_

* Unresponsive web interface when system partition is running out of space (#23)
* Startup artifacts after RocketLogger system startup or hardware reset (#19)
* Python library uses target instead of theoretical configured sample rate for relative timestamp calculation (#13)



## Version 1.1.6 (2019-07-31)

* [FIXED] Software: potential corruption of ambient file name (#127)
* [FIXED] Software: typo in ambient file channel name (#128)
* [FIXED] Software: wrong scale for default file split value (#124)
* [CHANGED] Python support: document successfully tested compatibility with recent Python and NumPy releases

_Notes:_

This hotfix release addresses two issues related to storing ambient sensor data and an invalid default value in the measurement file split configuration. The Python support package was tested to successfully work with recent Python and NumPy updates.


## Version 1.1.5 (2019-03-04)

* [FIXED] Software: corrupt data when writing files >2 GB (#119)
* [FIXED] Software/web interface: data aggregation issue for larger time scales (#117)
* [FIXED] Documentation: more detailed description of common measurement setups and user interfaces (#114, #120)
* [CHANGED] Software: split data files at 1 GB by default (relates to #119)

_Notes:_

This hotfix release resolves a file writing issue when saving the measurements to a single large data file and a data aggregation issue in the web interface preview. Furthermore, the documentation in the wiki on measurement setup and control has been updated and extended.


## Version 1.1.4 (2018-06-26)

* [FIXED] Software: corrupt data when using high sampling rates and values are close to full range (#116)
* [FIXED] Python support: single data block file import issue (#115).

_Notes:_

This hotfix release resolves a data wrap-around issue for sampling rates of 32 kSps and 64 kSps and a Python support bug when importing files with a single data block.


## Version 1.1.3 (2018-04-26)

* [FIXED] Software: corrupt low current valid channel when digital channels are disabled (#108)
* [FIXED] Software: invalid lux calculation for mid/high range of TSL4531 (#111)
* [FIXED] Software/web interface: add more detailed version information (#112)
* [FIXED] Software: invalid data block timestamps in sensor file (#113) by [@masoudinejad](https://github.com/masoudinejad)

_Notes:_

This hotfix release resolves an issue with the low current valid channel when disabling digital channels and two bugs related to logging external sensors.
The software now includes the git revision and build date in its version information output.


## Version 1.1.2 (2018-02-26)

* [FIXED] Software: invalid comment length calculation in binary file header (#107)

_Notes:_

This hotfix release fixes a comment length calculation issue that generated invalid files headers for some specific file comment lengths.


## Version 1.1.1 (2018-01-25)

* [FIXED] Python support: overflow in channel merge functionality (#106) by [@AlexRaimondi](https://github.com/AlexRaimondi)
* [FIXED] Python support: specify tested Python dependencies and automated testing for different environments

_Notes:_

This hotfix release fixes a data overflow problem in the Python support library's channel merge functionality that occurs with v1.1 on some platforms.


## Version 1.1 (2017-12-19)

* [FIXED] Hardware: ground loop in cape supply (#97)
* [FIXED] Software/web interface: filename length check (#96)
* [CHANGED] Binary file format: zero based valid link indexing
* [CHANGED] Binary file format: increment file version to `0x03`
* [ADDED] Ambient sensor logging integrated into the main application
* [ADDED] Python support for RocketLogger data file processing
* [ADDED] Command line option to set file comment field
* [ADDED] Command line option to configure mode of data aggregation when using sampling rates lower than native 1 kSPS
* [ADDED] Binary file format: define additional units for storing ambient sensor data
* [ADDED] Binary file format: distinguish between undefined and unit-less data units

_Notes:_

While previous file versions used (undocumented) one-based channel indexing (e.g. for valid links), this was changed to zero-based indexing for consistency reasons and added to the documentation. To reflect this backward incompatible change and format extensions listed above, the file version was incremented to `0x03`. The Python and Matlab analysis scripts were updated to support these changes, while guaranteeing full backward compatibility with files using the older data format.


## Version 1.0 (2017-03-15)

* [ADDED] First publicly available version
