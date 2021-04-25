#include<stdio.h>
#include<string.h>
#include<stdbool.h>

#include<curl/curl.h>
#include<archive.h>
#include<curl/easy.h>
#include<libtar.h>

size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}

void print_help()
{
    printf(
        "Usage: fold [OPTIONS]\n"
        "\n"
        "OPTIONS\n"
        " -h --help                  Display fold usage and options\n"
        " -i install                 Install a package\n"
        " -y --yes                   Assume yes to all questions\n"
        );
}

int main(int argc, char *argv[])
{
    if(argc < 2)
    {
        print_help();
        return -1;
    }

    char *install_pkg = NULL;

    short i;
    for(i = 1; i <= argc; i++)
    {
        if((strcmp(argv[1], "-h") == 0) || (strcmp(argv[i], "--help") == 0))
        {
            print_help();
            return 0;
        }
        else if((strcmp(argv[i], "install") == 0) || (strcmp(argv[1], "i") == 0))
        {
            if(argc >= i + 1)
            {
                if(install_pkg == NULL)
                {
                    i++;
                    install_pkg = argv[i];
                }
                else
                {
                    printf("You cannot have two install options two packages\n");
                    return -1;
                }
            }
            else
            {
                printf("You have to specify a package\n");
                return -1;
            }
        }
    }

    if(install_pkg != NULL)
    {
        CURL *curl;
FILE *fp;
CURLcode res;

char url[46 + strlen(install_pkg) + 7];
strcpy(url, "https://github.com/origami-linux/packages-repo/");
strcat(url, install_pkg);
strcat(url, ".tar.xz");
char outfile[4 + strlen(install_pkg) + 7];
strcat(outfile, "/tmp/");
strcat(outfile, install_pkg);
strcat(outfile, ".tar.xz");
curl = curl_easy_init();
if (curl)
{
    fp = fopen(outfile,"wb");
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, true);
    curl_easy_cleanup(curl);
    fclose(fp);
    if(res != CURLE_OK)
    {
        if(res == 440)
            printf("Package '%s' not found in repo\n", install_pkg);
        else
            printf("Curl had error %d\n", res);
        
        // TODO: remove file in temp
        return -1;
    }
}
else
{
    printf("Curl had an error while being initializing\n");
    return -1;
}
    }

    return 0;
}
