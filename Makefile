.PHONY: all
all: preBuild panorama

CC=gcc
MKDIR=mkdir
CPPFLAG=-g -Wall -Wextra

#debug info
CPPFLAG+=-DDEBUG_FUNC

PROJECT_ROOT = $(PWD)
INCLUDE_PATH=-I${PROJECT_ROOT}/include
LIB_FLAGS=-lm

DEPS = $(INCLUDE_PATH)/common.h

_OBJS :=
_OBJS += main.o
_OBJS += features2d.o
_OBJS += imgcodec.o
_OBJS += matrix.o
_OBJS += vector.o
_OBJS += utils.o
_OBJS += log.o
_OBJS += surf.o
_OBJS += features_match.o
_OBJS += stitch.o
_OBJS += panorama.o

APPNAME=pn
OBJDIR = $(PROJECT_ROOT)/obj
SRCDIR = $(PROJECT_ROOT)/src
OBJS = $(patsubst %, $(OBJDIR)/%, $(_OBJS))

.PHONY: preBuild
preBuild:
	$(MKDIR) -p $(OBJDIR)

# generate the final target
panorama: $(OBJS)
	$(CC) -o $(APPNAME)  $(INCLUDE_PATH) $(OBJS) $(LIB_FLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) -o $@ -c $< $(CPPFLAG) $(INCLUDE_PATH) $(LIB_FLAGS)

.PHONY: clean
clean:
	find $(PROJECT_ROOT) -name *.o | xargs rm -rf
	rmdir $(OBJDIR)
	rm $(APPNAME)
