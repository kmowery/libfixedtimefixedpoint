SHELL := /bin/bash
CC := gcc
OPTFLAGS := -O1 -fno-aggressive-loop-optimizations
CFLAGS := $(OPTFLAGS) -std=c99 -Wall -Werror -Wno-unused-function -Wno-strict-aliasing -fPIC
LDFLAGS := -lcmocka -lm -lftfp

LD_LIBRARY_PATH=.

progs             := test perf_test generate_test_helper
libs              := libftfp.so
ftfp_src          := ftfp.c autogen.c internal.c cordic.c power.c debug.c
ftfp_inc          := ftfp.h internal.h base.h lut.h
ftfp_obj          := $(ftfp_src:.c=.o)
ftfp_pre          := $(ftfp_src:.c=.pre)

autogens          := base.h lut.h autogen.c base.pyc

test_ftfp_src     := test.c
test_ftfp_obj     := $(test_ftfp_src:.c=.o)
test_ftfp_pre     := $(test_ftfp_src:.c=.pre)

perf_ftfp_src     := perf_test.c
perf_ftfp_obj     := $(perf_ftfp_src:.c=.o)
perf_ftfp_pre     := $(perf_ftfp_src:.c=.pre)

gen_test_src     := generate_test_helper.c
gen_test_obj     := $(gen_test_src:.c=.o)
gen_test_pre     := $(gen_test_src:.c=.pre)

.PHONY: all clean depend alltest
all: $(libs)

base.h : generate_base.py
	python3 generate_base.py --file base.h
base.py : generate_base.py
	python3 generate_base.py --pyfile base.py

autogen.c : generate_print.py base.py
	python3 generate_print.py --file autogen.c
lut.h : generate_base.py
	python3 generate_base.py --lutfile lut.h

%.o: %.c ${ftfp_inc} Makefile
	$(CC) -c -o $@ $(CFLAGS) $<

libftfp.so: $(ftfp_obj)
	$(CC) ${CFLAGS} -shared -o $@ $+

perf_test: $(perf_ftfp_obj) $(libs)
	$(CC) -lftfp -L . -o $@ $(CFLAGS) $<

test: $(test_ftfp_obj) $(libs)
	$(CC) -L . ${CFLAGS} -o $@ $< ${LDFLAGS}

generate_test_helper: $(gen_test_obj) $(libs)
	$(CC) -L . ${CFLAGS} -o $@ $< ${LDFLAGS}

pre: $(test_ftfp_pre) $(ftfp_pre) $(perf_ftfp_pre) $(gen_test_pre)

%.pre: %.c Makefile
	$(CC) -c -E -o $@ $(CFLAGS) $<

clean:
	$(RM) -r $(progs) $(libs) $(ftfp_obj) $(test_ftfp_obj) $(test_ftfp_pre) ${perf_ftfp_obj} ${perf_ftfp_pre} ${gen_test_obj} ${gen_test_pre} ${autogens}

run_tests:
	set -x ; \
	number=1 ; while [[ $$number -le 61  ]] ; do \
		echo "Testing" $$number "int bits..." && make clean && python -B generate_base.py --file base.h --pyfile base.py --intbits $$number && make test && LD_LIBRARY_PATH=. ./test || exit 1; \
		((number = number + 1)) ; \
	done

run_generate_test_helper:
	set -x ; \
	echo "#ifndef TEST_HELPER_H" > test_helper.h ; \
	echo "#define TEST_HELPER_H" >> test_helper.h ; \
	number=1 ; while [[ $$number -le 61  ]] ; do \
		make clean && python -B generate_base.py --file base.h --pyfile base.py --intbits $$number && make generate_test_helper && ./generate_test_helper; \
		((number = number + 1)) ; \
	done ; \
	echo >> test_helper.h ; \
	echo "#endif" >> test_helper.h ;
