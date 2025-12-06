#!/usr/bin/env bash

# call this script from the top-most Shield directory.

set -e
set -x

#
# This folder is shared b/w host and a VM. Copy the final executable here.
#
SHARED_FOLDER=/c/Users/dilaw/Shared

#
# Use mingw cmake. It support visual studio generator.
#
PATH="/mingw64/bin:$PATH"

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

cd $SCRIPT_DIR/..
ls -ltr

#
# MINIFILTER
#
(
    mkdir -p build
    cd build

    if [ ! -f CMakeCache.txt ]; then
        cmake.exe .. -G "Visual Studio 16 2019"
    fi

    cmake.exe --build . -t dist
    sha256sum dist/BackupOnDeleteShield-*-AMD64.exe
    cp dist/BackupOnDeleteShield*.exe $SHARED_FOLDER
)

ls -ltr $SHARED_FOLDER
