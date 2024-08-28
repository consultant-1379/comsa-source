#
# spec file for package COM Reencrypt Participant
#
# Copyright (C) 2018 Ericsson AB.
#

Name:         %{_reencryptparticipantname}
License:      Proprietary
Group:        Applications/System
Version:      %{_comsaver}
Release:      %{_comsarel}
Summary:      COM Re-encrypt participant
URL:          http://www.ericsson.com
Vendor:       Ericsson AB
Requires:     %{_comsaname} >= %{_comsaver}

Obsoletes:    com-reencrypt-participant-cxp90176974
Provides:     com-reencrypt-participant-cxp90176974

%description
COM Re-encrypt participant for reencrypting secret attributes

%clean

%build
install -d $RPM_BUILD_ROOT/opt/com/bin
install -m 750 $COM_REENCRYPT_PARTICIPANT $RPM_BUILD_ROOT/opt/com/bin
install -m 750 $REENCRYPTION_UTILS/com-reencrypt-participant.sh $RPM_BUILD_ROOT/opt/com/bin

%files
/opt/com/bin/com-reencrypt-participant
/opt/com/bin/com-reencrypt-participant.sh

%defattr(-,root,root,-)
