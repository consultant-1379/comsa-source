#
# Copyright (C) 2010 by Ericsson AB
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
#
# File: Makefile for CBA_SA
#
# The COM_SA is a simple program from a build point-of-view.
# One Makefile is considered to be enough.
#
# Author: uablrek
#
# Date:   2010-04-13
#
# Reviewed: efaiami 2010-10-06
#
# Modify: efaiami 2011-01-31 for LSB Complience
# Modify: efaiami 2011-07-05 for OI
# Modify: efaiami 2011-11-17 removed accessmgm
# Modify: xduncao 2012-06-07 for sdp1694
# Modify: efaiami 2012-08-30 for source code support
# Modify: xnikvap 2012-11-30 added code coverage support
# Modify: ejonajo 2013-03-11 bitbake adaptations, image based install support
# Modify: uabjoy  2014-02-17 adding build support for Trace CC adoptions
# Modify: xadaleg 2014-08-02 MR35347 - increase DN length
# Modify: xadaleg 2015-04-16 MR36131 - Processes should execute with least required privilege
# Modify: xjonbuc 2015-07-07 MR39525 CBA Packaging and Versioning
# Modify: xadaleg 2015-07-17 MR42277: Remove old interfaces and models
# Modify: xtronle 2015-07-25 MR40724: CBA offline installable RPMs for VNF image creation
# Modify: xadaleg 2015-10-12 MR43481: COM Internal: CI/Build - COMSA integrated in COM Step 1

# Build-tools. Usually the LSB Complience
DESTDIR := $(DESTDIR)
COMSA_VERSION ?= $(SA_VERSION)
ARCHITECTURE := $(ARCHITECTURE)
CC := $(CC)
CXX := $(CXX)
LD := $(CXX)

# CoreMW-COM_SA dirs. Default is the com_sa vob.
COM_SA_PWD :=  $(COMSA_DEV_DIR)/src
SLES_CBA_SA_CXPA := $(COMSA_RELEASE)
RHEL_CBA_SA_CXPA := $(COMSA_RELEASE)/rhel
COM_SA_DEV ?= $(COM_SA_PWD)/..
COM_SA_DEP ?= $(COM_SA_DEV)/dependencies
COM_SA_DEPLOYMENT_TEMPLATES ?= $(COM_SA_DEV)/deployment_templates
LD_LIBPATH ?= ./

ifeq ($(ENV_TYPE),Lm)
    COMSA_FOR_COREMW_DIR := lmsa-apr9010528
    CC_NAME := Lm
else
    COMSA_FOR_COREMW_DIR := comsa_for_coremw-apr9010555
    CC_NAME := Com
endif

# Includes, Libs and flags
SAF_INCL ?= -I$(COM_SA_DEP)/core_mw_api/inc
SAF_LIB ?= -L$(LSB_SHAREDLIBPATH)

LD_LIB ?= -L$(LD_LIBPATH)
COM_INCL ?= -I$(COM_SA_DEP)/com_spi
MAF_INCL ?= -I$(COM_SA_DEP)/maf_spi
TRACE_INCL ?= -I$(COM_SA_DEP)/tracesdk/usr/include

# If not set, then we should build PMT-SA as well
DISABLE_PMT_SA ?= NO
# If not set, we should build COM Re-encryption PArticipant as well
#TODO: Once we have source code ready, modify the value to NO to get this rpm by default in build
DISABLE_COM_REENCRYPTION ?= NO

ifeq ($(DISABLE_PMT_SA),YES)

$(info Performance Management Transfer service agent is DISABLED)

else

# Be sure that the value is 'NO' and nothing else.
DISABLE_PMT_SA=NO

endif

ifeq ($(DISABLE_COM_REENCRYPTION),YES)

$(info COM Re-encryption participant is DISABLED)

else

# Be sure that the value is 'NO' and nothing else.
DISABLE_COM_REENCRYPTION=NO

endif

#
# CPPFLAGS, CFLAGS, CXXFLAGS and LDFLAGS can all be brutally over-written
# if they're specified in the build command (example: make CFLAGS=-m64),
# thus we make sure that the following is always passed on
#
CSA_CPPFLAGS := -I$(COM_SA_DEV)/src/generic/incl -I$(COM_SA_DEV)/src/com_specific/incl $(SAF_INCL) $(COM_INCL) $(MAF_INCL) $(TRACE_INCL)
CSA_CFLAGS += -Wall -fPIC -pthread -g -DSA_EXTENDED_NAME_SOURCE -DCOMSA_FOR_COREMW_DIR='"$(COMSA_FOR_COREMW_DIR)"' -DCC_NAME='"$(CC_NAME)"'
ifeq ($(COMSA_THUNK),ON)
CSA_CFLAGS += -mindirect-branch=thunk
endif
CSA_CXXFLAGS := $(CSA_CFLAGS)
CSA_LDFLAGS += $(SAF_LIB) -lSaLog -lSaAmf -lSaNtf -lSaImmOm -lSaImmOi -lSaPmConsumer -lxml2 -lpthread -lrt -Wl,--no-undefined -ldl $(LD_LIB)
ifeq ($(DISABLE_COM_REENCRYPTION),NO)
CSA_CPPFLAGS += -I$(COM_SA_DEV)/src/com_specific/com_reencryptor/inc $(SAF_INCL)
CSA_REENCRYPT_LDFLAGS := $(SAF_LIB) -lSaImmOi -lSaLog -lSaImmOm -lpthread -lrt -Wl,--allow-shlib-undefined -Wl,-rpath-link,$(LSB_SHAREDLIBPATH) -ldl $(LD_LIB)
endif

