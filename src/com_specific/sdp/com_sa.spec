#
# spec file for package COM_SA
#
# Copyright (C) 2010 Ericsson AB.
#

%define is_installpmtsa  %(test "%{__disable_pmt_sa}" = "NO" && echo 1 || echo 0)
%define _pkgsudoersdir %{_sysconfdir}/sudoers.d
%define com_sa_so_filename %(basename ${COM_SA_SO})
%define trace_probe_so_filename %(basename ${TRACE_PROBE_SO})
%define pmt_sa_so_filename %(basename ${PMT_SA_SO})
%define is_sle %(test "%{_hostos}" = "sle" && echo 1 || echo 0)
%define uid_tag %(echo ${RECOMMENDS_UID_TAG})
%define gid_tag %(echo ${RECOMMENDS_GID_TAG})

Name:         %{_comsaname}
License:      Proprietary
Group:        Applications/System
Version:      %{_comsaver}
Release:      %{_comsarel}
Summary:      Support Agent for COM
URL:          http://www.ericsson.com
Vendor:       Ericsson AB
Requires:     %{_mafname}  >= %{_comsaver}
Requires:     %{_comname}  >= %{_comsaver}
%if %is_sle
Recommends:   %uid_tag
Recommends:   %gid_tag
%endif
Obsoletes:    ComSa comsa-cxp90176974
Provides:     ComSa comsa-cxp90176974

%description
Support Agent for COM

%pre
com_core_uid_def=311
system_nbi_data_gid_def=302
cmw_log_data_gid_def=314
cmw_imm_users_gid_def=315
cmw_pm_users_gid_def=316
com_core_gid_def=319
lde_cmwea_users_gid_def=356

MAPDIR=/usr/share/ericsson/cba/id-mapping

UIDMAPF=${MAPDIR}/uid.map.defs
UIDMAPOVRDF=${MAPDIR}/uid.map.override
GIDMAPF=${MAPDIR}/gid.map.defs
GIDMAPOVRDF=${MAPDIR}/gid.map.override

## PSO API
PERSISTENT_STORAGE_API="/usr/share/pso/storage-paths"
PERSISTENT_STORAGE_API_CONFIG="${PERSISTENT_STORAGE_API}/config"


# Function append_supplementary_groups
# Usage: append_supplementary_groups groups user
# Append supplemetary groups for a user
# Different behaviour on SLES11 to all other distros, such as SLES12 or RHEL
SLES11_DISTRIBUTION_FILE="/etc/SuSE-release"

