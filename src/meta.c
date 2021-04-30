#include "meta.h"

int meta_ini_handler(void *meta, const char *section, const char *name, const char *value)
{
	pkg_meta *pkgmeta = (pkg_meta*)meta;

	char* val = (char*)value;
    
    if(strcmp(name, "developer") == 0)
	{
		if(strcmp(val, "$NULL") == 0)
			return 0;

		pkgmeta->dev = val;
	}
    else if(strcmp(name, "description") == 0)
	{
		if(strcmp(val, "$NULL") == 0)
			return 0;

		pkgmeta->desc = val;
	}
	else if(strcmp(name, "version") == 0)
	{
		if(strcmp(val, "$NULL") == 0)
			return 0;

		pkgmeta->ver = val;
	}
	else if(strcmp(name, "size") == 0)
	{
		if(strcmp(val, "$NULL") == 0)
			return 0;

		pkgmeta->size = atoi(val);
	}
	else if(strcmp(name, "dependencies") == 0)
	{
		if(strcmp(val, "$NULL") == 0)
		{
			pkgmeta->deps = NULL;
		}
		else
		{
			removechars(val, ' ');
			pkgmeta->deps = strsplit(val, ',');
		}
	}
	else
	{
		return 0;
	}

    return 1;
}
