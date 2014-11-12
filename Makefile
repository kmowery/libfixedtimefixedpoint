CC := gcc
OPTFLAGS := -O -fforce-addr
CFLAGS := $(OPTFLAGS) -std=c99 -Wall -Werror -Wno-unused-function -Wno-strict-aliasing -fPIC
LDFLAGS := -lcmocka -lm

progs             := test perftest
libs		  := libftfp.so
ftfp_src          := ftfp.c autogen.c internal.c cordic.c power.c
ftfp_obj          := $(ftfp_src:.c=.o)
test_ftfp_src     := test.c $(ftfp_src)
test_ftfp_obj     := $(test_ftfp_src:.c=.o)
test_ftfp_pre     := $(test_ftfp_src:.c=.pre)


.PHONY: all clean depend
all: test libftfp.so perftest

autogen.c : generate_print.py
	python generate_print.py > autogen.c

%.o: %.c Makefile
	$(CC) -c -o $@ $(CFLAGS) $<

perftest: perf_test.c Makefile libftfp.so
	$(CC) -lftfp -L . -o $@ $(CFLAGS) $<

libftfp.so: $(ftfp_obj)
	$(CC) ${CFLAGS} -shared -o $@ $+

test: $(test_ftfp_obj)
	$(CC) ${CFLAGS} -o $@ $+ ${LDFLAGS}

pre: $(test_ftfp_pre)

%.pre: %.c Makefile
	$(CC) -c -E -o $@ $(CFLAGS) $<

clean:
	$(RM) -r $(progs) $(libs) $(test_ftfp_obj) $(test_ftfp_pre)