# Required programs ...
# Default get all these binarys in PATH env. If they are stored in other location,
# please update it manually.
export ECHO  := echo
RPM2CPIO := rpm2cpio
RPMBUILD := rpmbuild
TAR   := tar

ftest: CSA_CFLAGS += -DFTEST
ftest: CSA_CXXFLAGS += -DFTEST

ftest_v4: CSA_CFLAGS += -DFTEST
ftest_v4: CSA_CXXFLAGS += -DFTEST

coverage: CSA_CFLAGS   += -DCODE_COVERAGE --coverage
coverage: CSA_CXXFLAGS += -DCODE_COVERAGE --coverage
coverage: CSA_LDFLAGS  += -lgcov

#Setting the product numbers to COM or LM accordingly based on the COMPONENT_NAME set
#default comsa is built when no variable is set
#And setting the name of the rpm to be generated accordingly
#RPMBUILD_BUILD_PREFIX is the rpmbuild argument needed by the
#LM environment which is being set
#RHEL is set in case of COM

ifeq ($(ENV_TYPE),Lm)
     COMPONENT_NAME=lm
     RPMBUILD_PREFIX_OPT:=$(LMSA_BUILD_PREFIX)
     SLES_CBASA_CXP := CXP9020396_1
     SUPPORTED_SLES_DISTRO := sle12
     COM_SA_RESULT ?= /tmp/LM_SA_RESULT-$(USER)
     DISABLE_PMT_SA=YES
     DISABLE_COM_REENCRYPTION=YES
else
     COMPONENT_NAME=com
     COM_SA_RESULT ?= /tmp/COM_SA_RESULT-$(USER)
     RPMBUILD_PREFIX_OPT:=$(COM_SA_RESULT)
     SLES_CBASA_CXP := CXP9028493_1
     RHEL_CBASA_CXP := CXP9028494_1
     SUPPORTED_SLES_DISTRO := sle12
     SUPPORTED_RHEL_DISTRO := el6
     RHEL_RPM_NAME_TAG := $(shell echo "$(COMPONENT_NAME)-$(COMPONENT_NAME)sa-$(RHEL_CBASA_CXP)" | tr A-Z a-z | head -c -3)
     RHEL_COM_RPM_NAME := $(shell echo "$(COMPONENT_NAME)-$(RHEL_CBASA_CXP)" | tr A-Z a-z | head -c -3)
     RHEL_MAF_RPM_NAME := $(shell echo "$(COMPONENT_NAME)-maf-$(RHEL_CBASA_CXP)" | tr A-Z a-z | head -c -3)
endif


# RPM Name tag can be either of the following based on the COMPONENT_NAME being set
#com-comsa-<cxp_number>.rpm -- all lowercase without '_'
#lm-lmsa-<cxp_number>.rpm -- all lowercase without '_'
SLES_RPM_NAME_TAG := $(shell echo "$(COMPONENT_NAME)-$(COMPONENT_NAME)sa-$(SLES_CBASA_CXP)" | tr A-Z a-z | head -c -3)
SLES_COM_RPM_NAME := $(shell echo "$(COMPONENT_NAME)-$(SLES_CBASA_CXP)" | tr A-Z a-z | head -c -3)
SLES_MAF_RPM_NAME := $(shell echo "$(COMPONENT_NAME)-maf-$(SLES_CBASA_CXP)" | tr A-Z a-z | head -c -3)

# The COM_SA library to be loaded by COM
COM_SA_SO := $(COM_SA_RESULT)/coremw-$(COMPONENT_NAME)-sa.so
ifeq ($(DISABLE_PMT_SA),NO)
PMT_SA_SO := $(COM_SA_RESULT)/coremw-pmt-sa.so
endif
ifeq ($(DISABLE_COM_REENCRYPTION),NO)
COM_RENCRYPT_PARTICIPANT_RPM_NAME_TAG := com-reencrypt-participant
COM_REENCRYPT_PARTICIPANT := $(COM_SA_RESULT)/com-reencrypt-participant
endif
ifneq ($(ENV_TYPE),Lm)
TRACE_PROBE_SO := $(COM_SA_RESULT)/$(COMPONENT_NAME)sa_tp.so
endif


