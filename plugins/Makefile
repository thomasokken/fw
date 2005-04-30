###############################################################################
# Fractal Wizard -- a free fractal renderer for Linux
# Copyright (C) 1987-2005  Thomas Okken
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 2,
# as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
###############################################################################

CXXFLAGS = -MMD -Wall -g -I..
LDFLAGS = -shared

SRCS := $(wildcard *.cc)
OBJS := $(SRCS:.cc=.o)
TARGETS := $(SRCS:.cc=.so)

all: $(OBJS) $(TARGETS)

%.so: %.o
	$(CXX) $(LDFLAGS) -o $@ $<

clean: FORCE
	rm -f $(OBJS) $(TARGETS) $(SRCS:.cc=.d) core.*

FORCE:

-include $(SRCS:.cc=.d)
