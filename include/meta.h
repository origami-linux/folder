#ifndef ORIGAMI_META_H
#define ORIGAMI_META_H
#include <stdlib.h>
#include <ini.h>

#include "strutils.h"

typedef struct
{
	char *dev;
	char *desc;
	char *ver;
	int size;
	char **deps;
} pkg_meta;

int meta_ini_handler(void *meta, const char *section, const char *name, const char *value);

#endif
