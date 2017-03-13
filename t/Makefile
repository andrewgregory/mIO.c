# the tap.c submodule must be checked out to build and run tests

CFLAGS += -Wall -Wextra -g -I..

PROVE ?= prove

TESTS = 01-sanity.t 90-smoke.t

all: tests

# 01-sanity.t: CFLAGS += -std=c99 -pedantic -Werror

%.t: %.c ../mio.c tap.c/tap.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) $< $(LDLIBS) -o $@

check: tests
	$(PROVE) $(PROVEFLAGS) $(TESTS)

gcov: CC = gcc
gcov: CFLAGS += -fprofile-arcs -ftest-coverage
gcov: check
	gcov $(TESTS)

gprof: CC = gcc
gprof: CFLAGS += -pg
gprof: check

tests: $(TESTS)

clean:
	$(RM) $(TESTS)
	$(RM) *.gcov *.gcda *.gcno gmon.out

.PHONY: all clean check gcov gprof tests
