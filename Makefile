CXXFLAGS = -MMD -Wall -g -I/usr/X11R6/include
LDFLAGS = -L/usr/X11R6/lib -Xlinker --export-dynamic

# This works on Red Hat 7.3
LOADLIBES = -lXm -lXpm -ljpeg -lpng
# If that doesn't do it for you, try this:
# LOADLIBES = -lXm -lXp -lXt -lX11 -lXmu -lXpm -ldl -ljpeg -lpng -lz

# To link a "mostly" static version (everything linked statically,
# except libc, libm, libstdc++, and libdl):
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
