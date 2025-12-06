#!/usr/bin/env bash

set -xeu

CONFIG=Debug

echo "Building minifilter"
cd minifilter
mkdir -p _build
cd _build
cmake ..
cmake --build . --config $CONFIG

echo "Building app"
cd ../..
cd app
mkdir -p _build
cd _build
cmake ..
cmake --build . --config $CONFIG
