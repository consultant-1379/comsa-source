#
# Makefile for COM REENCRYPT PARTICIPANT
#
# Copyright (C) 2018 Ericsson AB
#
################################################################################
#                          Definition Section
################################################################################
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
$(warning "CONFIG must be set to 'DEBUG' or 'RELEASE' when building REENCRYPITON PARTICIPANT!")
$(error "Build COM REENCRYPT PARTICIPANT with 'make CONFIG=[DEBUG|RELEASE]'")
endif
endif


#
# Name directories to iterate over
# -----------------------------------------------------
#
SUB_DIRS =\
	src

DIRS = $(SUB_DIRS)

#
#  Set the make top directory
ifeq ($(MAKE_TOP),)
  MAKE_TOP = $(CURDIR)
endif

################################################################################
#                              Rule Section
################################################################################
#
# Declaration of phony targets i.e. a phony target will always be invoked
# regardless whether it exists a file or directory with the same name as
# the 'target'.
#
.PHONY : $(DIRS)

#
# Iterate over all subdirectories listed
# in the DIRS macro and invoke Make on the directory.
#
$(DIRS) :
	@if [ -d $@ ]; then\
	  cd $@;\
	  $(MAKE) $(MFLAGS) CONFIG=$(CONFIG) MAKE_TOP=$(MAKE_TOP) -f [Mm]akefile $(MAKECMDGOALS);\
	 fi

# The different make goals
#
all clean: $(DIRS)