# RPM Version tag is defined as <major_ver>.<minor_release>.<correction>
# <major ver> is last part of CXP  / <minor_release> is the current development track
MAJOR_REL := $(shell echo $(SLES_CBASA_CXP) | cut -d '_' -f 2 )
MINOR_REL := $(shell echo $(COMSA_VERSION) | cut -c 2-3 | sed -e 's/[A-Z]//' )
MINOR_REL := $(shell echo ${MINOR_REL}-1 |bc )
## CP NUMBER needs to be translated from legal letter to number --(should align with comsabuild)
## Legal letters are ABCDEFGHJKLMNSTUVXYZ
CP_NUMBER := $(shell echo $(COMSA_VERSION) | cut -c 2- | sed -e 's/[0-9]//g' |  tr "[A-H]" "[0-7]" | sed -e 's_J_8_g' -e 's_K_9_g' -e 's_L_10_g' -e 's_M_11_g' -e 's_N_12_g' -e 's_S_13_g' -e 's_T_14_g' -e 's_U_15_g' -e 's_V_16_g' -e 's_X_17_g' -e 's_Y_18_g'  -e 's_Z_19_g' )


RPM_VERSION_TAG := $(MAJOR_REL).$(MINOR_REL).$(CP_NUMBER)


# RPM Release tag is <shipment number>.<optional_field>.<distribution> for releases - R revision
# RPM Release tag is <git development branch> for development - R revision
## TBD - optional field
ifeq ($(shell echo $(COMSA_VERSION) | cut -c1),P)
	SLES_RPM_RELEASE_TAG := $(CURRENT_GIT_BRANCH)
        ifneq ($(ENV_TYPE),Lm)
	RHEL_RPM_RELEASE_TAG := $(CURRENT_GIT_BRANCH)
        endif
else
	#Trim leading letters and 0 from Revision to get Build number
	BUILD_NUMBER := $(shell echo $(COMSA_VERSION) |  cut -c4- | sed -e 's/^[A-Z]//' | sed -e 's/^0*//')
	SLES_RPM_RELEASE_TAG := $(BUILD_NUMBER).$(SUPPORTED_SLES_DISTRO)
        ifneq ($(ENV_TYPE),Lm)
	RHEL_RPM_RELEASE_TAG := $(BUILD_NUMBER).$(SUPPORTED_RHEL_DISTRO)
        endif
endif

#RPM filename is concatenation of Name-Version-Release tags.
SLES_RPM := $(SLES_RPM_NAME_TAG)-$(RPM_VERSION_TAG)-$(SLES_RPM_RELEASE_TAG).$(ARCHITECTURE).rpm
ifeq ($(DISABLE_COM_REENCRYPTION),NO)
REENCRYPTION_RPM := $(COM_RENCRYPT_PARTICIPANT_RPM_NAME_TAG)-$(RPM_VERSION_TAG)-$(SLES_RPM_RELEASE_TAG).$(ARCHITECTURE).rpm
endif
ifneq ( $(ENV_TYPE), Lm)
RHEL_RPM := $(RHEL_RPM_NAME_TAG)-$(RPM_VERSION_TAG)-$(RHEL_RPM_RELEASE_TAG).$(ARCHITECTURE).rpm
endif

ifeq ($(COMSA_DEV_DIR_ABS_PATH),)
SDPSRC := $(COM_SA_DEV)/src/com_specific/sdp
REENCRYPTION_UTILS := $(COM_SA_DEV)/src/com_specific/com_reencryptor/utils
else
SDPSRC := $(COMSA_DEV_DIR_ABS_PATH)/src/com_specific/sdp
REENCRYPTION_UTILS := $(COMSA_DEV_DIR_ABS_PATH)/src/com_specific/com_reencryptor/utils
endif
TMPD := $(COM_SA_RESULT)/tmp

#Spec file of comsa is in the comsa source
#Spec file of Lmsa is in the rpm_specs folder of Lm
ifeq ($(ENV_TYPE),Lm)
SPEC_FILE_LOCATION:=$(RPM_SPECS)
SDPSRC:=$(CBASA_SPECIFIC)
else
SPEC_FILE_LOCATION:=$(SDPSRC)
endif

# Add your source-dirs here!
VPATH = \
	$(COM_SA_DEV)/src/generic/ac \
	$(COM_SA_DEV)/src/com_specific/mdf \
	$(COM_SA_DEV)/src/generic/log \
	$(COM_SA_DEV)/src/generic/rlist \
	$(COM_SA_DEV)/src/generic/trace \
	$(COM_SA_DEV)/src/generic/internal_trace \
	$(COM_SA_DEV)/src/generic/accessmgm/utils \
	$(COM_SA_DEV)/src/generic/motransaction \
	$(COM_SA_DEV)/src/generic/motransaction/OIproxy \
	$(COM_SA_DEV)/src/generic/ntf \
	$(COMSA_VERIFICATION)/ft/replListTestApp \
	$(COM_SA_DEV)/src/generic/utils 

