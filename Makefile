build_dir = out
source_dir = src
obj_dir = obj

compiler = clang
linker = clang

c_flags = -std=gnu99 -O3 -pedantic
ld_flags = -s -lcurl -lpthread -Wl,--gc-sections -Wl,--as-needed

all: x86_64/$(build_dir)/folder

# Link
x86_64/$(build_dir)/folder: x86_64/$(obj_dir)/main.o
	$(compiler) -o $@ $^ -target x86_64-elf-linux $(ld_flags)

# Compile
x86_64/$(obj_dir)/main.o: $(source_dir)/main.c
	$(linker) -o $@ -c $< -target x86_64-elf-linux $(c_flags)

clean:
	rm -f $(shell find ./ -type f -name '*.o') $(shell find ./ -type f -name 'folder')