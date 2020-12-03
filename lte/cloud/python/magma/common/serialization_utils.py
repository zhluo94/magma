"""
Copyright (c) 2016-present, Facebook, Inc.
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree. An additional grant
of patent rights can be found in the PATENTS file in the same directory.
"""

import codecs
import os


def write_to_file_atomically(filename, value, temp_filename=None):
    """
    Atomically write to a file by first writing the value to a temp file, then
    moving that temp file to the specified file location.

    This function will create all directories necessary for the file as well.

    Args:
        filename: full path to the file to write to
        value: value to write to the file
        temp_filename: requested path of the intermediate temp file
        mode: mode to open the file
    """
    os.makedirs(os.path.dirname(filename), exist_ok=True)
    temp_filename = temp_filename or '{}.tmp'.format(filename)
    with codecs.open(temp_filename, 'w', encoding='utf8') as f:
        f.write(value)
        f.flush()
        os.fsync(f.fileno())
    os.replace(temp_filename, filename)
