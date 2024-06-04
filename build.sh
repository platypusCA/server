#!/bin/bash
rm -rf build
mkdir build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=/home/$USER/vcpkg/scripts/buildsystems/vcpkg.cmake ..
cmake --build .

