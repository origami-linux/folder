#include "meta.h"

pkg_meta parse_meta(char *path)
{
    pkg_meta ret;

    long length;
    char *filedata __attribute__((__cleanup__(free))) = read_file(path, &length);
    json_object *meta_json = json_tokener_parse(filedata);

    ret.dev     = (char*)json_object_get_string(json_object_object_get(meta_json, "developer"));
    ret.desc    = (char*)json_object_get_string(json_object_object_get(meta_json, "description"));
    ret.ver     = (char*)json_object_get_string(json_object_object_get(meta_json, "version"));
    ret.size    =        json_object_get_uint64(json_object_object_get(meta_json, "size"));

    array_list *deps_al = json_object_get_array(json_object_object_get(meta_json, "dependencies"));
    char **deps = NULL;
 
    for(int i = 0; i < deps_al->length; i++)
    {
        char *dep = (char*)json_object_get_string(deps_al->array[i]);
        deps = realloc(deps, sizeof(deps) + sizeof(dep));
        deps[i] = dep;
    }

    ret.deps = deps;

    return ret;
}
