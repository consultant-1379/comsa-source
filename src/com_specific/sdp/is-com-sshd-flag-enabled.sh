#!/bin/sh
##
## This script will return 0 if the 'sshdManagement' flag is 'true' in 'libcom_sshd_manager.cfg'
## if this flag is false or the file does not exist the script will return '1'
##
# Copyright (C) 2015 by Ericsson AB
# S - 125 26  STOCKHOLM
# SWEDEN, tel int + 46 10 719 0000
#
# The copyright to the computer program herein is the property of
# Ericsson AB. The program may be used and/or copied only with the
# written permission from Ericsson AB, or in accordance with the terms
# and conditions stipulated in the agreement/contract under which the
# program has been supplied.
#
# All rights reserved.


## PSO API
PERSISTENT_STORAGE_API="/usr/share/pso/storage-paths"
PERSISTENT_STORAGE_API_CONFIG="${PERSISTENT_STORAGE_API}/config"

CONFIG_REPO=`cat $PERSISTENT_STORAGE_API_CONFIG`

# Check sshd debug
SSHD_DEBUG=0

#define debuglogger
if [ $SSHD_DEBUG -eq 1 ] ; then
        debuglogger=/bin/logger
else
        debuglogger=/bin/true

fi

COM_SSHD_FLAG=false
$debuglogger "is-com-sshd-flag-enabled.sh called"

COM_DIR=/com-apr9010443/lib/comp/
COM_SSHD_CFG=libcom_sshd_manager.cfg
COM_SSHD_TAG=sshdManagement
COM_SSHD_CFG_FILE=${CONFIG_REPO}${COM_DIR}${COM_SSHD_CFG}
if [ ! -f "${COM_SSHD_CFG_FILE}" ]; then
	COM_SSHD_FLAG=false
	$debuglogger "is-com-sshd-flag-enabled.sh ${COM_SSHD_TAG} is UNKNOWN"
	exit 1
else
	COM_SSHD_FLAG=`cat ${COM_SSHD_CFG_FILE} | grep ${COM_SSHD_TAG} | sed 's/</ /g;s/>/ /g' | awk '{print $2}'`
fi

case "${COM_SSHD_FLAG}" in
	true)
		$debuglogger "is-com-sshd-flag-enabled.sh ${COM_SSHD_TAG} is true."
		exit 0
		;;
	false)
		$debuglogger "is-com-sshd-flag-enabled.sh ${COM_SSHD_TAG} is false."
		exit 1
		;;
	*)
		logger "is-com-sshd-flag-enabled.sh ERROR: ${COM_SSHD_TAG} is '${COM_SSHD_FLAG}'. Assuming false."
		exit 1
		;;
esac
