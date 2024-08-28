#
# Makefile for COM REENCRYPT PARTICIPANT
#
# Copyright (C) 2018 Ericsson AB
#

REENCRYPT_ROOT=$(CURDIR)

#
# Make sure a configuration is set, default to DEBUG
#
ifeq ($(CONFIG),)
CONFIG=DEBUG
$(warning "CONFIG was not set, setting it to 'DEBUG' per default.")
$(warning "Build COM REENCRYPT PARTICIPANT with 'make CONFIG=[DEBUG|RELEASE]'")
endif

#
# Make sure the configuration is correct!
#
ifneq ($(CONFIG),DEBUG)
ifneq ($(CONFIG),RELEASE)
$(warning "CONFIG must be set to 'DEBUG' or 'RELEASE' when building COM REENCRYPT PARTICIPANT!")
$(error "Build COM REENECRYPT PARTICIPANT with 'make CONFIG=[DEBUG|RELEASE]'")
endif
endif

# Libraries required for building
LIBS = -lpthread -ldl -lrt

# Compilation-flags depending on config built ...
REENCRYPT_CXXFLAGS = -pthread -Wall -Werror -fPIC -mindirect-branch=thunk
CXXFLAGS_DEBUG = -g3 -O0
CXXFLAGS_RELEASE = -g -O2 -DNDEBUG=1
CXXFLAGS = $(REENCRYPT_CXXFLAGS) $(CXXFLAGS_$(CONFIG))

# Set output depending on configuration
OUTDIR_RELEASE = $(REENCRYPT_ROOT)/Release
OUTDIR_DEBUG = $(REENCRYPT_ROOT)/Debug

OUTDIR = $(OUTDIR_$(CONFIG))

SAF_INCLUDES = ../../../dependencies/core_mw_api/inc
COM_REENCRYPTOR_INCLUDES = inc

CXXFILES := $(wildcard $(REENCRYPT_ROOT)/*.cc)
OBJFILES := $(patsubst %.cc,%.o, $(addprefix $(OUTDIR)/, $(notdir $(CXXFILES))))

ALL_INCS = -I$(SAF_INCLUDES) -I$(COM_REENCRYPTOR_INCLUDES)

all: $(OUTDIR)/com-reencrypt-participant

$(OUTDIR)/%.o: %.cc
	@echo "--- Compiling $<"
	@$(CXX) -o $@ $< $(CXXFLAGS) $(ALL_INCS)

$(OUTDIR)/com-reencrypt-participant: $(OBJFILES)
	@echo "--- Linking $@"
	@$(CXX) -o $@ $(OBJFILES) $(LIBS)

clean:
	@echo "Removing intermediate objects, dependency-files and target lib"
	@/bin/rm -fr $(OUTDIR)

$(OUTDIR)/%.d : %.cc
	@echo "--- Generating dependencies for $<"
	@mkdir -p $(OUTDIR)
	@set -e; rm -f $@; \
	$(CXX) -c $(CXXFLAGS)  $(ALL_INCS) -M $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,$(OUTDIR)/\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

# TODO:include generated dependency
ifneq ($(MAKECMDGOALS),clean)
-include $(patsubst %.cc,%.d, $(addprefix $(OUTDIR)/, $(notdir $(CXXFILES))))
endif
