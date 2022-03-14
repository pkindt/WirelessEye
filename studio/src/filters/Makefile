.PHONY: all clean

CFIS := $(patsubst %.c, %.cfi, $(wildcard *.c))

.c.o:
	gcc -c $< -g -o $@ 

%.cfi: %.o
	gcc -shared $*.o -o $@

all: $(CFIS)

clean:
	rm *.cfi
