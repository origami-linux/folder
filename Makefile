link_flags = -s -lcurl -larchive -lpthread
build_flags = -std=gnu99 -Ofast -pedantic -Isource

all: out/folder

out/folder: obj/main.o
	gcc -o $@ $^ $(link_flags)

obj/main.o: src/main.c
	gcc -o $@ -c $< $(build_flags)

clean:
	rm -f obj/*.o out/folder