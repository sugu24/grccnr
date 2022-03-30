CFLAGS=-std=c11 -g -static

grccnr: grccnr.c

test: grccnr
	./test.sh

clean:
	rm -f grccnr *.o *~ temp*

.PHONY: test clean
