CXXFLAGS = -MMD -Wall -g -I.. -I/usr/X11R6/include
LDFLAGS = -shared

SRCS := $(wildcard *.cc)
OBJS := $(SRCS:.cc=.o)
TARGETS := $(SRCS:.cc=.so)

all: $(TARGETS)

JPEGViewer.so: JPEGViewer.o
	$(CXX) $(LDFLAGS) -Llibjpeg -o JPEGViewer.so JPEGViewer.o -ljpeg

JPEGViewer.o: JPEGViewer.cc
	$(CXX) $(CXXFLAGS) -Ilibjpeg -c -o JPEGViewer.o JPEGViewer.cc

PNGViewer.so: PNGViewer.o
	$(CXX) $(LDFLAGS) -Llibpng -Lzlib -o PNGViewer.so PNGViewer.o -lpng -lz

PNGViewer.o: PNGViewer.cc
	$(CXX) $(CXXFLAGS) -Ilibpng -Izlib -c -o PNGViewer.o PNGViewer.cc

%.so: %.o
	$(CXX) $(LDFLAGS) -o $@ $<

clean: FORCE
	rm -f $(OBJS) $(TARGETS) $(SRCS:.cc=.d) core

FORCE:

-include $(SRCS:.cc=.d)
