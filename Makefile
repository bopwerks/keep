SHELL = /bin/sh
YFLAGS = -d
CFLAGS = -g

OBJ = \
	account.o \
	lex.o \
	main.o \
	track.o \
	util.o \
    keep.o \
    id.o

keep: $(OBJ)
lex.o: y.tab.h
keep.o: keep.y

.PHONY: clean
clean:
	rm -f keep $(OBJ) y.tab.h
