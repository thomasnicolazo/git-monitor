all:
	gcc main.c -o main -Wall -lncurses

panel:
		gcc test/panel.c -o panel -Wall -lpanel -lncurses

clean:
	rm main