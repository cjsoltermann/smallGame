all:
	gcc -ggdb3 -Wall -lncurses -std=c99 game.c

run:
	./a.out

.PHONY: run
