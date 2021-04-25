all: fold
	
fold: main.c
	gcc -ansi -O3 -Os -std=gnu89 -o folder -lcurl main.c

clean:
	rm -f fold
