CXXFLAGS = -MMD -Wall -g -I.. -Ilibjpeg -I/usr/X11R6/include
LDFLAGS = -shared -Llibjpeg

SRCS := $(wildcard *.cc)
OBJS := $(SRCS:.cc=.o)
TARGETS := $(SRCS:.cc=.so)

all: $(TARGETS)

JPEGViewer.so: JPEGViewer.o
	$(CXX) $(LDFLAGS) -o JPEGViewer.so JPEGViewer.o -ljpeg

%.so: %.o
	$(CXX) $(LDFLAGS) -o $@ $<

clean: FORCE
	rm -f $(OBJS) $(TARGETS) $(SRCS:.cc=.d) core

FORCE:

-include $(SRCS:.cc=.d)