ifeq ($(DISABLE_PMT_SA),NO)
# Add pmt source folder to VPATH
VPATH += $(COM_SA_DEV)/src/generic/pmt_sa/src
endif

ifeq ($(DISABLE_COM_REENCRYPTION),NO)
VPATH += $(COM_SA_DEV)/src/com_specific/com_reencryptor/src
endif

# Add your objs here!
COM_SA_OBJS := \
	$(COM_SA_RESULT)/ComSAAc.o \
	$(COM_SA_RESULT)/SaInternalTrace.o \
	$(COM_SA_RESULT)/ComSAMDF.o \
	$(COM_SA_RESULT)/ComSALogService.o \
	$(COM_SA_RESULT)/SelectTimer.o \
	$(COM_SA_RESULT)/ComSARlist.o \
	$(COM_SA_RESULT)/ComSATraceService.o \
	$(COM_SA_RESULT)/ImmCmd.o \
	$(COM_SA_RESULT)/OamSACache.o \
	$(COM_SA_RESULT)/OamSADataClasses.o \
	$(COM_SA_RESULT)/OamSAImmBridge.o \
	$(COM_SA_RESULT)/OamSAManagedObjects.o \
	$(COM_SA_RESULT)/OamSAOIProxy.o \
	$(COM_SA_RESULT)/OamSAOIProxyStructCache.o \
	$(COM_SA_RESULT)/OamSARegisterObjectImplementer.o \
	$(COM_SA_RESULT)/OamSARegisterObjectUtils.o \
	$(COM_SA_RESULT)/OamSATransactionalResource.o \
	$(COM_SA_RESULT)/OamSATransactionRepository.o \
	$(COM_SA_RESULT)/OamSAKeyAttributeRepository.o \
	$(COM_SA_RESULT)/MOMRootRepository.o \
	$(COM_SA_RESULT)/OamSATranslator.o \
	$(COM_SA_RESULT)/TxContext.o \
	$(COM_SA_RESULT)/trace.o \
	$(COM_SA_RESULT)/LogEventProducer.o \
	$(COM_SA_RESULT)/ComSANtf.o \
	$(COM_SA_RESULT)/CmEventHandler.o \
	$(COM_SA_RESULT)/CmEventProducer.o \
	$(COM_SA_RESULT)/ComSAOamComponent.o \
	$(COM_SA_RESULT)/imm_utils.o \
	$(COM_SA_RESULT)/ProcessUtil.o \
	$(COM_SA_RESULT)/Utils.o \
	$(COM_SA_RESULT)/OsCommand.o \
	$(COM_SA_RESULT)/saname_utils.o \
	$(COM_SA_RESULT)/ComSARlist_imm.o

ifeq ($(DISABLE_PMT_SA),NO)
PMT_SA_OBJS := \
	$(COM_SA_RESULT)/PerfMgmtTransferSA.o \
	$(COM_SA_RESULT)/PmComComponent.o \
	$(COM_SA_RESULT)/PmRunnable.o \
	$(COM_SA_RESULT)/PmEventHandler.o \
	$(COM_SA_RESULT)/PmInternalMessageStream.o \
	$(COM_SA_RESULT)/PmInternalMsg.o \
	$(COM_SA_RESULT)/PmConsumerInterface.o \
	$(COM_SA_RESULT)/PmConsumer.o \
	$(COM_SA_RESULT)/PmtSaTrace.o \
	$(COM_SA_RESULT)/PmShowCounters.o \
	$(COM_SA_RESULT)/PmSaSelectTimer.o \
	$(COM_SA_RESULT)/imm_utils.o \
	$(COM_SA_RESULT)/saname_utils.o \
	$(COM_SA_RESULT)/Utils.o \
	$(COM_SA_RESULT)/OsCommand.o \
	$(COM_SA_RESULT)/debug_log.o \
	$(COM_SA_RESULT)/trace.o
endif

ifeq ($(DISABLE_COM_REENCRYPTION),NO)
#TODO: Add the required reencryptor objects here and uncomment the below line
REENCRYPT_OBJS := \
	$(COM_SA_RESULT)/FileReaderUtil.o \
	$(COM_SA_RESULT)/ImmUtil.o \
	$(COM_SA_RESULT)/Reencryptor.o \
	$(COM_SA_RESULT)/SecUtil.o \
	$(COM_SA_RESULT)/Trace.o \
	$(COM_SA_RESULT)/TimerSelect.o \
	$(COM_SA_RESULT)/main.o

endif

ifneq ( $(ENV_TYPE),Lm)
TRACEPROBE_OBJS := \
	$(COM_SA_RESULT)/comsa_tp.o
endif



TESTOBJS := $(COM_SA_RESULT)/TestComponent.o

TESTOBJS_V4 := $(COM_SA_RESULT)/TestComponent_v4.o

