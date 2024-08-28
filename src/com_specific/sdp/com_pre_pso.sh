#!/bin/sh
##
## Copyright (c) 2018 Ericsson AB.
##
## All Rights Reserved. Reproduction in whole or in part is prohibited
## without the written consent of the copyright owner.
##
## ERICSSON MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE
## SUITABILITY OF THE SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING
## BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY,
## FITNESS FOR A PARTICULAR PURPOSE, OR NON-INFRINGEMENT. ERICSSON
## SHALL NOT BE LIABLE FOR ANY DAMAGES SUFFERED BY LICENSEE AS A
## RESULT OF USING, MODIFYING OR DISTRIBUTING THIS SOFTWARE OR ITS
## DERIVATIVES.
##
##

# This script is a participant to the coremw which is triggered from a unit file to remove com and comsa
# pso config directories for LDA restore and upgrade.

# The script is designed to work in a limited linux environment where the support of certain utilities
# like for instance ps and pgrep varies.
# This is a bourne shell script and is free from any non compatible syntax (e.g bash specific features)

SELF_NAME="$(basename "$0")"

SUCCESS=0
ERROR=1

# Enabling debug logs to syslog 0=disabled , 1=enabled (error logs will always be written to syslog).
DO_SYSLOG=0

# This function logs info messages to syslog.
syslog_info_message() {
    logger -p local0.info -t "$SELF_NAME[$$]" "$*"
}

# This function logs debug messages to syslog if DO_SYSLOG flag is enabled.
syslog_debug_message() {
   if [ $DO_SYSLOG -eq 1 ]; then
      logger -p local0.debug -t "$SELF_NAME[$$]" "$*"
   fi
}

# This function logs syslog_error_message messages to syslog and stderr.
syslog_error_message() {
   echo "ERROR $*" >&2
   logger -p local0.err -t "$SELF_NAME[$$]" "$*"
   exit $ERROR
}

# COM and COMSA PSO directories
COM_PSO_DIR="com-apr9010443"
COMSA_PSO_DIR="comsa_for_coremw-apr9010555"

# PSO API
PERSISTENT_STORAGE_API_DIR="/usr/share/pso/storage-paths"

if [ -d "${PERSISTENT_STORAGE_API_DIR}" ]; then
    PERSISTENT_STORAGE_API_CONFIG="${PERSISTENT_STORAGE_API_DIR}/config"
    PERSISTENT_STORAGE_API_SOFTWARE="${PERSISTENT_STORAGE_API_DIR}/software"
    PERSISTENT_STORAGE_API_CLEAR="${PERSISTENT_STORAGE_API_DIR}/clear"
    PERSISTENT_STORAGE_API_NO_BACKUP="${PERSISTENT_STORAGE_API_DIR}/no-backup"
    PERSISTENT_STORAGE_API_USER="${PERSISTENT_STORAGE_API_DIR}/user"
else
    syslog_error_message "PSO API unavailable"
    exit $ERROR
fi

if [ -f ${PERSISTENT_STORAGE_API_CONFIG} ]; then
    PSO_CONFIG=$(cat ${PERSISTENT_STORAGE_API_CONFIG})
else
    syslog_error_message "Config PSO API unavailable"
    exit $ERROR
fi

if [ -f ${PERSISTENT_STORAGE_API_SOFTWARE} ]; then
    PSO_SOFTWARE=$(cat ${PERSISTENT_STORAGE_API_SOFTWARE})
else
    syslog_error_message "Software PSO API unavailable"
    exit $ERROR
fi

if [ -f ${PERSISTENT_STORAGE_API_CLEAR} ]; then
    PSO_CLEAR=$(cat ${PERSISTENT_STORAGE_API_CLEAR})
else
    syslog_error_message "Clear PSO API unavailable"
    exit $ERROR
fi

if [ -f ${PERSISTENT_STORAGE_API_NO_BACKUP} ]; then
    PSO_NO_BACKUP=$(cat ${PERSISTENT_STORAGE_API_NO_BACKUP})
else
    syslog_error_message "No-Backup PSO API unavailable"
    exit $ERROR
fi

if [ -f ${PERSISTENT_STORAGE_API_USER} ]; then
    PSO_USER=$(cat ${PERSISTENT_STORAGE_API_USER})
else
    syslog_error_message "User PSO API unavailable"
    exit $ERROR
fi

check_and_remove() {
   if [ ! -z "$1" ]; then
      #shellcheck disable=SC2115
      rm -rf "${1}/${COM_PSO_DIR}" "${1}/${COMSA_PSO_DIR}" > /dev/null 2>&1
   fi
}

cleanup() {
    syslog_debug_message "Enter cleanup()"

    # Clean PSO CONFIG dir
    check_and_remove "${PSO_CONFIG}"

    # Clean PSO SOFTWARE dir
    check_and_remove "${PSO_SOFTWARE}"

    # Clean PSO CLEAR dir
    check_and_remove "${PSO_CLEAR}"

    # Clean PSO NO-BACKUP dir
    check_and_remove "${PSO_NO_BACKUP}"

    # Clean PSO USER dir
    check_and_remove "${PSO_USER}"

    syslog_debug_message "Exit cleanup()"
}

# Clean COM PSO area
syslog_info_message "com_pre_pso initiated"
cleanup

exit $SUCCESS
