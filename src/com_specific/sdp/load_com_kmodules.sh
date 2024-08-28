#!/bin/sh

SELF_NAME=`basename "$0"`
logger -t "${SELF_NAME}" "INFO: Loading COM FUSE kernel module"
modprobe fuse
modprobe_exit_code=$?
if [ $modprobe_exit_code -ne 0 ]; then
    logger -t "${SELF_NAME}""Error: Could not load COM FUSE Kernel Module due to $modprobe_exit_code"
    exit $modprobe_exit_code
fi

logger -t "${SELF_NAME}" "INFO: DONE! Loading COM FUSE kernel module."
exit 0
