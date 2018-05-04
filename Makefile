.PHONY: all
all: preBuild panorama

CC=gcc
MKDIR=mkdir
CPPFLAG=-g -Wall -Wextra -fPIC
LINKFLAG+=-shared

# debug info
CPPFLAG+=-DDEBUG_FUNC

# 是否支持矫正
#CPPFLAG+=-DUNDISTORT_SUPPORT

# 是否采用特征点匹配
#CPPFLAG+=-DFEATURE_BASE

PROJECT_ROOT = $(PWD)
INCLUDE_PATH=-I${PROJECT_ROOT}/include
LIB_FLAGS=-lm

DEPS = $(INCLUDE_PATH)/common.h

_OBJS :=
_OBJS += panorama_image.o
_OBJS += panorama_matrix.o
_OBJS += panorama_vector.o
_OBJS += panorama_utils.o
_OBJS += panorama_log.o
_OBJS += panorama_surf.o
_OBJS += panorama_features2d.o
_OBJS += panorama_features_match.o
_OBJS += panorama_stitch.o
_OBJS += panorama.o

LIBNAMEFORSHORT=panorama
LIBNAME=lib$(LIBNAMEFORSHORT).so
OBJDIR = $(PROJECT_ROOT)/obj
SRCDIR = $(PROJECT_ROOT)/src
LIBDIR = $(PROJECT_ROOT)/lib
LIBCONF = /etc/ld.so.conf.d/libpanorama.conf
OBJS = $(patsubst %, $(OBJDIR)/%, $(_OBJS))


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

######## test functions #####
_TEST_OBJS = main.o
TESTNAME = pn
TESTDIR = $(PROJECT_ROOT)/test
TEST_OBJS = $(patsubst %, $(OBJDIR)/%, $(_TEST_OBJS))

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
