#!/bin/sh

# Copyright (C) 2018 by Ericsson AB
#
# The copyright to the computer program herein is the property of
# Ericsson AB. The program may be used and/or copied only with the
# written permission from Ericsson AB, or in accordance with the terms
# and conditions stipulated in the agreement/contract under which the
# program has been supplied.
#
# All rights reserved.


# The script enables number of com-reencryptor process management capabilities.
# $ com-reencrypt-participant.sh <start|stop|status>
#
# The script is designed to work in a limited linux environment where the support of certain utilities
# like for instance ps and pgrep varies.
# This is a bourne shell script and is free from any non compatible syntax (e.g bash specific features)

SELF_NAME=$(basename $0)

MAPDIR="/usr/share/ericsson/cba/id-mapping"
UIDMAPF="${MAPDIR}/uid.map.defs"
GIDMAPF="${MAPDIR}/gid.map.defs"

RUNDIR="/opt/com/run"
COM_USER="com-core"

#define utilchown
utilchown=$(which chown 2> /dev/null)

info() {
    logger -t ${SELF_NAME} "[INFO] $1"
}

error() {
    logger -t ${SELF_NAME} "[ERROR] $1"
    if [ "$2" != "" ]; then
       exit "$2"
    else
       exit 1
    fi
}

warning() {
    logger -t ${SELF_NAME} "[WARNING] $1"
}

#Change file permissions and ownership on a single file
modify_permissions_and_group() {
    if [ -f ${UIDMAPF} ] && [ -f ${GIDMAPF} ]; then
        if [ -f $4 ] ; then
            $utilchown $2:$3 $4
            chmod $1 $4
        fi
    fi
}

get_pid() {
    res=""
    pids=$(ps haxocomm,pid | awk '$1~pname{print $2}' pname=com-reencrypt*)
    for pid in ${pids}
    do
        if grep ${REENCRYPTOR_MODEL_DATA} /proc/${pid}/cmdline > /dev/null 2>&1;
        then
            res="${pid}"
            break
        fi
    done
    echo ${res}
}

get_parent_pid() {
    if [ -n "$1" ]
    then
        ppid=$(cat /proc/${1}/status 2> /dev/null | grep ^PPid: | cut -f 2)
    fi
    echo ${ppid}
}

is_process_running() {
    COM_REENCRYPTOR_PID=$( get_pid )

    #If com-reencryptor process is not running, get_pid returns 0
    if [ "${COM_REENCRYPTOR_PID}" != "" ]
    then
        return 0 # NOTE: ZERO means success in bash
    fi
    return 1
}

updates_for_SUGaR() {

    if [ -f "${UIDMAPF}" ] && [ -f "${GIDMAPF}" ] ; then
        info "Enabling SUGaR support for COM Reencryptor.Exporting COM_USER_MODE=1 for re-encryption participant to run as non-root"
        export COM_USER_MODE=1
        export COM_USER # Least privilege user
    else
        info "com-reencrypt-participant.sh disabling SUGAR support in re-encryption participant - exporting COM_USER_MODE=0"
        export COM_USER_MODE=0
    fi

    # Change the permissions and group of the binary and start up script
    modify_permissions_and_group 750 root ${COM_USER} /opt/com/bin/com-reencrypt-participant
    modify_permissions_and_group 750 root ${COM_USER} /opt/com/bin/com-reencrypt-participant.sh
}

