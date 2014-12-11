SHELL := /bin/bash
CC := gcc
OPTFLAGS := -O
CFLAGS := $(OPTFLAGS) -std=c99 -Wall -Werror -Wno-unused-function -Wno-strict-aliasing -fPIC
LDFLAGS := -lcmocka -lm

progs             := test perf_test generate_print_results
libs              := libftfp.so
ftfp_src          := ftfp.c autogen.c internal.c cordic.c power.c
ftfp_inc          := ftfp.h internal.h base.h
ftfp_obj          := $(ftfp_src:.c=.o)
ftfp_pre          := $(ftfp_src:.c=.pre)

autogens          := base.h autogen.c base.pyc

test_ftfp_src     := test.c
test_ftfp_obj     := $(test_ftfp_src:.c=.o)
test_ftfp_pre     := $(test_ftfp_src:.c=.pre)

perf_ftfp_src     := perf_test.c
perf_ftfp_obj     := $(perf_ftfp_src:.c=.o)
perf_ftfp_pre     := $(perf_ftfp_src:.c=.pre)

gen_print_src     := generate_print_results.c
gen_print_obj     := $(gen_print_src:.c=.o)
gen_print_pre     := $(gen_print_src:.c=.pre)

.PHONY: all clean depend alltest
all: $(libs) $(progs)

base.h : generate_base.py
	python generate_base.py --file base.h
base.py : generate_base.py
	python generate_base.py --pyfile base.py

autogen.c : generate_print.py base.py
	python generate_print.py --file autogen.c

%.o: %.c ${ftfp_inc} Makefile
	$(CC) -c -o $@ $(CFLAGS) $<

libftfp.so: $(ftfp_obj)
	$(CC) ${CFLAGS} -shared -o $@ $+

perf_test: $(perf_ftfp_obj) $(libs)
	$(CC) -lftfp -L . -o $@ $(CFLAGS) $<

test: $(test_ftfp_obj) $(libs)
	$(CC) -lftfp -L . ${CFLAGS} -o $@ $< ${LDFLAGS}

generate_print_results: $(gen_print_obj) $(libs)
	$(CC) -lftfp -L . ${CFLAGS} -o $@ $< ${LDFLAGS}

pre: $(test_ftfp_pre) $(ftfp_pre) $(perf_ftfp_pre)

%.pre: %.c Makefile
	$(CC) -c -E -o $@ $(CFLAGS) $<

clean:
	$(RM) -r $(progs) $(libs) $(ftfp_obj) $(test_ftfp_obj) $(test_ftfp_pre) ${perf_ftfp_obj} ${perf_ftfp_pre} ${gen_print_obj} ${gen_print_pre} ${autogens}

alltests:
	set -x ; \
	number=1 ; while [[ $$number -le 61  ]] ; do \
		echo "Testing" $$number "int bits..." && make clean && python -B generate_base.py --file base.h --pyfile base.py --intbits $$number && make test && ./test || exit 1; \
		((number = number + 1)) ; \
	done

do_generate_print_results:
	set -x ; \
	echo "#ifndef TEST_PRINT_RESULTS_H" > test_print_results.h ; \
	echo "#define TEST_PRINT_RESULTS_H" >> test_print_results.h ; \
	number=1 ; while [[ $$number -le 61  ]] ; do \
		make clean && python -B generate_base.py --file base.h --pyfile base.py --intbits $$number && make generate_print_results && ./generate_print_results; \
		((number = number + 1)) ; \
	done ; \
	echo >> test_print_results.h ; \
	echo "#endif" >> test_print_results.h ;
