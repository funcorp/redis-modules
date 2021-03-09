# find the OS
uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')

# Compile flags for linux / osx
ifeq ($(uname_S),Linux)
	SHOBJ_LDFLAGS ?= -shared
else
    SHOBJ_CFLAGS ?= -dynamic
	SHOBJ_LDFLAGS ?= -bundle -undefined dynamic_lookup -macosx_version_min 10.14 -lSystem
endif

CFLAGS = -I../ -fno-common -W -Wall -g -ggdb -std=c11 -O2

.SUFFIXES: .c .so .xo .o

all: zrevmerge.so

.c.xo:
	$(CC) $(CFLAGS) $(SHOBJ_CFLAGS) -fPIC -c $< -o $@

zrevmerge.xo: common.h maxheap.h thpool.h

zrevmerge.so: zrevmerge.xo maxheap.xo thpool.xo
	$(LD) -o $@ $^ $(LDFLAGS) $(SHOBJ_LDFLAGS) -lpthread -lc

test: test-maxheap

test-maxheap: test_maxheap.xo maxheap.xo
	$(CC) -Wall -o $@ $^ -O0
	@(sh -c ./$@)
	rm ./$@
.PHONY: test-maxheap

clean:
	rm -rf *.xo *.so test
