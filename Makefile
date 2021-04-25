all: fold
	
fold: main.c
	gcc -ansi -std=gnu89 -o folder -lcurl main.c

clean:
	rm -f fold
