# Python interface Makefile
#
# Part of the Routino routing software.
#
# This file Copyright 2018, 2019 Andrew M. Bishop
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

# All configuration is in the top-level Makefile.conf

include ../Makefile.conf

# Programs

PYTHON=python3

SWIG=swig

# Compilation targets

PY_FILES=$(wildcard src/*.py)

C_FILES=$(wildcard src/*.c)
CC_FILES=$(wildcard src/*.cc)

SWIG_C=src/_router.c
SWIG_CC=src/_database.cc
SWIG_PY=src/router.py src/database.py

ifneq ($(HOST),MINGW)
  LIBROUTINO=../src/libroutino.so
else
  LIBROUTINO=../src/routino.dll
endif

BUILD_TIMESTAMP=build/.timestamp

# Check that we have Python3 and swig installed

HAVE_PYTHON=$(shell $(PYTHON) --version 2> /dev/null)

HAVE_SWIG=$(shell $(SWIG) -version 2> /dev/null)

ifeq ($(HAVE_PYTHON),)
  $(warning Python3 not installed - skipping Python module creation)
endif

ifeq ($(HAVE_SWIG),)
  $(warning Swig not installed - skipping Python module creation)
endif

########

all: $(and $(HAVE_SWIG),$(HAVE_PYTHON),all-if-python)

all-if-python: $(BUILD_TIMESTAMP)

########

$(BUILD_TIMESTAMP): $(SWIG_C) $(SWIG_CC) $(SWIG_PY) $(PY_FILES) $(C_FILES) $(CC_FILES) $(LIBROUTINO) setup.py
	@rm -f $@
	CFLAGS= LDFLAGS= $(PYTHON) setup.py build && touch $(BUILD_TIMESTAMP)

src/_router.c : src/router.i ../src/routino.h
	$(SWIG) -python -o $@ $<

src/router.py : src/_router.c
	@true # fake rule since src/router.py is created by the same rule as src/_router.c 

src/_database.cc : src/database.i src/database.hh
	$(SWIG) -c++ -python -o $@ $<

src/database.py : src/_database.cc
	@true # fake rule since src/database.py is created by the same rule as src/_database.cc 

src/%.o : src/%.c
	$(CC) -c $(CFLAGS) $< -o $@

src/%.o : src/%.cc
	$(CXX) -c $(CFLAGS) $< -o $@

$(LIBROUTINO):
	cd ../src && $(MAKE) all-lib

########

test: $(and $(HAVE_SWIG),$(HAVE_PYTHON),test-if-python)

test-if-python: $(BUILD_TIMESTAMP)
	cd test && $(MAKE) test

########

install: $(and $(HAVE_SWIG),$(HAVE_PYTHON),install-if-python)

install-if-python: all
	$(PYTHON) setup.py install --prefix $(DESTDIR)$(prefix)

########

clean: clean-local
	cd test && $(MAKE) $@

clean-local:
	rm -f *~
	rm -rf build
	rm -f $(SWIG_C)
	rm -f $(SWIG_CC)
	rm -f $(SWIG_PY)

########

distclean: distclean-local
	cd test && $(MAKE) $@

distclean-local: clean-local

########

.PHONY:: all test install clean distclean

.PHONY:: all-if-python test-if-python install-if-python

.PHONY:: clean-local distclean-local