ifeq ($(DISABLE_PMT_SA),NO)

DEPS = $(shell mkdir -p $(COM_SA_RESULT)) $(COM_SA_OBJS:.o=.d) $(PMT_SA_OBJS:.o=.d) $(TRACEPROBE_OBJS:.o=.d) $(REENCRYPT_OBJS:.o=.d) 

else

DEPS = $(shell mkdir -p $(COM_SA_RESULT)) $(COM_SA_OBJS:.o=.d) $(TRACEPROBE_OBJS:.o=.d) 

endif

# Implicit rules

$(COM_SA_RESULT)/%.o: %.c
	$(COMPILE.c) $(CSA_CPPFLAGS) $(CSA_CFLAGS) -o $@ $<

$(COM_SA_RESULT)/%.o: %.cc
	$(COMPILE.cc) $(CSA_CPPFLAGS) $(CSA_CXXFLAGS) -o $@ $<

$(COM_SA_RESULT)/%.o: %.cxx
	$(COMPILE.cc) $(CSA_CPPFLAGS) $(CSA_CXXFLAGS) -o $@ $<

$(COM_SA_RESULT)/%.o: %.cpp
	$(COMPILE.cpp) $(CSA_CPPFLAGS) $(CSA_CXXFLAGS) -o $@ $<

$(COM_SA_RESULT)/%.d: %.c
	@$(ECHO) -n $(COM_SA_RESULT)/ > $@
	@$(COMPILE.c) $(CSA_CPPFLAGS) -MM $< >> $@

$(COM_SA_RESULT)/%.d: %.cc
	@$(ECHO) -n $(COM_SA_RESULT)/ > $@
	@$(COMPILE.cc) $(CSA_CPPFLAGS) -MM $< >> $@

$(COM_SA_RESULT)/%.d: %.cxx
	@$(ECHO) -n $(COM_SA_RESULT)/ > $@
	@$(COMPILE.cc) $(CSA_CPPFLAGS) -MM $< >> $@

$(COM_SA_RESULT)/%.d: %.cpp
	@$(ECHO) -n $(COM_SA_RESULT)/ > $@
	@$(COMPILE.cpp) $(CSA_CPPFLAGS) -MM $< >> $@

# Main targets

.DEFAULT: sles_rpm

.PHONY: clean all sles_rpm ftest install destdircheck dist coverage traceprobe ftest_v4 

sles_rpm: $(COM_SA_RESULT)/$(SLES_RPM)

rhel_rpm: $(COM_SA_RESULT)/$(RHEL_RPM)

destdircheck:
	@if [ -n "$(DESTDIR)" ]; then \
		if [ -d $(DESTDIR) ]; then \
			$(ECHO) Installing into folder: $(DESTDIR) ;\
		else \
			mkdir -p $(DESTDIR); \
		fi; \
	else \
		$(ECHO) The variable \'DESTDIR\' must be given when doing install-targets; \
		exit 1; \
	fi

$(SLES_CBA_SA_CXPA) :
	if [ ! -d $@ ] ; then \
		mkdir -p $@ ; \
	fi

$(RHEL_CBA_SA_CXPA) :
	if [ ! -d $@ ] ; then \
		mkdir -p $@ ; \
	fi

# Don't provide loads and loads of test-files ....
dist:
	@rm -f $(COMSA_DEV_DIR)/COM_SA_SRC-CAY901203_1-$(COMSA_VERSION).tar.gz
	@cd $(COMSA_DEV_DIR); \
	 $(TAR) czf $(COMSA_DEV_DIR)/COM_SA_SRC-CAY901203_1-$(COMSA_VERSION).tar.gz \
		./dependencies \
		./src/Makefile ./src/generic \
		./src/com_specific/incl ./src/com_specific/mdf \
		./src/com_specific/sdp
	@$(ECHO) Created $(COMSA_DEV_DIR)/COM_SA_SRC-CAY901203_1-$(COMSA_VERSION).tar.gz

STORAGE_CONFIG ?= /cluster/storage/system/config
STORAGE_SOFTWARE ?= /cluster/storage/system/software
STORAGE_CLEAR ?=/cluster/storage/clear
STORAGE_NOBACKUP ?= /cluster/storage/no-backup
DATADIR ?= /usr/share
COMCOMPDIR ?= /opt/com/lib/comp
PN ?= com-sa

