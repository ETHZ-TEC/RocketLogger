
## v1.1 (2017-00-00)

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
