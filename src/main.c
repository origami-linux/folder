/* Folder - Origami Linux package manager.
	License - MIT

	Revision history:
		* 0.1 - 26 Apr 2021

	To do:
		* Cache the meta and data files (offline install/reinstall) (that's why i removed the `remove`)
		* Keep list/database of installed programs

	Notes:
		* Untested (once someone tests this remove this note)
	Bugs:
*/

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

CURLcode download(FILE *fp, char *url, CURL *curl) {
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fwrite);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, true);
	curl_easy_setopt(curl, CURLOPT_FAILONERROR, true);

	return curl_easy_perform(curl);
}

char **pkgs = NULL;

void install_pkg(char* pkg)
{
	CURL *curl;
	CURLcode res_meta, res_data;
	FILE *fp_meta, *fp_data;

	char outmeta[5 + strlen(pkg) + 5];
	strcpy(outmeta, "/tmp/");
	strcat(outmeta, pkg);
	strcat(outmeta, ".json");

	char outtar[5 + strlen(pkg) + 8];
	strcpy(outtar, "/tmp/");
	strcat(outtar, pkg);
	strcat(outtar, ".tar.xz");

	curl = curl_easy_init();

	if (!curl) {
		fprintf(stderr, "Curl had an error while being initializing\n");
		free(pkgs);
		exit(EXIT_FAILURE);
	}

	/* No need of `else`, since if curl doesn't initialize, we exit. */

	char url_meta[56 + strlen(pkg) + 10];
	strcpy(url, "https://github.com/origami-linux/packages-repo/raw/main/");
	strcat(url, pkg);
	strcat(url, "/meta.json");

	char url_data[56 + strlen(pkg) + 13];
	strcpy(url, "https://github.com/origami-linux/packages-repo/raw/main/");
	strcat(url, pkg);
	strcat(url, "/data.tar.xz");

	fp_meta = fopen(outmeta,"w");
	res_meta = download(fp_meta, url_meta, curl);
	fclose(fp_meta);

	fp_data = fopen(outtar,"wb");
	res_data = download(fp_data, url_data, curl);
	fclose(fp_data);

	curl_easy_cleanup(curl);

	if(res_meta != CURLE_OK)
	{
		if(res_meta == 22)
		{
			fprintf(stderr, "Package '%s' not found in repo\n", pkg);
		}
		else
		{
			fprintf(stderr, "Curl had error %d: '%s'\n", res, curl_easy_strerror(res));
		}

		//remove(outmeta);
		free(pkgs);
		exit(EXIT_FAILURE);
	} else if(res_data != CURLE_OK)
	{
		if(res_data == 22)
		{
			fprintf(stderr, "Data for package '%s' not found in repo\n", pkg);
		}
		else
		{
			fprintf(stderr, "Curl had error %d: '%s'\n", res_data, curl_easy_strerror(res_data));
		}

		//remove(outtar);
		free(pkgs);
		exit(EXIT_FAILURE);
	}

	//remove(outtar);
}

int main(int argc, char *argv[])
{
	int opt;
	bool yes = false;

	int pkg_num = 0;

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
			pkgs = realloc(pkgs, sizeof(pkgs) + sizeof(argv[i]));
			pkgs[pkg_num] = argv[i];
			pkg_num++;
		}
	}

	if (mode == INSTALL_PKG && pkg_num > 0)
	{
		for(int i = 0; i < pkg_num; i++)
		{
			install_pkg(pkgs[i]);
		}
	}
}
