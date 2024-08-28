#
# Makefile for PMT-SA
#
# Copyright (C) 2011 Ericsson AB
#

TESTPMTSA_ROOT=$(CURDIR)

#
# Make sure a configuration is set, default to DEBUG
#
ifeq ($(CONFIG),)
CONFIG=DEBUG
$(warning "CONFIG was not set, setting it to 'DEBUG' per default.")
$(warning "Build PMT-SA with 'make CONFIG=[DEBUG|RELEASE]'")
endif

#
# Make sure the configuration is correct!
#
ifneq ($(CONFIG),DEBUG)
ifneq ($(CONFIG),RELEASE)
$(warning "CONFIG must be set to 'DEBUG' or 'RELEASE' when building PMT-SA!")
$(error "Build PMT-SA with 'make CONFIG=[DEBUG|RELEASE]'")
endif
endif

# Libraries required for building
LIBS = -lpmt_sa -lpthread -ldl -lxml2

# Compilation-flags depending on config built ...
PMTSA_CXXFLAGS = -pthread -Wall -Werror -fPIC
CXXFLAGS_DEBUG = -g3 -O0
CXXFLAGS_RELEASE = -g -O2
CXXFLAGS = $(PMTSA_CXXFLAGS) $(CXXFLAGS_$(CONFIG))

# Set output depending on configuration
OUTDIR_RELEASE = $(TESTPMTSA_ROOT)/Release
OUTDIR_DEBUG = $(TESTPMTSA_ROOT)/Debug

OUTDIR = $(OUTDIR_$(CONFIG))

PMTSALIB_DEBUG = ../src/Debug
PMTSALIB_RELEASE = ../src/Release

PMTSA_INCLUDES = ../src
COMSA_INCLUDES = ../../incl
COM_INCLUDES = ../../../dependencies/com_spi
SAF_INCLUDES = ../../../dependencies/core_mw_api/inc

CXXFILES := $(wildcard $(TESTPMTSA_ROOT)/src/*.cpp)
DEPFILES := $(patsubst %.cpp,%.d, $(addprefix $(OUTDIR)/, $(notdir $(CXXFILES))))
OBJFILES := $(patsubst %.cpp,%.o, $(addprefix $(OUTDIR)/, $(notdir $(CXXFILES))))

all: $(DEPFILES) $(OUTDIR)/PmtSaTest ComSaUnitTest

$(OUTDIR)/%.o: src/%.cpp
	@echo "--- Compiling $<"
	@$(CXX) -c -o $@ $< $(CXXFLAGS) -I$(PMTSA_INCLUDES) -I$(COMSA_INCLUDES) -I$(COM_INCLUDES) -I$(SAF_INCLUDES)

$(OUTDIR)/PmtSaTest: $(OBJFILES) $(PMTSALIB_$(CONFIG))/libpmt_sa.so
	@echo "--- Linking $@"
	@$(CXX) -o $@ $(OBJFILES) -L$(PMTSALIB_$(CONFIG)) $(LIBS)

ComSaUnitTest:
	@echo "--- Building Com-SA Unittest"
	@cd /vobs/com_sa/dev/src/unittest && $(MAKE) tests

clean:
	@echo "Removing intermediate objects, dependency-files and target lib"
	@/bin/rm -fr $(OUTDIR)

$(PMTSALIB_$(CONFIG))/libpmt_sa.so:
	@cd ../src && make CONFIG=$(CONFIG) all

$(OUTDIR)/%.d : src/%.cpp
	@echo "--- Generating dependencies for $<"
	@mkdir -p $(OUTDIR)
	@set -e; rm -f $@; \
	$(CXX) -c $(CXXFLAGS) -I$(PMTSA_INCLUDES) -I$(COMSA_INCLUDES) -I $(COM_INCLUDES) -I $(SAF_INCLUDES) -M $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,$(OUTDIR)/\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

# include generated dependency
ifneq ($(MAKECMDGOALS),clean)
-include $(patsubst %.cpp,%.d, $(addprefix $(OUTDIR)/, $(notdir $(CXXFILES))))
endif
