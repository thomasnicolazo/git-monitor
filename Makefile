all:
	gcc -g main.c -o gitrepo -lncursesw

clean:
	rm gitrepo