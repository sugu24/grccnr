SRCS=$(wildcard *.gc)
ASMS=$(SRCS:.gc=.s)
OBJS=$(ASMS:.s=.o)
COM=../grccnr

cinter: $(OBJS) 
	$(CC) -o cinter $(OBJS)

$(OBJS): $(ASMS)
	$(CC) -o $@ -c $*.s

$(ASMS): $(SRCS) cinter.gh ../grccnr
	$(COM) $*.gc > $@

clean:
	rm cinter *.o *.s

test: cinter
	./test.sh

.PHONY:
	test clean