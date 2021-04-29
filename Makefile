outdir = out
srcdir = src
objdir = obj

compiler = clang
linker = clang

srcs = $(wildcard $(srcdir)/*.c)
objs = $(patsubst $(srcdir)/%,$(objdir)/%,$(srcs:.c=.o))

cflags = -std=gnu99 -O3 -pedantic -Iinclude
ldflags = -s -lcurl -larchive -linih -Wl,--gc-sections

all: x86_64/$(outdir)/folder

# Link
x86_64/$(outdir)/folder: $(patsubst $(objdir)/%,x86_64/$(objdir)/%,$(objs))
	$(compiler) -o $@ $< -target x86_64-elf-linux $(ldflags)

# Compile
x86_64/$(objdir)/%.o: $(srcdir)/%.c
	$(linker) -o $@ -c $< -target x86_64-elf-linux $(cflags)

clean:
	rm -f $(wildcard *.o) $(wildcard folder)