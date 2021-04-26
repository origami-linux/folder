#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#include <curl/curl.h>
#include <archive.h>
#include <curl/easy.h>
#include <libtar.h>

size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	size_t written = fwrite(ptr, size, nmemb, stream);
	return written;
}

int main(int argc, char *argv[])
{
	int opt;
	bool yes = false;

	char *pkg_name = NULL;

	enum mode { INSTALL_PKG } mode;

	while ((opt = getopt(argc, argv, "i:p: yh")) != -1) {
		switch (opt) {

		case 'i':
			mode = INSTALL_PKG;
			pkg_name = strdup(optarg);
			break;

		case 'y': yes = true; break;

		case 'h': /* Fall through */
		default:
			fprintf(stderr, "Usage: %s [-iyh] [package...]\n", argv[0]);
			exit(EXIT_FAILURE);
		}
	}

	// TODO: Debug this, gives SEGV
	if (mode == INSTALL_PKG && pkg_name != NULL) {
		CURL *curl;
		FILE *fp;
		CURLcode res;

		char url[46 + strlen(pkg_name) + 7];
		strcpy(url, "https://github.com/origami-linux/packages-repo/");
		strcat(url, pkg_name);
		strcat(url, ".tar.xz");
		char outfile[4 + strlen(pkg_name) + 7];
		strcat(outfile, "/tmp/");
		strcat(outfile, pkg_name);
		strcat(outfile, ".tar.xz");
		curl = curl_easy_init();
		if (curl)
		{
			fp = fopen(outfile,"wb");
			curl_easy_setopt(curl, CURLOPT_URL, url);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
			curl_easy_setopt(curl, CURLOPT_FAILONERROR, true);
			fclose(fp);
			curl_easy_cleanup(curl);
			if(res != CURLE_OK)
			{
				if(res == 440) printf("Package '%s' not found in repo\n", pkg_name);
				else printf("Curl had error %d\n", res);

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
}