start() {
    info "Starting com-reencrypt-participant daemon"

    if [ ! -d "${RUNDIR}" ]; then
        mkdir -p -m 770 ${RUNDIR}
        chgrp ${COM_USER} ${RUNDIR}
    fi

    # Any file created during runtime should be aligned with umask 007
    umask 007

    updates_for_SUGaR

    # The old com-reencryptor daemon process must be killed before we start the new process
    COM_REENCRYPTOR_PID=$( get_pid )

    if [ "${COM_REENCRYPTOR_PID}" != "" ]; then
        info "com-reencrypt-participant daemon is already running, stopping it (pid: ${COM_REENCRYPTOR_PID})..."
        /bin/kill -9 ${COM_REENCRYPTOR_PID}
    fi

    # Uncomment the below line to enable the debugs in systems where CMW LogM is not used
    # export ENABLE_DEBUGS=1

    ${COM_REENCRYPTOR_CMD} ${REENCRYPTOR_MODEL_DATA} 2>&1 &
    EXIT_CODE=$(echo $?)

    if [ ${EXIT_CODE} -eq 0 ]; then
        COM_REENCRYPTOR_PID=$( get_pid )
        if [ "${COM_REENCRYPTOR_PID}" = "" ]; then
           sleep 1
        fi

        COM_REENCRYPTOR_PID=$( get_pid )

        if [ "${COM_REENCRYPTOR_PID}" != "" ]; then
            info "com-reencrypt-participant daemon process (PID: ${COM_REENCRYPTOR_PID}) successfully started"
        else
            error "com-reencrypt-paticipant daemon process failed to start"
        fi
    else
        error "com-reencrypt-participant daemon process failed to start"
    fi

    exit ${EXIT_CODE}
}

# This method should not return anything except 0. Otherwise, AMF cannot start another component
stop() {
    info "Stopping com-reencrypt-participant daemon"

    COM_REENCRYPTOR_PID=$( get_pid )

    if [ ${COM_REENCRYPTOR_PID} != "" ]; then
        #Check Parent Process ID
        COM_REENCRYPTOR_PPID=$(get_parent_pid "${COM_REENCRYPTOR_PID}")

        if [ -z ${COM_REENCRYPTOR_PPID} ] || [ ${COM_REENCRYPTOR_PPID} -eq 1 ]; then
            info "Parent Process is killed. Trying to kill com-reencrypt-participant daemon now."
        else
            info "Parent Process is NOT killed."
            info "Trying to kill Parent Process. \"kill -9 ${COM_REENCRYPTOR_PPID}\""
            /bin/kill -9 ${COM_REENCRYPTOR_PPID}

            # hold a bit
            usleep 100000
            COM_REENCRYPTOR_PPID_2=$(get_parent_pid "${COM_REENCRYPTOR_PID}")

            if [ ${COM_REENCRYPTOR_PPID_2} -eq 1 ] || [ -z "${COM_REENCRYPTOR_PPID_2}" ]; then
                info "Parent Process is killed Now. Trying to kill com-reencrypt-participant daemon."
            else
                error "Failed to kill parent process(PPID: ${COM_REENCRYPTOR_PPID_2})" "0"
            fi
        fi

        /bin/kill -15 ${COM_REENCRYPTOR_PID} > /dev/null 2>&1 || true

        #Wait max 2 sec
        COUNT=20

        while [ ${COUNT} -gt 0 ]
        do
          if is_process_running ; then
            usleep 100000
            COUNT=$(expr ${COUNT} - 1)
          else
            # hold a bit and break the loop
            info "com-reencrypt-participant process is now closed"
            usleep 100000
            break
          fi
        done

        if is_process_running ; then
          COM_REENCRYPTOR_PID=$( get_pid )
          info "Have to do kill on com-reencrypt-participant (PID: ${COM_REENCRYPTOR_PID})."
          /bin/kill -9 ${COM_REENCRYPTOR_PID}
        fi
    else
        info "com-reencrypt-participant is already stopped."
    fi
}

REENCRYPTOR_MODEL_DATA="/opt/com/run/reencryptor-model-data"
COM_REENCRYPTOR_CMD="/opt/com/bin/com-reencrypt-participant"
[ -x ${COM_REENCRYPTOR_CMD} ] || error "Unable to locate com-reencrypt-participant"

case "$1" in
    start)
        start
        ;;
    stop)
        stop
        ;;
    status)
        if is_process_running
        then
            exit 0
        else
            exit 1;
        fi
        ;;
    *)
        info "Invalid command. Executed with wrong arguments: $0 $*"
        echo "Invalid command: $0 $*"
        echo "Usage: ${SELF_NAME} {start|stop|status}"
        exit 1
esac

exit 0
