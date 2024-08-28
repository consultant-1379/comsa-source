################################################################################
#                          Definition Section
################################################################################
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


#
# Name directories to iterate over
# -----------------------------------------------------
#
SUB_DIRS =\
	src\
	test\
	doc

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
