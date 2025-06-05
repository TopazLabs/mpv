#!/bin/bash

 conan install ./build-scripts/conanfile.py -u -pr:b ./build-scripts/profile_mac_armv8 -pr:h ./build-scripts/profile_mac14.0 -of ./conan