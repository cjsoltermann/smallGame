all:
	gcc -ggdb3 -Wall -lncurses game.c

run:
	./a.out

.PHONY: run
