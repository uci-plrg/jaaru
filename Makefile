include common.mk

PHONY += directories
MKDIR_P = mkdir -p
OBJ_DIR = bin

CPP_SOURCES := $(wildcard *.cc) $(wildcard API/*.cc) $(wildcard Memory/*.cc) $(wildcard Scheduler/*.cc) $(wildcard Collection/*.cc) $(wildcard Model/*.cc) $(wildcard Utils/*.cc)

C_SOURCES := $(wildcard *.c) $(wildcard API/*.c) $(wildcard Memory/*.c) $(wildcard Scheduler/*.c) $(wildcard Collection/*.c) $(wildcard Model/*.c) $(wildcard Utils/*.c)

J_SOURCES := $(wildcard *.java)

HEADERS := $(wildcard *.h) $(wildcard API/*.h) $(wildcard Memory/*.h) $(wildcard Scheduler/*.h) $(wildcard Collection/*.h) $(wildcard Model/*.h) $(wildcard Utils/*.h)


OBJECTS := $(CPP_SOURCES:%.cc=$(OBJ_DIR)/%.o) $(C_SOURCES:%.c=$(OBJ_DIR)/%.o)

J_OBJECTS := $(J_SOURCES:%.java=$(OBJ_DIR)/%.class)

CXXFLAGS := -std=c++1y -pthread
CFLAGS += -I. -IAPI -IMemory -IScheduler -ICollection -IModel -IUtils
LDFLAGS := -ldl -lrt -rdynamic -lpthread -g
SHARED := -shared

# Mac OSX options
ifeq ($(UNAME), Darwin)
LDFLAGS := -ldl
SHARED := -Wl,-undefined,dynamic_lookup -dynamiclib
endif

MARKDOWN := ../docs/Markdown/Markdown.pl

all: directories ${OBJ_DIR}/$(LIB_SO)

directories: ${OBJ_DIR}

${OBJ_DIR}:
	${MKDIR_P} ${OBJ_DIR}
	${MKDIR_P} ${OBJ_DIR}/API
	${MKDIR_P} ${OBJ_DIR}/Utils
	${MKDIR_P} ${OBJ_DIR}/Memory
	${MKDIR_P} ${OBJ_DIR}/Scheduler
	${MKDIR_P} ${OBJ_DIR}/Collection
	${MKDIR_P} ${OBJ_DIR}/Model

debug: CFLAGS += -DCONFIG_DEBUG
debug: all

test: all
	make -C Test

PHONY += docs
docs: $(C_SOURCES) $(HEADERS)
	doxygen

#Special flag for compiling malloc.c
${OBJ_DIR}/Memory/malloc.o: Memory/malloc.c
	$(CC) -fPIC -c $< -o $@ -DMSPACES -DONLY_MSPACES -DHAVE_MMAP=1 $(CFLAGS) -Wno-unused-variable

${OBJ_DIR}/$(LIB_SO): $(OBJECTS)
	$(CXX) -g $(SHARED) -o ${OBJ_DIR}/$(LIB_SO) $+ $(LDFLAGS)

${OBJ_DIR}/%.o: %.cc
	$(CXX) -fPIC -c $< -o $@ $(CFLAGS) $(CPPFLAGS) -Wno-unused-variable

${OBJ_DIR}/%.o: %.c
	$(CC) -fPIC -c $< -o $@ $(CFLAGS) -Wno-unused-variable

#javaapi: $(J_OBJECTS)

#${OBJ_DIR}/%.class: %.java
#	$(JAVAC) -d ${OBJ_DIR} $<

-include $(OBJECTS:%=$OBJ_DIR/.%.d)

PHONY += clean
clean:
	rm -f *.o *.so
	rm -rf $(OBJ_DIR)

PHONY += mrclean
mrclean: clean
	rm -rf ../docs

PHONY += tags
tags:
	ctags -R

tabbing:
	uncrustify -c C.cfg --no-backup *.cc */*.cc
	uncrustify -c C.cfg --no-backup *.h */*.h

wc:
	wc */*.cc */*.h *.cc *.h */*/*.cc */*/*.h

.PHONY: $(PHONY)
