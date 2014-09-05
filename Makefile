CC := gcc
OPTFLAGS := -O -fforce-addr
CFLAGS := $(OPTFLAGS) -std=c99 -Wall -Werror -Wno-unused-function -Wno-strict-aliasing
LDFLAGS := -lcmocka -lm

progs             := test
test_ftfp_src     := test.c ftfp.c
test_ftfp_obj     := $(test_ftfp_src:.c=.o)

.PHONY: all clean depend
all: test

%.o: %.c Makefile
	$(CC) -c -o $@ $(CFLAGS) $<

test: $(test_ftfp_obj)
	$(CC) ${CFLAGS} -o $@ $+ ${LDFLAGS}

clean:
	$(RM) -r $(progs) $(test_ftfp_obj)
