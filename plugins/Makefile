CXXFLAGS = -MMD -Wall -g -I.. -I/usr/X11R6/include
LDFLAGS = -shared

SRCS := $(wildcard *.cc)
OBJS := $(SRCS:.cc=.o)
TARGETS := $(SRCS:.cc=.so)

all: $(OBJS) $(TARGETS)

JPEGViewer.so: JPEGViewer.o
	$(CXX) $(LDFLAGS) -o JPEGViewer.so JPEGViewer.o -ljpeg

PNGViewer.so: PNGViewer.o
	$(CXX) $(LDFLAGS) -o PNGViewer.so PNGViewer.o -lpng -lz

%.so: %.o
	$(CXX) $(LDFLAGS) -o $@ $<

clean: FORCE
	rm -f $(OBJS) $(TARGETS) $(SRCS:.cc=.d) core.*

FORCE:

-include $(SRCS:.cc=.d)