#install target is executed based on the COMPONENT_NAME being set
install: destdircheck $(COM_SA_SO) $(PMT_SA_SO) $(COM_REENCRYPT_PARTICIPANT)
	ifeq ($(ENV_TYPE),Lm)
		@install -m 755 -d $(DESTDIR)/opt/lm/lib/comp
		@install -m 755 -d $(DESTDIR)/opt/lm/etc
		@install -m 755 $(COM_SA_SO) $(DESTDIR)/opt/lm/lib/comp
		@install -m 644 $(SDPSRC)/coremw-lm-sa.cfg $(DESTDIR)/opt/lm/lib/comp/coremw-lm-sa.cfg
		@install -m 644 $(SDPSRC)/lm_sa_trace.conf $(DESTDIR)/opt/lm/etc/lm_sa_trace.conf
		@install -m 640 $(SDPSRC)/lm_sa_log.cfg $(DESTDIR)/opt/lm/etc/lm_sa_log.cfg
		@install -m 640 $(SDPSRC)/lmsa.cfg $(DESTDIR)/opt/lm/etc/lmsa.cfg
		@install -m 755 -d $(DESTDIR)/usr/bin
		@install -m 750 $(SDPSRC)/lmsa_pso  $(DESTDIR)/usr/bin/lmsa_pso
		@install -d $(DESTDIR)/etc
	else
		@install -m 750 -d $(DESTDIR)/opt/com/etc
		@install -m 750 -d $(DESTDIR)/opt/com/util
		@install -m 750 -d $(DESTDIR)/opt/com/lib/comp
		@install -m 755 -d $(DESTDIR)/usr/bin
		@install -m 750 $(COM_SA_SO) $(DESTDIR)/opt/com/lib/comp
		ifeq ($(DISABLE_PMT_SA),NO)
			@install -m 750 $(PMT_SA_SO) $(DESTDIR)/opt/com/lib/comp
		endif
		ifeq ($(DISABLE_COM_REENCRYPTION),NO)
			@install -m 750 -d $(DESTDIR)/opt/com/bin
			@install -m 750 $(COM_REENCRYPT_PARTICIPANT) $(DESTDIR)/opt/com/bin
			@install -m 750 $(REENCRYPTION_UTILS)/com-reencrypt-participant.sh $(DESTDIR)/opt/com/bin
		endif
		@install -m 640 $(SDPSRC)/coremw-com-sa.cfg $(DESTDIR)/opt/com/lib/comp/
		@install -m 750 $(SDPSRC)/comsa-mim-tool $(DESTDIR)/usr/bin
		@install -m 750 $(SDPSRC)/restart-com $(DESTDIR)/usr/bin
		@install -m 750 $(SDPSRC)/comsa_pso $(DESTDIR)/usr/bin/
		@install -m 750 $(SDPSRC)/update_sshd $(DESTDIR)/usr/bin
		@install -m 750 $(SDPSRC)/reload_sshd $(DESTDIR)/usr/bin
		@install -m 750 $(SDPSRC)/com-sshd-wrapper.sh $(DESTDIR)/usr/bin
		@install -m 644 $(SDPSRC)/com-pre-pso.service $(DESTDIR)/usr/lib/systemd/system
		@install -m 644 $(SDPSRC)/10-com-oam-server.preset $(DESTDIR)/usr/lib/systemd/system-preset

		@install -m 750 $(SDPSRC)/com-tlsd-wrapper.sh $(DESTDIR)/usr/bin
		@install -m 750 $(SDPSRC)/is-com-sshd-flag-enabled.sh $(DESTDIR)/usr/bin
		@install -m 750 $(SDPSRC)/comsa_storage $(DESTDIR)/usr/bin/
		@install -m 750 $(SDPSRC)/comsa_mdf_consumer $(DESTDIR)/opt/com/util/
		@install -m 750 $(SDPSRC)/com_offline_model_consumer $(DESTDIR)/opt/com/util
		@install -m 755 $(SDPSRC)/com_pre_pso.sh $(DESTDIR)/opt/com/util/
		@install -m 750 $(SDPSRC)/load_com_kmodules.sh $(DESTDIR)/opt/com/util/
		@install -m 640 $(SDPSRC)/com-kernel-modules.conf $(DESTDIR)/opt/com/util/
		@install -m 640 $(SDPSRC)/com-kernel-modules.service $(DESTDIR)/opt/com/util/
		@install -m 640 $(SDPSRC)/com_sa_trace.conf $(DESTDIR)/opt/com/etc/
		@install -m 640 $(SDPSRC)/com_sa_log.cfg $(DESTDIR)/opt/com/etc/
		@install -m 640 $(SDPSRC)/comsa.cfg $(DESTDIR)/opt/com/etc/

		@sed -i "s,/opt/com/etc/com_sa_trace.conf,${STORAGE_CLEAR}/${COMSA_FOR_COREMW_DIR}/com_sa_trace.conf," $(DESTDIR)$(COMCOMPDIR)/coremw-com-sa.cfg
		@sed -i "s,/opt/com/log/com_sa.trace,${STORAGE_CLEAR}/${COMSA_FOR_COREMW_DIR}/com_sa.trace," $(DESTDIR)$(COMCOMPDIR)/coremw-com-sa.cfg
		@install -d $(DESTDIR)$(STORAGE_CONFIG)/$(COMSA_FOR_COREMW_DIR)/repository
		@install -d $(DESTDIR)$(STORAGE_CONFIG)/$(COMSA_FOR_COREMW_DIR)/etc
		@install -d $(DESTDIR)$(STORAGE_CLEAR)/$(COMSA_FOR_COREMW_DIR)
		@install -d $(DESTDIR)$(STORAGE_NOBACKUP)/$(COMSA_FOR_COREMW_DIR)/backup
		@install -d $(DESTDIR)$(STORAGE_SOFTWARE)
		@install -d $(DESTDIR)$(DATADIR)/$(PN)/etf
		@install -d $(DESTDIR)/var/opt/comsa
	endif

