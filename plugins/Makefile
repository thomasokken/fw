CXXFLAGS = -MMD -Wall -g -I.. -I/usr/X11R6/include

SRCS := $(wildcard *.cc)
OBJS := $(SRCS:.cc=.o)
TARGETS := $(SRCS:.cc=.so)

all: $(TARGETS)

%.so: %.o
	$(CXX) $(CXXFLAGS) -shared -o $@ $<

clean: FORCE
	rm -f $(OBJS) $(TARGETS) $(SRCS:.cc=.d) core

FORCE:

-include $(SRCS:.cc=.d)
