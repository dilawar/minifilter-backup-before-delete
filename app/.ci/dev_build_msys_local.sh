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
echo "Script directory ${SCRIPT_DIR}"

cd $SCRIPT_DIR/..
ls -ltr

mkdir -p build
cd build
if [ ! -f CMakeCache.txt ]; then
    cmake.exe .. -G "Visual Studio 16 2019"
fi
cmake.exe --build .

sha256sum BackupOnDeleteBackup.exe

cp BackupOnDeleteBackup.exe $SHARED_FOLDER

#
# copy the test executable as well.
#
cp tests/_test_live.exe $SHARED_FOLDER

find $SHARED_FOLDER -type f -print0 | xargs -0 -I file sha256sum file
