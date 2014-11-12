CC := gcc
OPTFLAGS := -O -fforce-addr
CFLAGS := $(OPTFLAGS) -std=c99 -Wall -Werror -Wno-unused-function -Wno-strict-aliasing
LDFLAGS := -lcmocka -lm

progs             := test
test_ftfp_src     := test.c ftfp.c autogen.c internal.c cordic.c power.c
test_ftfp_obj     := $(test_ftfp_src:.c=.o)
test_ftfp_pre     := $(test_ftfp_src:.c=.pre)

.PHONY: all clean depend
all: test

autogen.c : generate_print.py
	python generate_print.py > autogen.c

%.o: %.c Makefile
	$(CC) -c -o $@ $(CFLAGS) $<

test: $(test_ftfp_obj)
	$(CC) ${CFLAGS} -o $@ $+ ${LDFLAGS}

pre: $(test_ftfp_pre)

%.pre: %.c Makefile
	$(CC) -c -E -o $@ $(CFLAGS) $<

clean:
	$(RM) -r $(progs) $(test_ftfp_obj) $(test_ftfp_pre)
