#!/usr/bin/env python3
"""
Recover RLD measurement files suffering from bug #57.

Usage: ./bug57_recovery.sh <filename>
* <filename> the filename of the RLD file to recover

This script attempts to patch the file header of RLD files that do not
contain binary data channels, by adding a dummy "BUG57_PATCH" binary channel.
Adding this dummy channel makes files suffering from bug #57 again compliant
with the RLD file specification.
The script does not modify the original data, but stores the patched data in
a new data file with the input filename suffixed with "_patched".

See also: <https://github.com/ETHZ-TEC/RocketLogger/issues/57>
"""

import os
import re
import sys

_RLD_FILE_REGEX = re.compile(
    r"(?P<basename>.*?)(?P<part>_p\d+)?(?P<ext>\.rld)", re.IGNORECASE
)

# file header length offset and size in bytes
_HEADER_LENGTH_OFFSET = 0x06
_HEADER_LENGTH_BYTES = 2

# file comment length offset and size in bytes
_COMMENT_LENGTH_OFFSET = 0x30
_COMMENT_LENGTH_BYTES = 4

# file binary channel count offset and size in bytes
_BINARY_CHANNEL_COUNT_OFFSET = 0x34
_BINARY_CHANNEL_COUNT_BYTES = 2

# file analog channel count offset and size in bytes
_ANALOG_CHANNEL_COUNT_OFFSET = 0x36
_ANALOG_CHANNEL_COUNT_BYTES = 2

# Data Valid (binary) channel info with name "BUG57_PATCH"
_DUMMY_BINARY_CHANNEL_DATA = bytes([0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF,
    0x42, 0x55, 0x47, 0x35, 0x37, 0x5F, 0x50, 0x41, 0x54, 0x43, 0x48, 0x00, 0x00, 0x00, 0x00, 0x00])


class FilePatchingError(Exception):
    """File patching related errors."""

    pass


if __name__ == "__main__":

    # get filename argument
    if len(sys.argv) != 2:
        raise TypeError("the script takes exactly one filename argument")
    input_file = str(sys.argv[1])

    # output file name and check filename extension
    basename, ext = os.path.splitext(input_file)
    match = _RLD_FILE_REGEX.match(input_file)

    if not match.group("ext"):
        raise ValueError("the script only processes RLD files")

    if match.group("part"):
        output_file = (
            match.group("basename")
            + "_patched"
            + match.group("part")
            + match.group("ext")
        )
    else:
        output_file = match.group("basename") + "_patched" + match.group("ext")

    try:
        with open(input_file, "r+b") as file_in, open(output_file, "x+b") as file_out:
            print(f"Attempt to create patched file: {output_file}")

            # copy start of original header
            file_out.write(file_in.read(_HEADER_LENGTH_OFFSET))

            # update header length
            header_length = int.from_bytes(
                file_in.read(_HEADER_LENGTH_BYTES), byteorder="little", signed=False
            )
            patched_header_length = header_length + len(_DUMMY_BINARY_CHANNEL_DATA)
            file_out.write(
                int.to_bytes(
                    patched_header_length,
                    length=_HEADER_LENGTH_BYTES,
                    byteorder="little",
                    signed=False,
                )
            )

            # copy header up to comment length
            file_out.write(
                file_in.read(
                    _COMMENT_LENGTH_OFFSET
                    - _HEADER_LENGTH_BYTES
                    - _HEADER_LENGTH_OFFSET
                )
            )

            # get and copy comment length
            comment_length_bytes = file_in.read(_COMMENT_LENGTH_BYTES)
            file_out.write(comment_length_bytes)
            comment_length = int.from_bytes(
                comment_length_bytes, byteorder="little", signed=False
            )

            # update binary channel count
            binary_channel_count = int.from_bytes(
                file_in.read(_BINARY_CHANNEL_COUNT_BYTES),
                byteorder="little",
                signed=False,
            )
            if binary_channel_count != 0:
                raise FilePatchingError(
                    "Provided file contains binary therefore cannot (and never should) be patched"
                )

            binary_channel_count += 1
            file_out.write(
                int.to_bytes(
                    binary_channel_count,
                    length=_HEADER_LENGTH_BYTES,
                    byteorder="little",
                    signed=False,
                )
            )

            # copy remaining analog channel count and comment
            file_out.write(file_in.read(_ANALOG_CHANNEL_COUNT_BYTES + comment_length))

            # append dummy binary channel
            file_out.write(_DUMMY_BINARY_CHANNEL_DATA)

            # copy remaining file content
            file_out.write(file_in.read())
    except FileExistsError:
        print(f"A patched file `{output_file}` already exists! Skip patching...")
        sys.exit(1)
    except FilePatchingError as e:
        print(f"Failed to patch file: {e}")
        print(f"Removing incomplete file: {output_file}")
        os.remove(output_file)
        sys.exit(1)

    print(f"Patched file was successfully written to: {output_file}")
