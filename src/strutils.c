#include "strutils.h"

void removechar(char *str, int index)
{
    int i,j;
    i = 0;
    while(i < strlen(str))
    {
        if(i == index) 
        { 
            for(j = i; j < strlen(str); j++)
                str[j] = str[j+1];   
        } else i++;
    }
}

void removechars(char *str, char t)
{
    int i,j;
    i = 0;
    while(i < strlen(str))
    {
        if(str[i] == t) 
        { 
            for(j = i; j < strlen(str); j++)
                str[j] = str[j+1];   
        } else i++;
    }
}

char** strsplit(char* a_str, const char a_delim)
{
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;

    while (*tmp)
    {
        if (a_delim == *tmp)
        {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }

    count += last_comma < (a_str + strlen(a_str) - 1);
    count++;

    result = malloc(sizeof(char*) * count);

    if (result)
    {
        size_t idx  = 0;
        char* token = strtok(a_str, delim);

        while (token)
        {
            assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }

        *(result + idx) = 0;
    }

    return result;
}
