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
