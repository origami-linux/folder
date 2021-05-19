#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/limits.h>
#include <curl/curl.h>
#include <curl/easy.h>
#include <archive.h>
#include <archive_entry.h>

#include "meta.h"

#define lengthof(x)(sizeof(x) / sizeof(*x))

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

void free_ptrs()
{
	free(pkgs);
	free(metas);
}

CURLcode download(FILE *fp, char *url, CURL *curl)
{
	curl_easy_reset(curl);
	if(!curl)
	{
		fprintf(stderr, "Curl had an error while being initializing\n");
		
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

void download_pkg(char *pkg)
{
	CURL *curl = curl_easy_init();

	CURLcode res_data;
	FILE *fp_data;

	char url_data[56 + strlen(pkg) + 13];
	strcpy(url_data, "https://github.com/origami-linux/packages-repo/raw/main/");
	strcat(url_data, pkg);
	strcat(url_data, "/data.tar.xz");

	char out_data[23 + strlen(pkg) + 8];
	strcpy(out_data, "/var/cache/folder/data/");
	strcat(out_data, pkg);
	strcat(out_data, ".tar.xz");

	char out_data_filename[strlen(pkg) + 8];
	strcpy(out_data_filename, pkg);
	strcat(out_data_filename, ".tar.xz");

	remove(out_data);

	printf("Downloading '%s'...\n", out_data_filename);

	fp_data = fopen(out_data,"wb");
	if(!fp_data)
	{
		fprintf(stderr, "error: unable to write to file '%s'\n", out_data);
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

		
		exit(EXIT_FAILURE);
	}
}

int copy_data(struct archive *ar, struct archive *aw)
{
  	int r;
  	const void *buff;
  	size_t size;
  	off_t offset;

  	for(;;)
	{
    	r = archive_read_data_block(ar, &buff, &size, &offset);

    	if(r == ARCHIVE_EOF)
      		return(ARCHIVE_OK);
    	if(r < ARCHIVE_OK)
      		return(r);

    	r = archive_write_data_block(aw, buff, size, offset);

    	if(r < ARCHIVE_OK)
		{
      		fprintf(stderr, "%s\n", archive_error_string(aw));
      		return(r);
    	}
  	}
}

void extract(char *filename, char *dest)
{
  	struct archive *a;
  	struct archive *ext;
  	struct archive_entry *entry;
  	int flags;
  	int r;

  	flags = ARCHIVE_EXTRACT_TIME;
  	flags |= ARCHIVE_EXTRACT_PERM;
  	flags |= ARCHIVE_EXTRACT_ACL;
  	flags |= ARCHIVE_EXTRACT_FFLAGS;

  	a = archive_read_new();
  	archive_read_support_format_tar(a);
	archive_read_support_compression_xz(a);

  	ext = archive_write_disk_new();
  	archive_write_disk_set_options(ext, flags);
  	archive_write_disk_set_standard_lookup(ext);

  	if((r = archive_read_open_filename(a, filename, 10240)))
	{
		fprintf(stderr, "Unable to load '%s'\n", filename);
  	  	exit(EXIT_FAILURE);
	}

	char cwd[PATH_MAX];

	if(getcwd(cwd, sizeof(cwd)) == NULL)
	{
		fprintf(stderr, "Unable to get current directory");
		exit(EXIT_FAILURE);
	}

	rmdir(dest);
	mkdir("/tmp/folder-pkgs", 0444);
	mkdir(dest, 0777);
	chdir(dest);

  	for(;;)
	{
  	  	r = archive_read_next_header(a, &entry);

  	  	if(r == ARCHIVE_EOF)
  	    	break;
  	 	if(r < ARCHIVE_OK)
  	    	fprintf(stderr, "%s\n", archive_error_string(a));
  	  	if(r < ARCHIVE_WARN)
  	    	exit(EXIT_FAILURE);

  	  	r = archive_write_header(ext, entry);

  		if(r < ARCHIVE_OK)
		{
  	    	fprintf(stderr, "%s\n", archive_error_string(ext));
		}
  	  	else if(archive_entry_size(entry) > 0)
		{
  	    	r = copy_data(a, ext);

  	    	if(r < ARCHIVE_OK)
  	      		fprintf(stderr, "%s\n", archive_error_string(ext));
  	    	if(r < ARCHIVE_WARN)
  	      		exit(EXIT_FAILURE);
  	  	}

  	  	r = archive_write_finish_entry(ext);

  	  	if(r < ARCHIVE_OK)
  	    	fprintf(stderr, "%s\n", archive_error_string(ext));
  	  	if(r < ARCHIVE_WARN)
  	    	exit(EXIT_FAILURE);
  	}

	chdir(cwd);

  	archive_read_close(a);
  	archive_read_free(a);
  	archive_write_close(ext);
  	archive_write_free(ext);
}


void install_pkg(char *pkg)
{
	char in_data[23 + strlen(pkg) + 8];
	strcpy(in_data, "/var/cache/folder/data/");
	strcat(in_data, pkg);
	strcat(in_data, ".tar.xz");

	char dest[17 + strlen(pkg) + 1];
	strcpy(dest, "/tmp/folder-pkgs/");
	strcat(dest, pkg);

    extract((char*)&in_data,(char*)&dest); // extracts in /tmp/folder-pkgs/${pkg}
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

		char out_meta[23 + strlen(pkg) + 6];
		strcpy(out_meta, "/var/cache/folder/meta/");
		strcat(out_meta, pkg);
		strcat(out_meta, ".json");

		remove(out_meta);

		fp_meta = fopen(out_meta,"w");
		if(!fp_meta)
		{
			fprintf(stderr, "error: unable to write to file '%s'\n", out_meta);
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

			remove(out_meta);
			
			exit(EXIT_FAILURE);
		}

		pkg_meta meta = parse_meta(out_meta);
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

	printf("\nSize required on disk: %lu bytes\n\n", total_size);

	if(!yes)
	{
		printf("Do you want to continue? [Y/n] ");
	
		char ch = getchar();
	
		switch(ch)
		{
		case 'y':
		case 'Y':
			break;
	
		default:
			fprintf(stderr, "Aborting\n");
			exit(EXIT_FAILURE);
			break;
		}
	}

	for(int i = 0; i < pkg_num; i++)
	{
		download_pkg(pkgs[i]);
	}

	for(int i = 0; i < pkg_num; i++)
	{
		install_pkg(pkgs[i]);
	}
}

int main(int argc, char *argv[])
{
	if(geteuid() != 0)
	{
		fprintf(stderr, "You need root privileges to run folder\n");
		exit(EXIT_FAILURE);
	}

	atexit(free_ptrs);

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
					
					fprintf(stderr, "Cannot have multiple install options\n");
					exit(EXIT_FAILURE);
				}
				break;

			case 'y': yes = true; break;

			case 'h':
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

	if(mode == INSTALL_PKG && pkg_num > 0)
	{
		if(pkgs == NULL)
		{
			fprintf(stdout, "Usage: %s [-iyh, install] [package...]\n", argv[0]);
			exit(EXIT_FAILURE);
		}

		pre_install();
	}
}
