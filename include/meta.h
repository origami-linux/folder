#ifndef ORIGAMI_META_H
#define ORIGAMI_META_H
#include <stdlib.h>

#include <json-c/json.h>

#include "ioutils.h"

typedef struct
{
	char *dev;
	char *desc;
	char *ver;
	uint64_t size;
	char **deps;
} pkg_meta;

pkg_meta parse_meta(char *path);

#endif
