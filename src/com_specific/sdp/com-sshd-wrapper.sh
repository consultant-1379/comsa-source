#!/bin/sh
##
## This script is the gateway to let AMF control COM SSHD.
## $ com-sshd-wrapper.sh <start|stop|restart|status>
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

# Check sshd debug

# logger command will be invoked with '-t' option to avoid slow execution in case LDAP
# server is not reachable. Using "logger" as tag to maintain backward compatibility
TAG="logger"

SSHD_DEBUG=0

#define debuglogger
if [ $SSHD_DEBUG -eq 1 ] ; then
	debuglogger="/bin/logger -t $TAG"
else
	debuglogger=/bin/true

fi

$debuglogger "com-sshd-wrapper.sh called with $1"

# check if the COM SA file for SSHD control exists
COMSA_AMF_SSHD=AMF_handles_SSHD
COMSA_HDL_SSHD=ComSA_handles_SSHD
COMSA_AMF_SSHD_FILE=/opt/com/run/${COMSA_AMF_SSHD}
COMSA_HDL_SSHD_FILE=/opt/com/run/${COMSA_HDL_SSHD}

if [ ! -f "${COMSA_AMF_SSHD_FILE}" ]; then
	$debuglogger "com-sshd-wrapper.sh ${COMSA_AMF_SSHD_FILE} is not present."
	case "${1}" in
		start | restart)
			$debuglogger "com-sshd-wrapper.sh(re/start): COM SA is updating and starting the main SSHD."
			update_sshd
			reload_sshd
			if [ -f "${COMSA_HDL_SSHD_FILE}" ]; then
				rm -f ${COMSA_HDL_SSHD_FILE}
			fi
			sudo -u com-core touch ${COMSA_HDL_SSHD_FILE}
			exit 0
			;;
		stop)
			if [ -f "${COMSA_HDL_SSHD_FILE}" ]; then
				$debuglogger "com-sshd-wrapper.sh(stop): Restoring and restarting the main SSHD, was modified by COM SA"
				update_sshd -d
				reload_sshd
				rm -f ${COMSA_HDL_SSHD_FILE}
			else
				$debuglogger "com-sshd-wrapper.sh(stop): The main SSHD already restored by COM SA"
			fi
			# need to stop the AMF SSHD as well in case of switchover
			$debuglogger "com-sshd-wrapper.sh calling com_sshd.sh (with stop)..."
			# need to check the existence of com_sshd.sh file before calling it
			# This check aims to guarantee that COM version is being used supports SSHD
			# In case COM does not support, we MUST NOT call com_sshd.sh at all
			if [ "com_sshd.sh" = "`ls /opt/com/bin | grep com_sshd 2>/dev/null`" ]; then
				. /opt/com/bin/com_sshd.sh
				exit 0
			fi
			exit 0
			;;
		status)
			if [ -f "${COMSA_HDL_SSHD_FILE}" ]; then
				$debuglogger "com-sshd-wrapper.sh(status): SSHD is controlled by COM SA"
			else
				# Should we exit with '1' here? Is this an error?
				$debuglogger "com-sshd-wrapper.sh(status): SSHD is controlled by COM SA but ${COMSA_HDL_SSHD_FILE} is not present"
			fi
			exit 0
			;;
		*)
			logger -t $TAG "ERROR: com-sshd-wrapper.sh called with ${1}"
			exit 1
			;;
	esac
else
	$debuglogger "com-sshd-wrapper.sh ${COMSA_AMF_SSHD_FILE} is present."
	case "${1}" in
		start | restart)
			if [ -f "${COMSA_HDL_SSHD_FILE}" ]; then
				$debuglogger "com-sshd-wrapper.sh(start): Restoring and restarting the main SSHD that was controlled by COM SA"
				update_sshd -d
				reload_sshd
				rm -f ${COMSA_HDL_SSHD_FILE}
			else
				$debuglogger "com-sshd-wrapper.sh(start): The main SSHD is already restored by COM SA"
			fi
			;;
		stop)
			if [ -f "${COMSA_HDL_SSHD_FILE}" ]; then
				$debuglogger "com-sshd-wrapper.sh(stop): Restoring and restarting the main SSHD, was controlled by COM SA"
				update_sshd -d
				reload_sshd
				rm -f ${COMSA_HDL_SSHD_FILE}
				# need to stop the AMF SSHD as well in case of switchover
				$debuglogger "com-sshd-wrapper.sh calling com_sshd.sh (with stop)..."
				. /opt/com/bin/com_sshd.sh
				exit 0
			else
				$debuglogger "com-sshd-wrapper.sh(stop): The main SSHD is already restored and restarted by COM SA"
			fi
			;;
		status)
			if [ -f "${COMSA_HDL_SSHD_FILE}" ]; then
				# Should we do 'COM SA SSHD STOP' here? Call 'update_sshd -d' and 'reload_sshd'?
				$debuglogger "com-sshd-wrapper.sh(status): The main SSHD is still controlled by COM SA"
				exit 0
			else
				$debuglogger "com-sshd-wrapper.sh: The main SSHD is no longer configured and controlled by COM SA"
			fi
			;;
		*)
			logger -t $TAG "ERROR: com-sshd-wrapper.sh called with ${1}"
			exit 1
			;;
	esac
fi

## Load the start script and continue execution in com_sshd.sh script in the start function
$debuglogger "com-sshd-wrapper.sh calling com_sshd.sh with '$1'"
. /opt/com/bin/com_sshd.sh