function append_supplementary_groups()
{
    # New supplementary groups and username
    local NEWGROUPS="${1}" USER="${2}"
    #detect if SLES 11 = usermod -A // or SLES 12 & RHEL = usermod -a -G
    sles11=''
    if [ -f $SLES11_DISTRIBUTION_FILE ]; then
       sles11=`cat $SLES11_DISTRIBUTION_FILE | grep "VERSION = 11"`
    fi
    if [ -z $sles11] ; then
        usermod -a -G $NEWGROUPS $USER > /dev/null 2>&1
    else
        usermod -A $NEWGROUPS $USER > /dev/null 2>&1
    fi
}
# Function setnewid
# Usage: setnewid  mapoverridefile mapfile id numidname
# If id is found in override or map file, the variable
# $numidname is updated with the new value.
function setnewid()
{
    local IDFILE="" NEWID=""
    # Check for existing UID override files, otherwise use default file
    [[ -f $1 ]] && IDFILE=$(cat $1)
    # Search for username in override file.
    [[ -f $IDFILE ]] && NEWID=$(awk -F= "/$3/ { print \$2 }" $IDFILE)
    # If not found, check the default map file.
    [[ $NEWID -eq 0 && -f $2 ]] && NEWID=$(awk -F= "/$3/ { print \$2 }" $2)
    [[ $NEWID -gt 0 ]] && let "$4=$NEWID"
} 2> /dev/null
# Function create_user_groups
# Usage: create_user_groups user default_uid group default_gid supplementary_groups[@] supplementary_gids[@]
# Create group if not exists. Create user if not exists. Create supplementary groups if not exists and add user to the groups.
# Use provided default user/group ids if not found by looking up mapping files.
# Note: supplementary_groups and supplementary_gids are arrays.
function create_user_groups()
{
    # Username and default uid
    local NEWUSER="${1}" NEWUID="${2}"
    # Group name and default gid
    local NEWGROUP="${3}" NEWGID="${4}"
    # Supplementary groups and default values, note: Array syntax!
    local SUPPGROUPS=("${!5}") SUPPGIDS=("${!6}")
    # Only assign the UID if found in override or default file
    setnewid $UIDMAPOVRDF $UIDMAPF $NEWUSER NEWUID
    # Only assign the GID if found in override or default file
    setnewid $GIDMAPOVRDF $GIDMAPF $NEWGROUP NEWGID
    pkgdatadir="/usr/share/com-core"
    # Test for group and create it if necessary
    if ( ! getent group $NEWGROUP > /dev/null ) ; then
        groupadd -g $NEWGID $NEWGROUP
    fi
    # Test for user and create it if necessary
    if ( ! getent passwd $NEWUSER > /dev/null ) ; then
        useradd -r -u $NEWUID -g $NEWGID -d $pkgdatadir -s /sbin/nologin  -c $NEWUSER $NEWUSER > /dev/null 2>&1
    else
        # If user is already exist then just set the primary group only.
        usermod -g $NEWGID $NEWUSER > /dev/null 2>&1
    fi

    local groups=""
    # Search for each supplementary group in map and override files, as above
    for ((GRP=0; $GRP<${#SUPPGROUPS[@]} ; GRP++)) ; do
        CURGRP=${SUPPGROUPS[GRP]}
        CURGID=${SUPPGIDS[GRP]}
        # Only assign the GID if found in override or default file
        setnewid $GIDMAPOVRDF $GIDMAPF $CURGRP CURGID
        # only create if group does not already exist
        if ( ! getent group $CURGRP > /dev/null ) ; then
            groupadd -g $CURGID $CURGRP
        fi
        # Add group to user's list
        groups=$(echo "$groups $CURGRP")
    done

    # Convert group list to another delimiter
    groups=$(echo $groups | sed 's| |,|g')

    # Set user supplementary groups
    append_supplementary_groups $groups $NEWUSER > /dev/null 2>&1
}


if [ -f $UIDMAPF ] && [ -f $GIDMAPF ] ; then
    # Create users
    LDE_NONROOT_API="/usr/share/ericsson/cba/lde_nonroot"
    if [ -f $LDE_NONROOT_API ] && [ `grep "enabled" $LDE_NONROOT_API` ] ; then
        SG_NAMES=(system-nbi-data cmw-log-data cmw-imm-users cmw-pm-users lde-cmwea-users)
        SG_IDS=(${system_nbi_data_gid_def} ${cmw_log_data_gid_def} ${cmw_imm_users_gid_def} ${cmw_pm_users_gid_def} ${lde_cmwea_users_gid_def})
    else
        SG_NAMES=(system-nbi-data cmw-log-data cmw-imm-users cmw-pm-users )
        SG_IDS=(${system_nbi_data_gid_def} ${cmw_log_data_gid_def} ${cmw_imm_users_gid_def} ${cmw_pm_users_gid_def})
    fi
    create_user_groups com-core ${com_core_uid_def} com-core ${com_core_gid_def} SG_NAMES[@] SG_IDS[@]
fi

#enable systemd service for pre-pso
if [ %is_sle -eq 1 ]; then
    %service_add_pre com-pre-pso.service
fi

%clean

# To unpack the sources, uncomment %-prep and %-setup below, also
# remove the '-' sign between the percent-sign and the 'action':
#%-prep
#%-setup

# Patch to to enable installation to a specified destination directory.
#%-patch1 -p1

%build
install -d $RPM_BUILD_ROOT/opt/com/etc
install -d $RPM_BUILD_ROOT/opt/com/util
install -d $RPM_BUILD_ROOT/opt/com/lib/comp
install -d $RPM_BUILD_ROOT/usr/bin
install -d $RPM_BUILD_ROOT/usr/lib/systemd/system
install -d $RPM_BUILD_ROOT/usr/lib/systemd/system-preset

install -m 750 $COM_SA_SO $RPM_BUILD_ROOT/opt/com/lib/comp
install -m 750 $TRACE_PROBE_SO $RPM_BUILD_ROOT/opt/com/lib
%if %is_installpmtsa
install -m 750 $PMT_SA_SO $RPM_BUILD_ROOT/opt/com/lib/comp
%endif
install -m 750 $SDPSRC/comsa-mim-tool $RPM_BUILD_ROOT/usr/bin
install -m 750 $SDPSRC/restart-com $RPM_BUILD_ROOT/usr/bin
install -m 750 $SDPSRC/comsa_pso $RPM_BUILD_ROOT/usr/bin
install -m 750 $SDPSRC/update_sshd $RPM_BUILD_ROOT/usr/bin
install -m 750 $SDPSRC/reload_sshd $RPM_BUILD_ROOT/usr/bin
install -m 750 $SDPSRC/com-sshd-wrapper.sh $RPM_BUILD_ROOT/usr/bin
install -m 750 $SDPSRC/com-tlsd-wrapper.sh $RPM_BUILD_ROOT/usr/bin
install -m 750 $SDPSRC/is-com-sshd-flag-enabled.sh $RPM_BUILD_ROOT/usr/bin
install -m 750 $SDPSRC/comsa_storage $RPM_BUILD_ROOT/usr/bin
install -m 644 $SDPSRC/com-pre-pso.service $RPM_BUILD_ROOT/usr/lib/systemd/system
install -m 644 $SDPSRC/10-com-oam-server.preset $RPM_BUILD_ROOT/usr/lib/systemd/system-preset
# Moved the comsa configuration files and scripts from deployment packages to rpm
install -m 750 $SDPSRC/comsa_mdf_consumer $RPM_BUILD_ROOT/opt/com/util
install -m 750 $SDPSRC/com_offline_model_consumer $RPM_BUILD_ROOT/opt/com/util
install -m 640 $SDPSRC/coremw-com-sa.cfg $RPM_BUILD_ROOT/opt/com/lib/comp
install -m 640 $SDPSRC/com_sa_trace.conf $RPM_BUILD_ROOT/opt/com/etc
install -m 640 $SDPSRC/comsa.cfg $RPM_BUILD_ROOT/opt/com/etc
install -m 640 $SDPSRC/com_sa_log.cfg $RPM_BUILD_ROOT/opt/com/etc
install -m 755 $SDPSRC/com_pre_pso.sh $RPM_BUILD_ROOT/opt/com/util
install -m 640 $SDPSRC/com-kernel-modules.conf $RPM_BUILD_ROOT/opt/com/util
install -m 640 $SDPSRC/com-kernel-modules.service $RPM_BUILD_ROOT/opt/com/util
install -m 750 $SDPSRC/load_com_kmodules.sh $RPM_BUILD_ROOT/opt/com/util

#enable systemd service for pre-pso
%post
if [ %is_sle -eq 1 ]; then
    %service_add_post com-pre-pso.service
fi

if [ -d "/etc/systemd/system/lde-kernel-modules.service.d" ]; then
    cp /opt/com/util/com-kernel-modules.conf /etc/systemd/system/lde-kernel-modules.service.d/
    chown root:root /etc/systemd/system/lde-kernel-modules.service.d/com-kernel-modules.conf
    chmod 644 /etc/systemd/system/lde-kernel-modules.service.d/com-kernel-modules.conf
fi

#enable and start com-kernel-module.service of COM
cp /opt/com/util/com-kernel-modules.service /usr/lib/systemd/system
chown root:root /usr/lib/systemd/system/com-kernel-modules.service
chmod 644 /usr/lib/systemd/system/com-kernel-modules.service
/usr/bin/systemctl enable com-kernel-modules.service
/usr/bin/systemctl start com-kernel-modules.service

#delete systemd service for pre-pso
%postun
if [ %is_sle -eq 1 ]; then
    %service_del_postun com-pre-pso.service
fi

%files
/opt/com/lib/comp/%{com_sa_so_filename}
/opt/com/lib/%{trace_probe_so_filename}
/opt/com/lib/comp/coremw-com-sa.cfg
/opt/com/etc/com_sa_trace.conf
/opt/com/etc/comsa.cfg
/opt/com/etc/com_sa_log.cfg
/opt/com/util/comsa_mdf_consumer
/opt/com/util/com_offline_model_consumer
/opt/com/util/com_pre_pso.sh
/opt/com/util/com-kernel-modules.conf
/opt/com/util/com-kernel-modules.service
/opt/com/util/load_com_kmodules.sh
/usr/lib/systemd/system/com-pre-pso.service
/usr/lib/systemd/system-preset/10-com-oam-server.preset
%if %is_installpmtsa
/opt/com/lib/comp/%{pmt_sa_so_filename}
%endif
/usr/bin/*


%defattr(-,root,root,-)

%changelog
* Tue Mar 03 2015 Adam Leggo <adam.leggo@dektech.com.au>
- Implementing SUGaR users
* Wed Sep 17 2014 Jonas Jonsson <jonas.l.jonsson@ericsson.com>
- Conditional include of PMT-SA
* Mon Mar 10 2014 Johnny Larsson <johnny.larsson@ericsson.com>
- Addin a start script wrapper for the COM process.
* Fri Nov 09 2012 Jonas Jonsson L <jonas.l.jonsson@ericsson.com>
- Using 'install' command instead of cp and mkdir, and cleaned up.
* Fri Apr 30 2010 Lars G Ekman <lars.g.ekman@ericsson.com>
- Created specfile.
