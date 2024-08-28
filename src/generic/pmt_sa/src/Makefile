#
# Makefile for PMT-SA
#
# Copyright (C) 2011 Ericsson AB
#

PMTSA_ROOT=$(CURDIR)

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
LIBS = -lpthread -ldl

# Compilation-flags depending on config built ...
PMTSA_CXXFLAGS = -pthread -Wall -Werror -fPIC -mindirect-branch=thunk
CXXFLAGS_DEBUG = -g3 -O0
CXXFLAGS_RELEASE = -g -O2 -DNDEBUG=1
CXXFLAGS = $(PMTSA_CXXFLAGS) $(CXXFLAGS_$(CONFIG))
CXXLDFLAGS = -lSaLog

# Set output depending on configuration
OUTDIR_RELEASE = $(PMTSA_ROOT)/Release
OUTDIR_DEBUG = $(PMTSA_ROOT)/Debug

OUTDIR = $(OUTDIR_$(CONFIG))

COM_INCLUDES = ../../../dependencies/com_spi
SAF_INCLUDES = ../../../dependencies/core_mw_api/inc
COM_SA_INCLUDES = ../../incl

CXXFILES := $(wildcard $(PMTSA_ROOT)/*.cxx)
CFILES := ../../motransaction/trace.c
DEPFILES := $(patsubst %.cxx,%.d, $(addprefix $(OUTDIR)/, $(notdir $(CXXFILES))))
OBJFILES := $(patsubst %.cxx,%.o, $(addprefix $(OUTDIR)/, $(notdir $(CXXFILES))))
DEPFILESC := $(patsubst %.c,%.d, $(addprefix $(OUTDIR)/, $(notdir $(CFILES))))
OBJFILESC := $(patsubst %.c,%.o, $(addprefix $(OUTDIR)/, $(notdir $(CFILES))))

ALL_INCS = -I$(COM_INCLUDES) -I$(SAF_INCLUDES) -I$(COM_SA_INCLUDES)

all: $(DEPFILES) $(DEPFILESC) $(OUTDIR)/libpmt_sa.so

$(OUTDIR)/%.o: %.cxx
	@echo "--- Compiling $<"
	@$(CXX) -c -o $@ $< $(CXXFLAGS) $(ALL_INCS)

$(OUTDIR)/%.o: ../../motransaction/trace.c
	@echo "--- Compiling $<"
	@$(CC) -c -o $@ $< $(CXXFLAGS) $(ALL_INCS)

$(OUTDIR)/libpmt_sa.so: $(OBJFILES) $(OBJFILESC)
	@echo "--- Linking $@"
	@$(CXX) -shared -o $@ $(OBJFILES) $(OBJFILESC) $(LIBS) $(CXXLDFLAGS)

clean:
	@echo "Removing intermediate objects, dependency-files and target lib"
	@/bin/rm -fr $(OUTDIR)

$(OUTDIR)/%.d : %.cxx
	@echo "--- Generating dependencies for $<"
	@mkdir -p $(OUTDIR)
	@set -e; rm -f $@; \
	$(CXX) -c $(CXXFLAGS)  $(ALL_INCS) -M $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,$(OUTDIR)/\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

$(OUTDIR)/%.d : ../../motransaction/trace.c
	@echo "--- Generating dependencies for $<"
	@mkdir -p $(OUTDIR)
	@set -e; rm -f $@; \
	$(CC) -c $(CXXFLAGS)  $(ALL_INCS) -M $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,$(OUTDIR)/\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

# include generated dependency
ifneq ($(MAKECMDGOALS),clean)
-include $(patsubst %.cxx,%.d, $(addprefix $(OUTDIR)/, $(notdir $(CXXFILES))))
-include $(patsubst %.c,%.d, $(addprefix $(OUTDIR)/, $(notdir $(CFILES))))
endif
