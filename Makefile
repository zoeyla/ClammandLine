CC=gcc
CFLAGS=-Wall -Werror -O1 -g
PTHREADSLIB=-lpthread

ALL_SAMPLES=argv exectest exectest_with_args files forktest getanumber norace pipe pthreads_test race race_with_lock redirect shell208 signaltest

all: $(ALL_SAMPLES)

pthreads_test: pthreads_test.c
	$(CC) $(CFLAGS) -o $@ $< $(PTHREADSLIB)

exectest: exectest.c getanumber
	$(CC) $(CFLAGS) -o $@ $<

%: %.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(ALL_SAMPLES)

