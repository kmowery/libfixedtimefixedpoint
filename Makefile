CC := gcc
CFLAGS := -O3 -std=gnu99 -Wall -Werror
LDFLAGS := -lcmocka

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
