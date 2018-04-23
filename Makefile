.PHONY: all
all: preBuild panorama

CC=gcc
MKDIR=mkdir
CPPFLAG=-g -Wall -Wextra -fPIC
LINKFLAG+=-shared

#debug info
CPPFLAG+=-DDEBUG_FUNC
#CPPFLAG+=-DFEATURE_BASE

PROJECT_ROOT = $(PWD)
INCLUDE_PATH=-I${PROJECT_ROOT}/include
LIB_FLAGS=-lm

DEPS = $(INCLUDE_PATH)/common.h

_OBJS :=
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

_TEST_OBJS = main.o
TESTNAME = pn

LIBNAMEFORSHORT=panorama
LIBNAME=lib$(LIBNAMEFORSHORT).so
OBJDIR = $(PROJECT_ROOT)/obj
SRCDIR = $(PROJECT_ROOT)/src
LIBDIR = $(PROJECT_ROOT)/lib
TESTDIR = $(PROJECT_ROOT)/test
LIBCONF = /etc/ld.so.conf.d/libpanorama.conf
OBJS = $(patsubst %, $(OBJDIR)/%, $(_OBJS))
TEST_OBJS = $(patsubst %, $(OBJDIR)/%, $(_TEST_OBJS))


.PHONY: preBuild
preBuild:
	$(MKDIR) -p $(OBJDIR)
	$(MKDIR) -p $(LIBDIR)

# generate the final target
panorama: $(OBJS)
	$(CC) -o $(LIBNAME)  $(LINKFLAG) $(INCLUDE_PATH) $(OBJS) $(LIB_FLAGS)
	mv $(LIBNAME) $(LIBDIR)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) -o $@ -c $< $(CPPFLAG) $(INCLUDE_PATH) $(LIB_FLAGS)

# test functions
test: preBuild panorama pn

pn: $(TEST_OBJS)
	$(CC) -o $(TESTNAME) $(TEST_OBJS) -L$(LIBDIR) -l$(LIBNAMEFORSHORT) $(LIB_FLAGS)

#$(OBJDIR)/$.o: $(TESTDIR)/$.c
$(OBJDIR)/main.o: $(TESTDIR)/main.c
	$(CC) -o $@ -c $< $(CPPFLAG) $(INCLUDE_PATH) $(LIB_FLAGS) -l$(LIBNAMEFORSHORT) 

.PHONY: clean
clean:
	find $(PROJECT_ROOT) -name *.o | xargs rm -rf
	rmdir $(OBJDIR)
	rm $(LIBDIR)/$(LIBNAME)
	rmdir $(LIBDIR)
	rm $(TESTNAME)
