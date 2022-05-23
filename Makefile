CFLAGS=-std=c11 -g -static
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

grccnr: $(OBJS)
	$(CC) -o grccnr $(OBJS) $(LDFLAGS)

$(OBJS): grccnr.h

test: grccnr
	./test.sh

clean:
	rm -f grccnr *.o *~ temp*

.PHONY: test clean
