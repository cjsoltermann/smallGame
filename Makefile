all:
	gcc -ggdb3 -Wall -lncurses main.c

run:
	./a.out

.PHONY: run
