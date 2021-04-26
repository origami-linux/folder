link_flags = -s -lcurl -lpthread
build_flags = -std=gnu99 -Ofast -pedantic -Isource

all: folder

folder: main.o
	gcc -o $@ $^ $(link_flags)

main.o: source/main.c
	gcc -o $@ -c $< $(build_flags)

clean:
	rm -f folder