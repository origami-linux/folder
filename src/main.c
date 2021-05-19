#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <wchar.h>
#include <stdlib.h>
#include <unistd.h>
#include <curl/curl.h>
#include <curl/easy.h>
#include <archive.h>

#include "meta.h"

#define lengthof(x) (sizeof(x) / sizeof(*x))

int is_string_in_array(char** arr, char* val)
{
	for(int i = 0; i < lengthof(arr); i++)
		if(strcmp(arr[i], val) == 0)
			return 1;

	return 0;
}

bool yes = false;
char **pkgs = NULL;
pkg_meta *metas = NULL;
int pkg_num = 0;
int meta_num = 0;

CURLcode download(FILE *fp, char *url, CURL *curl)
{
	curl_easy_reset(curl);
	if (!curl)
	{
		fprintf(stderr, "Curl had an error while being initializing\n");
		free(pkgs);
		exit(EXIT_FAILURE);
	}

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fwrite);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, true);
	curl_easy_setopt(curl, CURLOPT_FAILONERROR, true);

	CURLcode ret = curl_easy_perform(curl);

	return ret;
}

void install_pkg(char *pkg)
{
	CURL *curl = curl_easy_init();

	CURLcode res_data;
	FILE *fp_data;

	char url_data[56 + strlen(pkg) + 13];
	strcpy(url_data, "https://github.com/origami-linux/packages-repo/raw/main/");
	strcat(url_data, pkg);
	strcat(url_data, "/data.tar.xz");

	char outtar[23 + strlen(pkg) + 8];
	strcpy(outtar, "/var/cache/folder/data/");
	strcat(outtar, pkg);
	strcat(outtar, ".tar.xz");

	remove(outtar);

	fp_data = fopen(outtar,"wb");
	if(!fp_data)
	{
		fprintf(stderr, "error: unable to write to file '%s'\n", outtar);
	}

	res_data = download(fp_data, url_data, curl);
	fclose(fp_data);

	curl_easy_cleanup(curl);

	if(res_data != CURLE_OK)
	{
		if(res_data == 22)
		{
			fprintf(stderr, "Data for package '%s' not found in repo\n", pkg);
		}
		else
		{
			fprintf(stderr, "Curl had error %d: '%s'\n", res_data, curl_easy_strerror(res_data));
		}

		free(pkgs);
		exit(EXIT_FAILURE);
	}

}

void pre_install()
{	
	uint64_t total_size = 0;

	for(int i = 0; i < pkg_num; i++)
	{
		CURL *curl = curl_easy_init();

		char *pkg = pkgs[i];

		CURLcode res_meta;
		FILE *fp_meta;

		char url_meta[56 + strlen(pkg) + 11];
		strcpy(url_meta, "https://github.com/origami-linux/packages-repo/raw/main/");
		strcat(url_meta, pkg);
		strcat(url_meta, "/meta.json");

		char outmeta[23 + strlen(pkg) + 6];
		strcpy(outmeta, "/var/cache/folder/meta/");
		strcat(outmeta, pkg);
		strcat(outmeta, ".json");

		remove(outmeta);

		fp_meta = fopen(outmeta,"w");
		if(!fp_meta)
		{
			fprintf(stderr, "error: unable to write to file '%s'\n", outmeta);
		}

		res_meta = download(fp_meta, url_meta, curl);
		fclose(fp_meta);
		curl_easy_cleanup(curl);

		if(res_meta != CURLE_OK)
		{			
			if(res_meta == 22)
			{
				fprintf(stderr, "Package '%s' not found in repo\n", pkg);
			}
			else
			{
				fprintf(stderr, "Curl had error %d: '%s'\n", res_meta, curl_easy_strerror(res_meta));
			}

			remove(outmeta);
			free(pkgs);
			exit(EXIT_FAILURE);
		}

		pkg_meta meta = parse_meta(outmeta);
		metas = realloc(metas, sizeof(metas) + sizeof(meta));
		metas[meta_num] = meta;

		if(metas[meta_num].deps != NULL)
		{
			for(int j = 0; j < lengthof(metas[meta_num].deps); j++)
			{
				if(is_string_in_array(pkgs, metas[meta_num].deps[j]))
					continue;

				pkgs = realloc(pkgs, sizeof(pkgs) + sizeof(metas[meta_num].deps[j]));
				pkgs[pkg_num] = metas[meta_num].deps[j];
				pkg_num++;
			}
		}

		total_size += meta.size;

		printf("  * Name:'%s'    Developer:'%s'    Version:'%s'    Size:%lu bytes\n", pkg, meta.dev, meta.ver, meta.size);
		meta_num++;
	}

	printf("\nSize required on disk: %lu bytes\n", total_size);
}

int main(int argc, char *argv[])
{
	int opt;

	enum mode { NO_MODE, INSTALL_PKG } mode = NO_MODE;

	for(int i = 1; i < argc; i++)
	{
		if(argv[i][0] == '-' && strlen(argv[i]) >= 2)
		{
			switch(argv[i][1])
			{
			case 'i':
				if(mode != INSTALL_PKG)
				{
					mode = INSTALL_PKG;
				}
				else
				{
					free(pkgs);
					fprintf(stderr, "Cannot have multiple install options\n");
					exit(EXIT_FAILURE);
				}
				break;

			case 'y': yes = true; break;

			case 'h': /* Fall through */
			default:
				fprintf(stdout, "Usage: %s [-iyh, install] [package...]\n", argv[0]);
				exit(EXIT_FAILURE);
			}
		}
		else if(strcmp(argv[i], "install") == 0)
		{
			if(mode != INSTALL_PKG)
			{
				mode = INSTALL_PKG;
			}
			else
			{
				fprintf(stderr, "Cannot have multiple install options\n");
				free(pkgs);
				exit(EXIT_FAILURE);
			}
		}
		else
		{
			if(pkgs != NULL)
				if(is_string_in_array(pkgs, argv[i]))
					continue;

			pkgs = realloc(pkgs, sizeof(pkgs) + sizeof(argv[i]));
			pkgs[pkg_num] = argv[i];
			pkg_num++;
		}
	}

	if (mode == INSTALL_PKG && pkg_num > 0)
	{
		pre_install();
	}
}
