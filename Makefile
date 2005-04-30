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

CXXFLAGS = -MMD -Wall -g -I/usr/X11R6/include
LDFLAGS = -L/usr/X11R6/lib -Xlinker --export-dynamic

# This works on Red Hat 7.3
LOADLIBES = -lXm -lXpm -ljpeg -lpng
# If that doesn't do it for you, try this:
# LOADLIBES = -lXm -lXp -lXt -lX11 -lXmu -lXpm -ldl -ljpeg -lpng -lz

# To link a "mostly" static version (everything linked statically,
# except libc, libm, and libstdc++):
# LOADLIBES = -Xlinker -Bstatic -lXmu -lXm -lXp -lXt -lX11 -lXpm -ljpeg \
#             -lpng -lz -lSM -lICE -lXext -ldl -Xlinker -Bdynamic

SRCS := $(wildcard *.cc)
OBJS := $(SRCS:.cc=.o)

all: fw plugins

fw: $(OBJS)
	$(CXX) $(LDFLAGS) -o fw $(OBJS) $(LOADLIBES)

plugins: FORCE
	cd plugins && $(MAKE)

clean: FORCE
	rm -f $(OBJS) $(SRCS:.cc=.d) fw core.*
	cd plugins && $(MAKE) clean

FORCE:

-include $(SRCS:.cc=.d)
