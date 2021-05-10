#ifndef ORIGAMI_META_H
#define ORIGAMI_META_H
#include <stdlib.h>
#include <json-c.h>

#include "strutils.h"

typedef struct
{
	char *dev;
	char *desc;
	char *ver;
	int size;
	char **deps;
} pkg_meta;

#endif
