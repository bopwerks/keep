SHELL = /bin/sh
YFLAGS = -d

keep: keep.o lex.o
lex.o: y.tab.h
keep.o: keep.y

.PHONY: clean
clean:
	rm -f keep *.o
