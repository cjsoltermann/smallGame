all:
	gcc -ggdb3 -Wall -lncurses main.c game.c

run:
	./a.out

.PHONY: run
