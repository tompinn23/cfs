CC ?= cc
CFLAGS = -I. -I../ -g

examples/ex_1: examples/ex_1.o cfs.o
	$(CC) -o $@ $^ $(LDFLAGS) 


.PHONY: clean
clean:
	rm -f *.o examples/*.o ex_1