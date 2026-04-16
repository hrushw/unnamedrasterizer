CC=gcc
# warn about failure to inline functions
# warn when using variable-length arrays
# warn about all cases of overflow
WFLAGS=-Wall -Wextra -Wpedantic -Wvla -Winline -Wstrict-overflow=5 -Wshift-overflow=2
# enforce 2s complement signed arithmetic
CFLAGS=-fwrapv -O3
LFLAGS=-lX11 -lXext

test00: main.c test00.c
	$(CC) $(CFLAGS) $(WFLAGS) $(LFLAGS) -o $@ $@.c

test01: main.c test01.c
	$(CC) $(CFLAGS) $(WFLAGS) $(LFLAGS) -lm -o $@ $@.c

clean:
	rm -f test00 test01 *.o *.ppm *.png

test00_run: test00
	./test_run.sh $<

test01_run: test01
	./test_run.sh $<

run: test00_run
