
## v1.1.6 (2019-07-31)

- [FIXED] Software: potential corruption of ambient file name (#127)
- [FIXED] Software: typo in ambient file channel name (#128)
- [FIXED] Software: wrong scale for default file split value (#124)
- [CHANGED] Python support: document successfully tested compatibility with recent Python and NumPy releases

_Notes:_

This hotfix release addresses two issues related to storing ambient sensor data and an invalid default value in the measurement file split configuration. The Python support package was tested to successfully work with recent Python and NumPy updates.


## v1.1.5 (2019-03-04)

- [FIXED] Software: corrupt data when writing files >2 GB (#119)
- [FIXED] Software/web interface: data aggregation issue for larger time scale (#117).
- [FIXED] Documentation: more detailed description of common measurement setups and user interfaces (#114, #120).
- [CHANGED] Software: split data files at 1 GB by default (relates to #119)

_Notes:_

This hotfix release resolves a file writing issue when saving the measurements to single large data file and a data aggregation issue in the web interface preview. Furthermore, the documentation in the wiki on measurement setup and control has been updated and extended.


## v1.1.4 (2018-06-26)

- [FIXED] Software: corrupt data when using high sampling rates and values are close to full range (#116)
- [FIXED] Python support: single data block file import issue (#115).

_Notes:_

This hotfix release resolves a data wrap-around issue for sampling rates of 32 kSps and 64 kSps and a python support bug when importing files with a single data block.


## v1.1.3 (2018-04-26)

- [FIXED] Software: corrupt low current valid channel when digital channels are disabled (#108)
- [FIXED] Software: invalid lux calculation for mid/high range of TSL4531 (#111)
- [FIXED] Software/web interface: add more detailed version information (#112).
- [FIXED] Software: invalid data block timestamps in sensor file (#113). Credits: Mojtaba Masoudinejad

_Notes:_

This hotfix release resolves an issue with the low current valid channel when disabling digital channels and two bugs related to logging external sensors.
The software now includes the git revision and build date in its version information output.


## v1.1.2 (2018-02-26)

- [FIXED] Software: invalid comment length calculation in binary file header (#107)

_Notes:_

This hotfix release fixes a comment length calculation issue that generated invalid files headers for some specific file comment lengths.


## v1.1.1 (2018-01-25)

- [FIXED] Python support: overflow in channel merge functionality (#106). Credits: Alex Raimondi
- [FIXED] Python support: specify tested python dependencies and automated testing for different environments

_Notes:_

This hotfix release fixes a data overflow problem in the python support library's channel merge functionality that occurs with v1.1 on some platforms.


## v1.1 (2017-12-19)

- [FIXED] Hardware: ground loop in cape supply (#97)
- [FIXED] Software/web interface: filename length check (#96)
- [CHANGED] Binary file format: zero based valid link indexing
- [CHANGED] Binary file format: increment file version to 0x03
- [ADDED] Ambient sensor logging integrated into the main application
- [ADDED] Python support for RocketLogger data file processing
- [ADDED] Command line option to set file comment field
- [ADDED] Command line option to configure mode of data aggregation when using sampling rates lower than native 1 kSPS
- [ADDED] Binary file format: define additional units for storing ambient sensor data
- [ADDED] Binary file format: distinguish between undefined and unit-less data units

_Notes:_

While previous file versions used (undocumented) one-based channel indexing (e.g. for valid links), this was changed to zero-based indexing for consistency reasons and added to the documentation. To reflect this backward incompatible change and format extensions listed above, the file version was incremented to `0x03`. The Python and Matlab analysis scripts were updated to support these changes, while guaranteeing full backward compatibility with files using the older data format.


## v1.0 (2017-03-15)

- [ADDED] First publicly available version
