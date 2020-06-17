SHELL = /bin/sh
YFLAGS = -d
CFLAGS = -g

keep: keep.o lex.o account.o main.o track.o util.o
lex.o: y.tab.h
keep.o: keep.y

.PHONY: clean
clean:
	rm -f keep *.o y.tab.h
