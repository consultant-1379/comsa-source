#
# Makefile to build unit tests with the google test framework
# Use module add gcc/4.1.2 to build
#
# Modified: xduncao 2012-08-30 for new src directory structure
#
# Modified: eaparob 2013-10-10 removed unused code(~200 lines). Reason: COM SA OI Proxy code change affected the unittest, but in unittest the OI Proxy testing code never used.
# Modified: xdonngu 2014-03-05: UT for OI-Proxy, now only test for HS37161 - Make OI-Proxy for runtime mulltivalue structure (currently only struct with strings are supported).
# Modified: xdonngu 2014-03-18: Update gmock to version 1.7.0
# Modified: xadaleg 2014-07-04: Enable OI proxy files
# Modified: xadaleg 2014-08-02  MR35347 - increase DN length
# Modified: xadaleg 2014-12-16  MR37637 - Adapt IMM for replicated list service instead of CKPT
#
.SUFFIXES:
.SUFFIXES: .c .cc .cpp .cxx .o #implicit rules that we want to use

.PHONY: all tests coverage

UNITTESTSRC = $(CURDIR)
COM_SA_DEV = $(UNITTESTSRC)/../../../
GMOCK = $(UNITTESTSRC)/gmock-1.7.0
BINOUTDIR = $(UNITTESTSRC)/DepsAndObjs
DX_SYSROOT_X86_64 = $(COM_SA_DEV)/coremw-tools/lotc4.0_api/usr
COMSA_FOR_COREMW_DIR := comsa_for_coremw-apr9010555

CXX := g++
CC := gcc
# Build-tools. Usually the LSB Complience

ARCHITECTURE := x86_64

