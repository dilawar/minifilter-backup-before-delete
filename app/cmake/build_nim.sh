#!/usr/bin/env bash

set -x
set -e

#
# A wrapper to build nim files.
#
export CC=/mingw64/bin/gcc.exe
export PATH=/mingw64/bin:$PATH

nim.exe c --cc:env -d:mingw $@
