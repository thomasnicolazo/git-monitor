all:
	gcc -g main.c -o gitrepo -lncurses

clean:
	rm gitrepo