# related to code coverage
LCOV		= $(COM_SA_DEV)/tools/lcov/1.10-5.4/usr/bin/lcov
GENHTML		= $(COM_SA_DEV)/tools/lcov/1.10-5.4/usr/bin/genhtml
HTML_DIR	= html_ut
COV_RESULT	= comsa_ut_cov.info
COV_RES_INI	= comsa_ut_cov_ini.info
REMOVE_PTN	= *4.8.1/* *unittest/* *dependencies/* *usr/include/*  # results to remove: any gcc 4.8.1 stuff, any unittest and dependencies

LD := $(CXX)
COM_SA_DEP ?= $(COM_SA_DEV)/dependencies
LSB_SHAREDLIBPATH := $(COM_SA_DEP)/core_mw_api/lib/$(ARCHITECTURE)-suse-linux

COM_SA_SRC ?= $(COM_SA_DEV)/src

MO_DIR := $(COM_SA_SRC)/generic/motransaction
AA_DIR := $(COM_SA_SRC)/generic/accessmgm
MDF_DIR := $(COM_SA_SRC)/com_specific/mdf
PMT_SA_DIR := $(COM_SA_SRC)/generic/pmt_sa/src
NTF_DIR := $(COM_SA_SRC)/generic/ntf
LOG_DIR:= $(COM_SA_SRC)/generic/log
OIPROXY_DIR := $(COM_SA_SRC)/generic/motransaction/OIproxy
UTILS_DIR := $(COM_SA_SRC)/generic/utils
RLIST_DIR := $(COM_SA_SRC)/generic/rlist
AC_DIR := $(COM_SA_SRC)/generic/ac
TRACE_DIR := $(COM_SA_SRC)/generic/trace
REENCRYPT_DIR := $(COM_SA_SRC)/com_specific/com_reencryptor/src

includes := \
	$(GMOCK)/fused-src/gmock \
	$(GMOCK)/fused-src/gtest \
	$(GMOCK)/fused-src \
	$(AA_DIR)/utils \
	$(AA_DIR) \
	$(MDF_DIR) \
	$(MO_DIR) \
	$(PMT_SA_DIR) \
	$(UNITTESTSRC) \
	$(COM_SA_SRC)/generic/incl \
	$(COM_SA_SRC)/com_specific/incl \
	$(COM_SA_SRC)/com_specific/com_reencryptor/inc \
	$(COM_SA_DEP)/core_mw_api/inc \
	$(COM_SA_DEP)/com_spi \
	$(COM_SA_DEP)/maf_spi \
	$(COM_SA_DEP)/tracesdk/usr/include \
	$(DX_SYSROOT_X86_64)/include \
	$(DX_SYSROOT_X86_64)/include/libxml2

UTEST_SRCS := \
	$(UNITTESTSRC)/MockProcessUtil.cpp \
	$(UNITTESTSRC)/OamSAImmBridge_unittest.cc \
	$(UNITTESTSRC)/Ac_unittest.cc \
	$(UNITTESTSRC)/ComSALogService_unittest.cc \
	$(UNITTESTSRC)/saAis_dummy.cc \
	$(MDF_DIR)/ComSAMDF.cc \
	$(MO_DIR)/OamSAImmBridge.cc \
	$(MO_DIR)/OamSAKeyAttributeRepository.cc \
	$(MO_DIR)/OamSATranslator.cc \
	$(MO_DIR)/MOMRootRepository.cc \
	$(MO_DIR)/OamSACache.cc \
	$(MO_DIR)/TxContext.cc \
	$(MO_DIR)/OamSADataClasses.cc \
	$(MO_DIR)/OamSATransactionRepository.cc \
	$(MO_DIR)/trace.c \
	$(MO_DIR)/OamSATransactionalResource.cc \
	$(UNITTESTSRC)/pmt_sa_unittest.cc \
	$(UNITTESTSRC)/PmtSaCompUnittest.cc \
	$(UNITTESTSRC)/Utils_Unittest.cc \
	$(PMT_SA_DIR)/PmConsumerInterface.cxx \
	$(PMT_SA_DIR)/PmConsumer.cxx \
	$(PMT_SA_DIR)/PerfMgmtTransferSA.cxx \
	$(PMT_SA_DIR)/PmComComponent.cxx \
	$(PMT_SA_DIR)/PmEventHandler.cxx \
	$(PMT_SA_DIR)/PmInternalMessageStream.cxx \
	$(PMT_SA_DIR)/PmInternalMsg.cxx \
	$(PMT_SA_DIR)/PmRunnable.cxx \
	$(PMT_SA_DIR)/PmShowCounters.cxx \
	$(PMT_SA_DIR)/PmtSaTrace.cxx \
	$(PMT_SA_DIR)/PmSaSelectTimer.cxx \
	$(LOG_DIR)/ComSALogService.c \
	$(LOG_DIR)/LogEventProducer.cc \
	$(TRACE_DIR)/ComSATraceService.c \
	$(TRACE_DIR)/SaInternalTrace.c \
	$(UTILS_DIR)/OsCommand.cc \
	$(UTILS_DIR)/Utils.cc \
	$(UTILS_DIR)/saname_utils.c \
	$(GMOCK)/fused-src/gmock_main.cc \
	$(GMOCK)/fused-src/gmock-gtest-all.cc

UTEST_SRCS_NTF_RLIST := \
	$(UNITTESTSRC)/ComSANtf_unittest.cc \
	$(UNITTESTSRC)/ComSARlist_unittest.cc \
	$(UNITTESTSRC)/ComSARlist_imm_unittest.cc \
	$(UNITTESTSRC)/saAis_dummy.cc \
	$(UNITTESTSRC)/imm_utils_dummy.c \
	$(MO_DIR)/trace.c \
	$(NTF_DIR)/ComSANtf.c \
	$(NTF_DIR)/CmEventProducer.cc \
	$(RLIST_DIR)/ComSARlist.c \
	$(RLIST_DIR)/ComSARlist_imm.c \
	$(UTILS_DIR)/OsCommand.cc \
	$(UTILS_DIR)/Utils.cc \
	$(UTILS_DIR)/saname_utils.c \
	$(UTILS_DIR)/debug_log.cc \
	$(GMOCK)/fused-src/gmock_main.cc \
	$(GMOCK)/fused-src/gmock-gtest-all.cc

UTEST_SRCS_CMCACHE := \
	$(UNITTESTSRC)/CmCache_unittest.cc \
	$(UNITTESTSRC)/saAis_dummy.cc \
	$(NTF_DIR)/CmEventHandler.cc \
	$(MO_DIR)/trace.c \
	$(UTILS_DIR)/OsCommand.cc \
        $(UTILS_DIR)/Utils.cc \
	$(UTILS_DIR)/saname_utils.c \
	$(GMOCK)/fused-src/gmock_main.cc \
	$(GMOCK)/fused-src/gmock-gtest-all.cc

UTEST_SRCS_OIPROXY := \
	$(OIPROXY_DIR)/OamSAOIProxy.cc \
	$(OIPROXY_DIR)/OamSARegisterObjectImplementer.cc \
	$(OIPROXY_DIR)/OamSARegisterObjectUtils.cc \
	$(OIPROXY_DIR)/OamSAOIProxyStructCache.cc \
	$(MO_DIR)/trace.c \
	$(UTILS_DIR)/OsCommand.cc \
	$(UTILS_DIR)/Utils.cc \
	$(UTILS_DIR)/saname_utils.c \
	$(UNITTESTSRC)/imm_utils_dummy.c \
	$(UNITTESTSRC)/OamSAOiProxy_unittest.cc \
	$(UNITTESTSRC)/OamSACache_dummy.cpp \
	$(UNITTESTSRC)/OamSATranslator_dummy.cpp \
	$(UNITTESTSRC)/MockTransactionMasterIF2.cpp \
	$(UNITTESTSRC)/OIProxy_handleAdminOperation_unittest.cc \
	$(UNITTESTSRC)/ParameterVerifier.cpp \
	$(UNITTESTSRC)/StubIMM.cpp \
	$(UNITTESTSRC)/OamSARegisterObjectImplementer_unittest.cc \
	$(UNITTESTSRC)/saAis_dummy.cc \
	$(GMOCK)/fused-src/gmock_main.cc \
	$(GMOCK)/fused-src/gmock-gtest-all.cc

UTEST_SRCS_IMMCMD := \
	$(MO_DIR)/ImmCmd.cc \
	$(MO_DIR)/trace.c \
	$(UTILS_DIR)/OsCommand.cc \
	$(UTILS_DIR)/Utils.cc \
	$(UTILS_DIR)/saname_utils.c \
	$(UNITTESTSRC)/TxContext_dummy.cc \
	$(UNITTESTSRC)/saAis_dummy.cc \
	$(UNITTESTSRC)/OamSACache_dummy.cpp \
	$(UNITTESTSRC)/ImmCmd_unittest.cc \
	$(GMOCK)/fused-src/gmock_main.cc \
	$(GMOCK)/fused-src/gmock-gtest-all.cc

UTEST_SRCS_REENCRYPT := \
	$(UNITTESTSRC)/Reencryptor_CmwUtil_dummy.cc \
	$(REENCRYPT_DIR)/Trace.cc \
	$(REENCRYPT_DIR)/TimerSelect.cc \
	$(REENCRYPT_DIR)/ImmUtil.cc \
	$(REENCRYPT_DIR)/SecUtil.cc \
	$(REENCRYPT_DIR)/Reencryptor.cc \
	$(REENCRYPT_DIR)/FileReaderUtil.cc \
	$(UNITTESTSRC)/Reencryptor_unittest.cc \
	$(GMOCK)/fused-src/gmock_main.cc \
	$(GMOCK)/fused-src/gmock-gtest-all.cc

UTEST_SRCS_MOCK_SEC_CRYPTO := \
	$(UNITTESTSRC)/MockSecUtil.cc

#UTEST_SRCS_NTF_RLIST
UTESTNTF_RLIST_EXE		:= UnittestNTF_RList
UTESTNTF_RLIST_OBJS		:= $(UTESTNTF_RLIST_OBJS:%.cc=%.o)
UTESTNTF_RLIST_OBJS		:= $(UTESTNTF_RLIST_OBJS:%.c=%.o)

UTESTCMCACHE_EXE	:= UnittestCmCache

UTESTOIPROXY_EXE	:= UnittestOiProxy

UTESTIMMCMD_EXE		:= UnittestImmCmd

UTESTREENCRYPTOR_EXE	:= UnittestReencryptor

LIB_SECCRYPTO_SO := $(UNITTESTSRC)/libsec_crypto_api.so.1

VPATH = $(patsubst %,:%,$(dir $(UTEST_SRCS) $(UTEST_SRCS_NTF_RLIST) $(UTEST_SRCS_CMCACHE) $(UTEST_SRCS_OIPROXY) $(UTEST_SRCS_REENCRYPT)))

OBJFILES1 := $(patsubst %.cxx,%.o, $(addprefix $(BINOUTDIR)/, $(notdir $(UTEST_SRCS))))
OBJFILES1 := $(patsubst %.cpp,%.o, $(OBJFILES1))
OBJFILES1 := $(patsubst %.cc,%.o, $(OBJFILES1))
OBJFILES1 := $(patsubst %.c,%.o, $(OBJFILES1))

OBJFILES2 := $(patsubst %.cxx,%.o, $(addprefix $(BINOUTDIR)/, $(notdir $(UTEST_SRCS_NTF_RLIST))))
OBJFILES2 := $(patsubst %.cpp,%.o, $(OBJFILES2))
OBJFILES2 := $(patsubst %.cc,%.o, $(OBJFILES2))
OBJFILES2 := $(patsubst %.c,%.o, $(OBJFILES2))

OBJFILES3 := $(patsubst %.cxx,%.o, $(addprefix $(BINOUTDIR)/, $(notdir $(UTEST_SRCS_CMCACHE))))
OBJFILES3 := $(patsubst %.cpp,%.o, $(OBJFILES3))
OBJFILES3 := $(patsubst %.cc,%.o, $(OBJFILES3))
OBJFILES3 := $(patsubst %.c,%.o, $(OBJFILES3))

OBJFILES4 := $(patsubst %.cxx,%.o, $(addprefix $(BINOUTDIR)/, $(notdir $(UTEST_SRCS_OIPROXY))))
OBJFILES4 := $(patsubst %.cpp,%.o, $(OBJFILES4))
OBJFILES4 := $(patsubst %.cc,%.o, $(OBJFILES4))
OBJFILES4 := $(patsubst %.c,%.o, $(OBJFILES4))

OBJFILES5 := $(patsubst %.cxx,%.o, $(addprefix $(BINOUTDIR)/, $(notdir $(UTEST_SRCS_IMMCMD))))
OBJFILES5 := $(patsubst %.cpp,%.o, $(OBJFILES5))
OBJFILES5 := $(patsubst %.cc,%.o, $(OBJFILES5))
OBJFILES5 := $(patsubst %.c,%.o, $(OBJFILES5))

OBJFILES6 := $(patsubst %.cc,%.o, $(addprefix $(BINOUTDIR)/, $(notdir $(UTEST_SRCS_REENCRYPT))))
OBJFILES6 := $(patsubst %.c,%.o, $(OBJFILES6))

executable := Unittest

# CPPFLAGS, CFLAGS, CXXFLAGS and LDFLAGS are used by built-in implicit rules.
# Do not change the names of these variables.
#
# override allows user to override variables, that is:
# make CFLAGS=-03
# make CXXFLAGS=-Wno-error

common_compile_flags := -ggdb -fno-inline -pthread -DUNIT_TEST -ftest-coverage -fprofile-arcs -w -DSA_EXTENDED_NAME_SOURCE -DCOMSA_FOR_COREMW_DIR='"$(COMSA_FOR_COREMW_DIR)"' -DCC_NAME='"Com"'
override CPPFLAGS := $(addprefix -I, $(includes))
override CFLAGS := $(common_compile_flags) $(CFLAGS)
override CXXFLAGS := $(common_compile_flags) $(CXXFLAGS)
override LDFLAGS := -L$(DX_SYSROOT_X86_64)/lib64 -L$(UNITTESTSRC) -pthread -lxml2 -lrt -Wl,--allow-shlib-undefined,-rpath=$(UNITTESTSRC) -ldl $(LDFLAGS) -ftest-coverage -fprofile-arcs

coverage: CFLAGS += -DCODE_COVERAGE
coverage: CXXFLAGS += -DCODE_COVERAGE

coverage: CFLAGS   += -DCODE_COVERAGE
coverage: CXXFLAGS += -DCODE_COVERAGE

# Main targets
all: $(executable) $(UTESTNTF_RLIST_EXE) $(UTESTCMCACHE_EXE) $(UTESTOIPROXY_EXE) $(UTESTIMMCMD_EXE) $(UTESTREENCRYPTOR_EXE) $(LIB_SECCRYPTO_SO)

# Dependency rules
$(BINOUTDIR)/%.d : %.cxx
	@echo "--- Generating dependencies for $<"
	@mkdir -p $(BINOUTDIR)
	@set -e; rm -f $@; \
	$(CXX) -c $(CXXFLAGS) $(addprefix -I, $(includes)) -M $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,$(BINOUTDIR)/\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

$(BINOUTDIR)/%.d : %.cpp
	@echo "--- Generating dependencies for $<"
	@mkdir -p $(BINOUTDIR)
	@set -e; rm -f $@; \
	$(CXX) -c $(CXXFLAGS) $(addprefix -I, $(includes)) -M $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,$(BINOUTDIR)/\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

$(BINOUTDIR)/pmt_sa_unittest.d : pmt_sa_unittest.cc pmt_sa.mock
	@echo "--- Generating dependencies for $<"
	@mkdir -p $(BINOUTDIR)
	@set -e; rm -f $@; \
	$(CXX) -c $(CXXFLAGS) $(addprefix -I, $(includes)) -M $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,$(BINOUTDIR)/\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

$(BINOUTDIR)/%.d : %.cc $(GMOCK)/fused-src/gtest/gtest.h
	@echo "--- Generating dependencies for $<"
	@mkdir -p $(BINOUTDIR)
	@set -e; rm -f $@; \
	$(CXX) -c $(CXXFLAGS) $(addprefix -I, $(includes)) -M $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,$(BINOUTDIR)/\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

$(BINOUTDIR)/%.d : $(GMOCK)/fused-src/%.cc
	@echo "--- Generating dependencies for $<"
	@mkdir -p $(BINOUTDIR)
	@set -e; rm -f $@; \
	$(CXX) -c $(CXXFLAGS) $(addprefix -I, $(includes)) -M $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,$(BINOUTDIR)/\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

$(BINOUTDIR)/%.d : %.c
	@echo "--- Generating dependencies for $<"
	@mkdir -p $(BINOUTDIR)
	@set -e; rm -f $@; \
	$(CXX) -c $(CXXFLAGS) $(addprefix -I, $(includes)) -M $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,$(BINOUTDIR)/\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

$(executable): $(OBJFILES1)
	@echo "--- Linking $@"
	@$(CXX) -o $@ $^ $(LDFLAGS)

$(UTESTNTF_RLIST_EXE): $(OBJFILES2)
	@echo "--- Linking NTF and RList $@"
	@$(CXX) -o $@ $^ $(LDFLAGS)

$(UTESTCMCACHE_EXE): $(OBJFILES3)
	@echo "--- Linking CmCache $@"
	@$(CXX) -o $@ $^ $(LDFLAGS)

$(UTESTOIPROXY_EXE): $(OBJFILES4)
	@echo "--- Linking OiProxy $@"
	@$(CXX) -o $@ $^ $(LDFLAGS)

$(UTESTIMMCMD_EXE): $(OBJFILES5)
	@echo "--- Linking ImmCmd $@"
	@$(CXX) -o $@ $^ $(LDFLAGS)

$(UTESTREENCRYPTOR_EXE): $(OBJFILES6)
	@echo "--- Linking Reencryptor $@"
	@$(CXX) -o $@ $^ $(LDFLAGS)

$(LIB_SECCRYPTO_SO): $(UTEST_SRCS_MOCK_SEC_CRYPTO)
	@echo "--- Creating Shared Object $@"
	@$(CXX) -fPIC -shared $^ -o $@

# The gmock sources are located in the gmock zip file
# This construction serializes the unziping of the gtest files
$(GMOCK)/fused-src/gmock-gtest-all.cc \
$(GMOCK)/fused-src/gmock_main.cc \
$(GMOCK)/fused-src/gtest/gtest.h \
$(GMOCK)/fused-src/gmock/gmock.h \
pmt_sa.mock \
: $(GMOCK)/unzipped

$(GMOCK)/unzipped: $(UNITTESTSRC)/gmock-1.7.0.zip # ugly but it works
	@unzip -n $<                                  # unzip all files
	@find gmock-1.7.0 -print0 | xargs -0 touch # updates time stamps
	@touch $@                                  # creates 'phony' target

# c, c++ files etc are already covered by built-in implicit rules.
# Files ending in .cxx are not recognized though, so we must create
# the rule our selves
$(BINOUTDIR)/%.o: %.cxx
	@echo "--- Compiling with CXX $<"
	@$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

$(BINOUTDIR)/%.o: %.cpp
	@echo "--- Compiling with CXX $<"
	@$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

$(BINOUTDIR)/%.o: %.cc
	@echo "--- Compiling with CXX $<"
	@$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

$(BINOUTDIR)/%.o: %.c
	@echo "--- Compiling with CC $<"
	@$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

pmt_sa.mock: $(PMT_SA_DIR)/PmConsumerInterface.hxx
	@echo "--- Generating $@"
	@$(GMOCK)/scripts/generator/gmock_gen.py $< > $@

# cleans executable, object files, and dep files
.PHONY: clean

clean:
	@rm -rf $(executable) $(BINOUTDIR) $(MO_DIR_OI)/*.o $(MO_DIR)/*.o $(AA_DIR)/*.o $(AA_DIR)/implementor/*.o pmt_sa.mock gmock-1.7.0 $(HTML_DIR) $(COV_RESULT) $(COV_RES_INI) $(UTESTNTF_RLIST_EXE) $(UTESTCMCACHE_EXE) $(UTESTOIPROXY_EXE) $(UTESTIMMCMD_EXE) $(UTESTREENCRYPTOR_EXE)
	@rm -f $(LIB_SECCRYPTO_SO)

.NOTPARALLEL: tests

tests: all
	@echo "--- Running unittests"
	-@./runUnitTest.sh # ignoring any error return code

coverage: tests
	@echo "--- Code coverage post-processing"
	@$(LCOV) -d . -b . --capture --output-file $(COV_RES_INI)
	@$(LCOV) -r $(COV_RES_INI) $(REMOVE_PTN) --output-file $(COV_RESULT) # extract the data only for the interesting files
	@mkdir -p $(HTML_DIR)
	@$(GENHTML) -o $(HTML_DIR) $(COV_RESULT)

ifneq ($(MAKECMDGOALS),clean)
-include $(patsubst %.cxx,%.d, $(addprefix $(BINOUTDIR)/, $(notdir $(UTEST_SRCS))))
-include $(patsubst %.cpp,%.d, $(addprefix $(BINOUTDIR)/, $(notdir $(UTEST_SRCS))))
-include $(patsubst %.cc,%.d, $(addprefix $(BINOUTDIR)/, $(notdir $(UTEST_SRCS))))
-include $(patsubst %.c,%.d, $(addprefix $(BINOUTDIR)/, $(notdir $(UTEST_SRCS))))

endif
