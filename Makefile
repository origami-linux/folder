outdir = out
srcdir = src
objdir = obj

compiler = clang
linker = clang

srcs = $(wildcard $(srcdir)/*.c)
objs = $(patsubst $(srcdir)/%,$(objdir)/%,$(srcs:.c=.o))

cflags = -std=gnu99 -O3 -pedantic -Iinclude -Wno-deprecated
ldflags = -s -lcurl -larchive -ljson-c -Wall -Wextra -Wl,--gc-sections

all: x86_64/$(outdir)/folder

# Compile
x86_64/$(objdir)/%.o: $(srcdir)/%.c
	$(linker) -c $< -target x86_64-elf-linux $(cflags) -o $@

# Link
x86_64/$(outdir)/folder: $(patsubst $(objdir)/%,x86_64/$(objdir)/%,$(objs))
	$(compiler) $(patsubst $(objdir)/%,x86_64/$(objdir)/%,$(objs)) -target x86_64-elf-linux $(ldflags) -o $@

clean:
	rm -f $(wildcard x86_64/obj/*.o) x86_64/out/folder
