#!/bin/sh
##
## This script is the gateway to let AMF control COM TLSD.
## $ com-tlsd-wrapper.sh <start|stop|restart|status>
##
# Copyright (C) 2016 by Ericsson AB
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

SELF_NAME=`basename $0`

# logger command will be invoked with '-t' option to avoid slow execution in case LDAP
# server is not reachable. Using "logger" as tag to maintain backward compatibility
TAG="logger"

## PSO API
PERSISTENT_STORAGE_API="/usr/share/pso/storage-paths"
PERSISTENT_STORAGE_API_CONFIG="${PERSISTENT_STORAGE_API}/config"

CONFIG_REPO=`cat $PERSISTENT_STORAGE_API_CONFIG`

TLSD_DEBUG=0

#define debuglogger
if [ $TLSD_DEBUG -eq 1 ] ; then
	debuglogger="/bin/logger -t $TAG"
else
	debuglogger=/bin/true

fi

info() {
    logger -t $TAG "[INFO ${SELF_NAME}] $1"
}

debug_log() {
    $debuglogger "[DEBUG ${SELF_NAME}] $1"
}

COM_DIR=/com-apr9010443/lib/comp/
COM_TLSD_CFG=libcom_tlsd_manager.cfg
COM_TLS_PROXY_CFG=libcom_tls_proxy.cfg
COM_TLSD_TAG=tlsdManagement
COM_TLSD_CFG_FILE=${CONFIG_REPO}${COM_DIR}${COM_TLSD_CFG}
COM_TLS_PROXY_CFG_FILE=${CONFIG_REPO}${COM_DIR}${COM_TLS_PROXY_CFG}
# check TLSD feature enabled or not
is_tlsd_enabled()
{
    if [ ! -f "${COM_TLSD_CFG_FILE}" ]; then
        COM_TLSD_FLAG=false
        info "com_tlsd.sh ${COM_TLSD_TAG} is UNKNOWN"
    else
        COM_TLSD_FLAG=`cat ${COM_TLSD_CFG_FILE} | grep ${COM_TLSD_TAG} | sed 's/</ /g;s/>/ /g' | awk '{print $2}'`
	if $COM_TLSD_FLAG; then
	    if [ -f "${COM_TLS_PROXY_CFG_FILE}" ]; then
                COM_TLSD_FLAG=`cat ${COM_TLS_PROXY_CFG_FILE} | grep ${COM_TLSD_TAG} | sed 's/</ /g;s/>/ /g' | awk '{print $2}'`
            fi
	fi
    fi

    if $COM_TLSD_FLAG; then
        info "TLSD feature enabled"
        return 0
    else
        info "TLSD feature disabled"
        return 1
    fi
}

# check if the COM SA file for TLSD control exists
COMSA_AMF_TLSD=AMF_handles_TLSD
COMSA_AMF_TLSD_FILE=/opt/com/run/${COMSA_AMF_TLSD}

debug_log  "called with $1"
case "${1}" in
		start | restart)
			info "called with $1"
			if is_tlsd_enabled
			then
			    if [ ! -f "${COMSA_AMF_TLSD_FILE}" ]; then
			        touch ${COMSA_AMF_TLSD_FILE}
				chown com-core:com-core ${COMSA_AMF_TLSD_FILE}
			    fi
			    debug_log "Created comsa amf tlsd file ${COMSA_AMF_TLSD_FILE}"
			else
			    rm -rf ${COMSA_AMF_TLSD_FILE}
			    debug_log "Removed comsa amf tlsd file ${COMSA_AMF_TLSD_FILE}"
			    exit 0
			fi
			;;
		stop)
                        info "called with $1"
			if [ ! -f "${COMSA_AMF_TLSD_FILE}" ]; then
			    debug_log "Comsa Amf tlsd file ${COMSA_AMF_TLSD_FILE}  not exists in stop()"
			    exit 0
			fi
			;;
		status)
			if [ ! -f "${COMSA_AMF_TLSD_FILE}" ]; then
			    debug_log "Comsa Amf tlsd file ${COMSA_AMF_TLSD_FILE}  not exists in status()"
			    exit 0
			fi
			;;
		*)
			info "ERROR: com-tlsd-wrapper.sh called with ${1}"
			exit 1
			;;
esac


## Load the start script and continue execution in com_tlsd.sh script in the start function
debug_log "calling com_tlsd.sh with '$1'"
. /opt/com/bin/com_tlsd.sh
