#
#
# Makefile for building documentation
#
#

.PHONY: doc all

all: pdf

doc: pdf

pdf: Doxyfile
	doxygen
	cd GeneratedDocs/latex && make >/dev/null 2>&1

clean:
	rm -fr GeneratedDocs
