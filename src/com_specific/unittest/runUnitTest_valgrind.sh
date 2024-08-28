#!/bin/csh

# This script runs all the unit test cases
# Also exports the needed environmental variable for "Unittest"
#     Note: "Unittest" expected to fail if "LD_LIBRARY_PATH" not set properly.

# Create valgrind tmp folder
if ( -d valgrind_logs ) then
    rm -rf valgrind_logs
endif
mkdir valgrind_logs

setenv LD_LIBRARY_PATH $LSB_SHAREDLIBPATH

hostname | grep esekilxxen171 > /dev/null
if ( $? == 0 ) then
    # This is run in Kista IT hub, try to load latest possible valgrind
    eval `/app/modules/0/bin/modulecmd tcsh add valgrind/3.10.1-1`
endif

setenv valgrind `which valgrind`
if ( $? == 0 ) then
    # for running on IT hub: valgrind is old
    setenv check_option `${valgrind} --help | grep -e "--show-possibly-lost"`
    if ( $? == 0 ) then
        setenv valgrind "${valgrind} --show-possibly-lost=yes"
    endif
    ${valgrind} --leak-check=full --show-reachable=yes --malloc-fill=25 --free-fill=27 --suppressions=unittest.supp --log-file=valgrind_logs/valUnittest.log ./Unittest
    ${valgrind} --leak-check=full --show-reachable=yes --malloc-fill=25 --free-fill=27 --suppressions=unittest.supp --log-file=valgrind_logs/valUnittestNTF_RList.log ./UnittestNTF_RList
    ${valgrind} --leak-check=full --show-reachable=yes --malloc-fill=25 --free-fill=27 --suppressions=unittest.supp --log-file=valgrind_logs/valUnittestCmCache.log ./UnittestCmCache
    ${valgrind} --leak-check=full --show-reachable=yes --malloc-fill=25 --free-fill=27 --suppressions=unittest.supp --log-file=valgrind_logs/valUnittestOiProxy.log ./UnittestOiProxy
    ${valgrind} --leak-check=full --show-reachable=yes --malloc-fill=25 --free-fill=27 --suppressions=unittest.supp --log-file=valgrind_logs/valUnittestImmCmd.log ./UnittestImmCmd
    ${valgrind} --leak-check=full --show-reachable=yes --malloc-fill=25 --free-fill=27 --suppressions=unittest.supp --log-file=valgrind_logs/valUnittestReencryptor.log ./UnittestReencryptor

else
    ./Unittest
    ./UnittestNTF_RList
    ./UnittestCmCache
    ./UnittestOiProxy
    ./UnittestImmCmd
endif
