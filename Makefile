CXXFLAGS = -MMD -Wall -g -I/usr/X11R6/include
LDFLAGS = -L/usr/X11R6/lib -rdynamic
LOADLIBES = -lXm -lXt -lX11 -lXmu -lXp -lXpm -ldl -ljpeg -lpng -lz

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
