#!/bin/bash

# This script runs all the unit test cases
# Also exports the needed environmental variable for "Unittest"
#     Note: "Unittest" expected to fail if "LD_LIBRARY_PATH" not set properly.

export LD_LIBRARY_PATH=$LSB_SHAREDLIBPATH
./Unittest
./UnittestNTF_RList
./UnittestCmCache
./UnittestOiProxy
./UnittestImmCmd
./UnittestReencryptor