all: $(COM_SA_SO) $(PMT_SA_SO) $(TRACE_PROBE_SO) $(COM_REENCRYPT_PARTICIPANT) 
	@$(ECHO) Done $@

coverage: sles_rpm rhel_rpm
	$(TAR) zcf $(SLES_CBA_SA_CXPA)/obj.files.tgz $(COM_SA_RESULT)/
	$(TAR) zcf $(RHEL_CBA_SA_CXPA)/obj.files.tgz $(COM_SA_RESULT)/


ftest: $(TESTOBJS) $(COM_SA_OBJS)
	$(LD) -o $(COM_SA_SO) -shared $(TESTOBJS) $(COM_SA_OBJS) $(CSA_LDFLAGS)
	$(MAKE) sles_rpm
	$(MAKE) rhel_rpm


ftest_v4: $(TESTOBJS_V4) $(COM_SA_OBJS)
	@$(ECHO) Building the Test Component for COM SA 4.0 ... $@
	$(LD) -o $(COM_SA_SO) -shared $(TESTOBJS_V4) $(COM_SA_OBJS) $(CSA_LDFLAGS)
	$(MAKE) sles_rpm
	$(MAKE) rhel_rpm

traceprobe: $(TRACE_PROBE_SO)


sles_rpm: $(COM_SA_RESULT)/$(SLES_RPM)
ifeq ($(DISABLE_COM_REENCRYPTION),NO)
sles_rpm: $(COM_SA_RESULT)/$(REENCRYPTION_RPM)
endif

$(COM_SA_RESULT)/$(SLES_RPM): $(SPEC_FILE_LOCATION)/$(COMPONENT_NAME)_sa.spec $(COM_SA_SO) $(PMT_SA_SO) $(TRACE_PROBE_SO) 
	@rm -rf $(COM_SA_RESULT)/buildroot $(COM_SA_RESULT)/rpmtop
	@mkdir -p $(COM_SA_RESULT)/buildroot $(COM_SA_RESULT)/rpmtop/BUILD \
                $(COM_SA_RESULT)/rpmtop/RPMS/$(ARCHITECTURE)
	COM_SA_SO=$(COM_SA_SO) PMT_SA_SO=$(PMT_SA_SO) TRACE_PROBE_SO=$(TRACE_PROBE_SO) SDPSRC=$(SDPSRC) \
	$(RPMBUILD) --target $(ARCHITECTURE) --quiet -bb \
                --buildroot $(COM_SA_RESULT)/buildroot \
                --define='_prefix $(RPMBUILD_PREFIX_OPT)'\
                --define='_$(COMPONENT_NAME)saname $(SLES_RPM_NAME_TAG)'\
                --define='_$(COMPONENT_NAME)saver $(RPM_VERSION_TAG)'\
                --define='_$(COMPONENT_NAME)sarel $(SLES_RPM_RELEASE_TAG)'\
                --define='_mafname $(SLES_MAF_RPM_NAME)'\
                --define='_comname $(SLES_COM_RPM_NAME)'\
                --define='_topdir $(COM_SA_RESULT)/rpmtop' \
                --define='__disable_pmt_sa $(DISABLE_PMT_SA)'\
                $(SPEC_FILE_LOCATION)/$(COMPONENT_NAME)_sa.spec > /dev/null
	cp $(COM_SA_RESULT)/rpmtop/RPMS/$(ARCHITECTURE)/$(SLES_RPM) $(COM_SA_RESULT)
	cp $(COM_SA_RESULT)/$(SLES_RPM) $(COMSA_RELEASE)
	@$(ECHO) Built $@

