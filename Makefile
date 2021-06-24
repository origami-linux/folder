outdir = out
srcdir = src
objdir = obj

compiler = clang
linker = clang

srcs = $(wildcard $(srcdir)/*.c)
objs = $(patsubst $(srcdir)/%,$(objdir)/%,$(srcs:.c=.o))

cflags = -std=gnu99 -O3 -pedantic -Iinclude -Iolutils/include -Wno-deprecated
ldflags = -s -lolutils -lcurl -larchive -ljson-c -Wall -Wextra -Wl,--gc-sections

all: olutils/x86_64/libolutils.a x86_64/$(outdir)/folder

olutils/x86_64/libolutils.a: olutils/olutils.c
	make -C olutils

# Compile
x86_64/$(objdir)/%.o: $(srcdir)/%.c
	$(compiler) -c $< -target x86_64-elf-linux $(cflags) -o $@

# Link
x86_64/$(outdir)/folder: $(patsubst $(objdir)/%,x86_64/$(objdir)/%,$(objs))
	$(linker) $(patsubst $(objdir)/%,x86_64/$(objdir)/%,$(objs)) -target x86_64-elf-linux -Lolutils/x86_64 $(ldflags) -o $@

clean:
	rm -f $(wildcard x86_64/obj/*.o) x86_64/out/folder
	make -C olutils clean
