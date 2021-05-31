#ifndef ORIGAMI_IOUTILS_H
#define ORIGAMI_IOUTILS_H
#include<stdio.h>
#include<stdlib.h>
#include<string.h>

char *read_file(char *path, long *len_ref);
void write_file(char *path, char *buffer, long lenght);

#endif