$(COM_SA_RESULT)/$(REENCRYPTION_RPM): $(REENCRYPTION_UTILS)/$(COMPONENT_NAME)_reencrypt_participant.spec $(COM_REENCRYPT_PARTICIPANT)
	@rm -rf $(COM_SA_RESULT)/buildroot/com_reencyrptor $(COM_SA_RESULT)/rpmtop
	@mkdir -p $(COM_SA_RESULT)/buildroot/com_reencryptor $(COM_SA_RESULT)/rpmtop/BUILD \
                $(COM_SA_RESULT)/rpmtop/RPMS/$(ARCHITECTURE)
	COM_REENCRYPT_PARTICIPANT=$(COM_REENCRYPT_PARTICIPANT) REENCRYPTION_UTILS=$(REENCRYPTION_UTILS) \
	$(RPMBUILD) --target $(ARCHITECTURE) --quiet -bb \
                --buildroot $(COM_SA_RESULT)/buildroot/com_reencryptor \
                --define='_prefix $(RPMBUILD_PREFIX_OPT)'\
                --define='_reencryptparticipantname $(COM_RENCRYPT_PARTICIPANT_RPM_NAME_TAG)'\
                --define='_$(COMPONENT_NAME)saver $(RPM_VERSION_TAG)'\
                --define='_$(COMPONENT_NAME)sarel $(SLES_RPM_RELEASE_TAG)'\
                --define='_comsaname $(SLES_RPM_NAME_TAG)'\
                --define='_topdir $(COM_SA_RESULT)/rpmtop' \
                $(REENCRYPTION_UTILS)/$(COMPONENT_NAME)_reencrypt_participant.spec > /dev/null
	cp $(COM_SA_RESULT)/rpmtop/RPMS/$(ARCHITECTURE)/$(REENCRYPTION_RPM) $(COM_SA_RESULT)
	cp $(COM_SA_RESULT)/$(REENCRYPTION_RPM) $(COMSA_RELEASE)
	@$(ECHO) Built $@

ifneq ($(ENV_TYPE), Lm)
rhel_rpm: $(COM_SA_RESULT)/$(RHEL_RPM)
endif

$(COM_SA_RESULT)/$(RHEL_RPM): $(SPEC_FILE_LOCATION)/$(COMPONENT_NAME)_sa.spec $(COM_SA_SO) $(PMT_SA_SO) $(TRACE_PROBE_SO)
	@rm -rf $(COM_SA_RESULT)/buildroot $(COM_SA_RESULT)/rpmtop
	@mkdir -p $(COM_SA_RESULT)/buildroot $(COM_SA_RESULT)/rpmtop/BUILD \
           $(COM_SA_RESULT)/rpmtop/RPMS/$(ARCHITECTURE)
	COM_SA_SO=$(COM_SA_SO) PMT_SA_SO=$(PMT_SA_SO) TRACE_PROBE_SO=$(TRACE_PROBE_SO) SDPSRC=$(SDPSRC) \
	$(RPMBUILD) --target $(ARCHITECTURE) --quiet -bb \
                --buildroot $(COM_SA_RESULT)/buildroot \
                --define='_prefix $(RPMBUILD_PREFIX_OPT)'\
                --define='_$(COMPONENT_NAME)saname $(RHEL_RPM_NAME_TAG)'\
                --define='_$(COMPONENT_NAME)saver $(RPM_VERSION_TAG)'\
                --define='_$(COMPONENT_NAME)sarel $(RHEL_RPM_RELEASE_TAG)'\
                --define='_mafname $(RHEL_MAF_RPM_NAME)'\
                --define='_comname $(RHEL_COM_RPM_NAME)'\
                --define='_topdir $(COM_SA_RESULT)/rpmtop' \
                --define='__disable_pmt_sa $(DISABLE_PMT_SA)' \
                $(SPEC_FILE_LOCATION)/$(COMPONENT_NAME)_sa.spec > /dev/null
	cp $(COM_SA_RESULT)/rpmtop/RPMS/$(ARCHITECTURE)/$(RHEL_RPM) $(COM_SA_RESULT)
	cp $(COM_SA_RESULT)/$(RHEL_RPM) $(COMSA_RELEASE)
	@$(ECHO) Built $@



$(COM_SA_SO): $(COM_SA_OBJS)
	$(LD) -o $@ -shared $(COM_SA_OBJS) $(CSA_LDFLAGS)

ifeq ($(DISABLE_PMT_SA),NO)

$(PMT_SA_SO): $(PMT_SA_OBJS)
	$(LD) -o $@ -shared $(PMT_SA_OBJS) $(CSA_LDFLAGS)

endif

ifeq ($(DISABLE_COM_REENCRYPTION),NO)

$(COM_REENCRYPT_PARTICIPANT): $(REENCRYPT_OBJS)
	$(LD) -o $@ $(REENCRYPT_OBJS) $(CSA_REENCRYPT_LDFLAGS)

endif

ifneq ( $(ENV_TYPE),Lm)
$(TRACE_PROBE_SO): $(TRACEPROBE_OBJS)
	$(LD) -o $@ -shared $(TRACEPROBE_OBJS)
endif


$(COM_SA_RESULT):
	mkdir -p $(COM_SA_RESULT)

clean:
	rm -rf $(COM_SA_RESULT) \
	$(SLES_CBA_SA_CXPA)/obj.files.tgz
	rm -f $(COMSA_RELEASE)/*.rpm
	rm -rf $(RHEL_CBA_SA_CXPA)/obj.files.tgz \
	rm -f $(COMSA_RELEASE)/cxp_archive/*

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(MAKECMDGOALS),dist)
-include $(DEPS)
endif
endif
