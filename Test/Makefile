BASE := ..
LLVMCC := ./gcc
LLVMCXX := ./g++

OBJECTS := $(patsubst %.cc, ../bin/%, $(wildcard *.cc))
COBJECTS := $(patsubst %.c, ../bin/%, $(wildcard *.c))
PYOBJECT := $(patsubst %.py, ../bin/%, $(wildcard *.py))
include $(BASE)/common.mk

DEPS := $(join $(addsuffix ., $(dir $(OBJECTS))), $(addsuffix .d, $(notdir $(OBJECTS))))

CPPFLAGS += -I$(BASE) -I$(BASE)/API -I$(BASE)/Memory -I$(BASE)/Model -I$(BASE)/Schedule -I$(BASE)/Test
CFLAGS += -lpthread -lpmcheck
all: $(OBJECTS) $(COBJECTS) ../bin/run.sh $(PYOBJECT)

-include $(DEPS)

../bin/%: %.cc
	$(LLVMCXX) -MMD -MF $(@D)/.$(@F).d -o ../bin/$@ $< $(CPPFLAGS) -L$(BASE)/bin/ $(CFLAGS)

../bin/%: %.c
	$(LLVMCC) -MMD -MF $(@D)/.$(@F).d -o ../bin/$@ $< $(CPPFLAGS) -L$(BASE)/bin/ $(CFLAGS)

../bin/run.sh: run.sh
	cp run.sh ../bin/run.sh
../bin/%: %.py
	cp $< ../bin/$<

clean::
	rm -f $(OBJECTS) $(COBJECTS) $(DEPS) ../bin/run.sh
