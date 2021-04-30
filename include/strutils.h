#ifndef ORIGAMI_STRUTILS_H
#define ORIGAMI_STRUTILS_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

void removechar(char *str, int index);
void removechars(char *str, char t);
char** strsplit(char* a_str, const char a_delim);

#endif
