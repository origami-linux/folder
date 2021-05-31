#include"ioutils.h"

char *read_file(char *path, long *len_ref)
{
    char *buffer = 0;
    long length;
    FILE *f = fopen(path, "rb");

    if(!f)
    {
        fprintf(stderr, "Unable to open file '%s'\n", path);
        exit(EXIT_FAILURE);
    }
    
    fseek(f, 0, SEEK_END);
    *len_ref = length = ftell (f);
    fseek(f, 0, SEEK_SET);
    buffer = malloc(length);
    if(!buffer)
    {
        fprintf(stderr, "Unable to allocate buffer\n");
        exit(EXIT_FAILURE);
    }

    fread(buffer, 1, length, f);
    fclose(f);

    return buffer;
}

void write_file(char *path, char *buffer, long length)
{
    FILE *f = fopen(path, "wb");

    if(!f)
    {
        fprintf(stderr, "Unable to open or create file '%s'\n", path);
        exit(EXIT_FAILURE);
    }

    fwrite(buffer, 1, length, f);
    fclose(f);
}
