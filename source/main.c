#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <wchar.h>
#include <stdlib.h>
#include <unistd.h>

#include <curl/curl.h>
#include <archive.h>
#include <curl/easy.h>

size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	size_t written = fwrite(ptr, size, nmemb, stream);
	return written;
}

char **pkgs = NULL;

void install_pkg(char* pkg)
{
	struct archive *a;
	struct archive *entry;
	int archive_ret;

	CURL *curl;
	CURLcode res;
	FILE *fp;
	
	char outtar[5 + strlen(pkg) + 8];
	strcpy(outtar, "/tmp/");
	strcat(outtar, pkg);
	strcat(outtar, ".tar.xz");

	char outmeta[5 + strlen(pkg) + 6];
	strcpy(outmeta, "/tmp/");
	strcat(outmeta, pkg);
	strcat(outmeta, ".json");

	curl = curl_easy_init();

	if (curl)
	{
		char url[56 + strlen(pkg) + 13];
		strcpy(url, "https://github.com/origami-linux/packages-repo/raw/main/");
		strcat(url, pkg);
		strcat(url, "/data.tar.xz");

		fp = fopen(outtar,"wb");
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, true);
		curl_easy_setopt(curl, CURLOPT_FAILONERROR, true);

		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);
		fclose(fp);
		if(res != CURLE_OK)
		{
			if(res == 22)
			{
				fprintf(stderr, "Package '%s' not found in repo\n", pkg);
			}
			else
			{
				fprintf(stderr, "Curl had error %d: '%s'\n", res, curl_easy_strerror(res));
			}

			while(remove(outtar) != 0) {;}
			free(pkgs);
			exit(EXIT_FAILURE);
		}
	}
	else
	{
		fprintf(stderr, "Curl had an error while being initializing\n");
		free(pkgs);
		exit(EXIT_FAILURE);
	}

	curl = curl_easy_init();

	if (curl)
	{
		char url[56 + strlen(pkg) + 11];
		strcpy(url, "https://github.com/origami-linux/packages-repo/raw/main/");
		strcat(url, pkg);
		strcat(url, "/meta.json");

		fp = fopen(outmeta,"w");
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, true);
		curl_easy_setopt(curl, CURLOPT_FAILONERROR, true);

		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);
		fclose(fp);

		if(res != CURLE_OK)
		{
			if(res == 22)
			{
				fprintf(stderr, "Metadata for package '%s' not found in repo\n", pkg);
			}
			else
			{
				fprintf(stderr, "Curl had error %d: '%s'\n", res, curl_easy_strerror(res));
			}

			while(remove(outtar) != 0) {;}
			while(remove(outmeta) != 0) {;}
			free(pkgs);
			exit(EXIT_FAILURE);
		}
	}
	else
	{
		fprintf(stderr, "Curl had an error while being initializing\n");
		free(pkgs);
		exit(EXIT_FAILURE);
	}

	// a = archive_read_new();
	while(remove(outtar) != 0) {;}
	while(remove(outmeta) != 0) {;}
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
