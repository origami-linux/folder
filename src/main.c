#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <locale.h>

#include <curl/curl.h>
#include <curl/easy.h>
#include <archive.h>
#include <archive_entry.h>

#include "meta.h"
#include "olutils/bool.h"
#include "olutils/json.h"

typedef struct
{
    char *pkg_name;
    bool is_dep;
} Pkg;


#define lengthof(x)(sizeof(x) / sizeof(*x))

bool yes = false;
Pkg *pkgs = NULL;
pkg_meta *metas = NULL;
int pkg_num = 0;
int meta_num = 0;

void free_ptrs()
{
	free(pkgs);
	free(metas);
}

bool is_pkg_marked(Pkg val)
{
	for(int i = 0; i < pkg_num; i++)
		if(strcmp(pkgs[i].pkg_name, val.pkg_name) == 0)
			return true;

	return false;
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

void download_pkg(char *pkg_name)
{
	CURL *curl = curl_easy_init();

	CURLcode res_data;
	FILE *fp_data;

	char url_data[56 + strlen(pkg_name) + 13];
	strcpy(url_data, "https://github.com/origami-linux/packages-repo/raw/main/");
	strcat(url_data, pkg_name);
	strcat(url_data, "/data.tar.xz");

	

	char out_data_filename[strlen(pkg_name) + 8];
	strcpy(out_data_filename, pkg_name);
	strcat(out_data_filename, ".tar.xz");
    
    char out_data[24 + strlen(out_data_filename)];
	strcpy(out_data, "/var/cache/folder/data/");
	strcat(out_data, out_data_filename);

	if(access(out_data, F_OK) != 0)
    {
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
	    		fprintf(stderr, "Data for package '%s' not found in repo\n", pkg_name);
	    	}
	    	else
	    	{
	    		fprintf(stderr, "Curl had error %d: '%s'\n", res_data, curl_easy_strerror(res_data));
	    	}

	    	exit(EXIT_FAILURE);
	    }
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

void install_pkg(Pkg pkg)
{
    char *pkg_name = pkg.pkg_name;

	printf("Installing package '%s'...\n", pkg_name);

	char in_data[23 + strlen(pkg_name) + 8];
	strcpy(in_data, "/var/cache/folder/data/");
	strcat(in_data, pkg_name);
	strcat(in_data, ".tar.xz");

	char data_dest[17 + strlen(pkg_name) + 1];
	strcpy(data_dest, "/tmp/folder-pkgs/");
	strcat(data_dest, pkg_name);

    extract(in_data, data_dest); // extracts in /tmp/folder-pkgs/${pkg_name}
    
    struct dirent *de;

	char bin_src[strlen(data_dest) + 6];
	strcpy(bin_src, data_dest);
	strcat(bin_src, "/bin/");
  
    DIR *dr = opendir(bin_src);
    if (dr != NULL)
    {
        while((de = readdir(dr)) != NULL)
	    {
	    	if(strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
	    		continue;

	    	char srcpath[strlen(bin_src) + strlen(de->d_name) + 1];
	    	strcpy(srcpath, bin_src);
	    	strcat(srcpath, de->d_name);

            char destpath[10 + strlen(de->d_name)];
	    	strcpy(destpath, "/usr/bin/");
	    	strcat(destpath, de->d_name);

            struct stat stats;
            stat(srcpath, &stats);

            long length;

	    	char *filedata __attribute__((__cleanup__(free))) = read_file(srcpath, &length, true);

            write_file(destpath, filedata, length);
            chmod(destpath, stats.st_mode);
	    }

        closedir(dr);
    }

    char include_src[strlen(data_dest) + 10];
	strcpy(include_src, data_dest);
	strcat(include_src, "/include/");
  
    dr = opendir(include_src);
    if (dr != NULL)
    {
        while((de = readdir(dr)) != NULL)
	    {
	    	if(strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
	    		continue;

	    	char srcpath[strlen(include_src) + strlen(de->d_name) + 1];
	    	strcpy(srcpath, include_src);
	    	strcat(srcpath, de->d_name);

            char destpath[14 + strlen(de->d_name)];
	    	strcpy(destpath, "/usr/include/");
	    	strcat(destpath, de->d_name);

            struct stat stats;
            stat(srcpath, &stats);

            long length;

	    	char *filedata __attribute__((__cleanup__(free))) = read_file(srcpath, &length, true);

            write_file(destpath, filedata, length);
            chmod(destpath, stats.st_mode);
	    }

        closedir(dr);
    }

    char lib_src[strlen(data_dest) + 6];
	strcpy(lib_src, data_dest);
	strcat(lib_src, "/lib/");
  
    dr = opendir(lib_src);
    if (dr != NULL)
    {
        while((de = readdir(dr)) != NULL)
	    {
	    	if(strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
	    		continue;

	    	char srcpath[strlen(lib_src) + strlen(de->d_name) + 1];
	    	strcpy(srcpath, lib_src);
	    	strcat(srcpath, de->d_name);

            char destpath[10 + strlen(de->d_name)];
	    	strcpy(destpath, "/usr/lib/");
	    	strcat(destpath, de->d_name);

            struct stat stats;
            stat(srcpath, &stats);

            long length;

	    	char *filedata __attribute__((__cleanup__(free))) = read_file(srcpath, &length, true);

            write_file(destpath, filedata, length);
            chmod(destpath, stats.st_mode);
	    }

        closedir(dr);
    }

    char lib32_src[strlen(data_dest) + 8];
	strcpy(lib32_src, data_dest);
	strcat(lib32_src, "/lib32/");
  
    dr = opendir(lib32_src);
    if (dr != NULL)
    {
        while((de = readdir(dr)) != NULL)
	    {
	    	if(strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
	    		continue;

	    	char srcpath[strlen(lib32_src) + strlen(de->d_name) + 1];
	    	strcpy(srcpath, lib32_src);
	    	strcat(srcpath, de->d_name);

            char destpath[12 + strlen(de->d_name)];
	    	strcpy(destpath, "/usr/lib32/");
	    	strcat(destpath, de->d_name);

            struct stat stats;
            stat(srcpath, &stats);

            long length;

	    	char *filedata __attribute__((__cleanup__(free))) = read_file(srcpath, &length, true);

            write_file(destpath, filedata, length);
            chmod(destpath, stats.st_mode);
	    }

        closedir(dr);
    }

    char lib64_src[strlen(data_dest) + 8];
	strcpy(lib64_src, data_dest);
	strcat(lib64_src, "/lib64/");
  
    dr = opendir(lib64_src);
    if (dr != NULL)
    {
        while((de = readdir(dr)) != NULL)
	    {
	    	if(strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
	    		continue;

	    	char srcpath[strlen(lib64_src) + strlen(de->d_name) + 1];
	    	strcpy(srcpath, lib64_src);
	    	strcat(srcpath, de->d_name);

            char destpath[12 + strlen(de->d_name)];
	    	strcpy(destpath, "/usr/lib64/");
	    	strcat(destpath, de->d_name);

            struct stat stats;
            stat(srcpath, &stats);

            long length;

	    	char *filedata __attribute__((__cleanup__(free))) = read_file(srcpath, &length, true);

            write_file(destpath, filedata, length);
            chmod(destpath, stats.st_mode);
	    }

        closedir(dr);
    }

    char libexec_src[strlen(data_dest) + 10];
	strcpy(libexec_src, data_dest);
	strcat(libexec_src, "/libexec/");
  
    dr = opendir(libexec_src);
    if (dr != NULL)
    {
        while((de = readdir(dr)) != NULL)
	    {
	    	if(strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
	    		continue;

	    	char srcpath[strlen(libexec_src) + strlen(de->d_name) + 1];
	    	strcpy(srcpath, libexec_src);
	    	strcat(srcpath, de->d_name);

            char destpath[14 + strlen(de->d_name)];
	    	strcpy(destpath, "/usr/libexec/");
	    	strcat(destpath, de->d_name);

            struct stat stats;
            stat(srcpath, &stats);

            long length;

	    	char *filedata __attribute__((__cleanup__(free))) = read_file(srcpath, &length, true);

            write_file(destpath, filedata, length);
            chmod(destpath, stats.st_mode);
	    }

        closedir(dr);
    }

    char local_src[strlen(data_dest) + 8];
	strcpy(local_src, data_dest);
	strcat(local_src, "/local/");
  
    dr = opendir(local_src);
    if (dr != NULL)
    {
        while((de = readdir(dr)) != NULL)
	    {
	    	if(strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
	    		continue;

	    	char srcpath[strlen(local_src) + strlen(de->d_name) + 1];
	    	strcpy(srcpath, local_src);
	    	strcat(srcpath, de->d_name);

            char destpath[12 + strlen(de->d_name)];
	    	strcpy(destpath, "/usr/local/");
	    	strcat(destpath, de->d_name);

            struct stat stats;
            stat(srcpath, &stats);

            long length;

	    	char *filedata __attribute__((__cleanup__(free))) = read_file(srcpath, &length, true);

            write_file(destpath, filedata, length);
            chmod(destpath, stats.st_mode);
	    }

        closedir(dr);
    }

    char sbin_src[strlen(data_dest) + 7];
	strcpy(sbin_src, data_dest);
	strcat(sbin_src, "/sbin/");
  
    dr = opendir(sbin_src);
    if (dr != NULL)
    {
        while((de = readdir(dr)) != NULL)
	    {
	    	if(strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
	    		continue;

	    	char srcpath[strlen(sbin_src) + strlen(de->d_name) + 1];
	    	strcpy(srcpath, sbin_src);
	    	strcat(srcpath, de->d_name);

            char destpath[11 + strlen(de->d_name)];
	    	strcpy(destpath, "/usr/sbin/");
	    	strcat(destpath, de->d_name);

            struct stat stats;
            stat(srcpath, &stats);

            long length;

	    	char *filedata __attribute__((__cleanup__(free))) = read_file(srcpath, &length, true);

            write_file(destpath, filedata, length);
            chmod(destpath, stats.st_mode);
	    }

        closedir(dr);
    }

    char share_src[strlen(data_dest) + 8];
	strcpy(share_src, data_dest);
	strcat(share_src, "/share/");
  
    dr = opendir(share_src);
    if (dr != NULL)
    {
        while((de = readdir(dr)) != NULL)
	    {
	    	if(strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
	    		continue;

	    	char srcpath[strlen(share_src) + strlen(de->d_name) + 1];
	    	strcpy(srcpath, share_src);
	    	strcat(srcpath, de->d_name);

            char destpath[12 + strlen(de->d_name)];
	    	strcpy(destpath, "/usr/share/");
	    	strcat(destpath, de->d_name);

            struct stat stats;
            stat(srcpath, &stats);

            long length;

	    	char *filedata __attribute__((__cleanup__(free))) = read_file(srcpath, &length, true);

            write_file(destpath, filedata, length);
            chmod(destpath, stats.st_mode);
	    }

        closedir(dr);
    }

    char src_src[strlen(data_dest) + 6];
	strcpy(src_src, data_dest);
	strcat(src_src, "/src/");
  
    dr = opendir(src_src);
    if (dr != NULL)
    {
        while((de = readdir(dr)) != NULL)
	    {
	    	if(strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
	    		continue;

	    	char srcpath[strlen(src_src) + strlen(de->d_name) + 1];
	    	strcpy(srcpath, src_src);
	    	strcat(srcpath, de->d_name);

            char destpath[10 + strlen(de->d_name)];
	    	strcpy(destpath, "/usr/src/");
	    	strcat(destpath, de->d_name);

            struct stat stats;
            stat(srcpath, &stats);

            long length;

	    	char *filedata __attribute__((__cleanup__(free))) = read_file(srcpath, &length, true);

            write_file(destpath, filedata, length);
            chmod(destpath, stats.st_mode);
	    }

        closedir(dr);
    }
}

void pre_install()
{	
	uint64_t total_size = 0;

	for(int i = 0; i < pkg_num; i++)
	{
		CURL *curl = curl_easy_init();

		Pkg pkg = pkgs[i];
        char *pkg_name = pkg.pkg_name;

		CURLcode res_meta;
		FILE *fp_meta;

		char url_meta[56 + strlen(pkg_name) + 11];
		strcpy(url_meta, "https://github.com/origami-linux/packages-repo/raw/main/");
		strcat(url_meta, pkg_name);
		strcat(url_meta, "/meta.json");

		char out_meta[23 + strlen(pkg_name) + 6];
		strcpy(out_meta, "/var/cache/folder/meta/");
		strcat(out_meta, pkg_name);
		strcat(out_meta, ".json");

        if(access(out_meta, F_OK) != 0)
        {
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
	    			fprintf(stderr, "Package '%s' not found in repo\n", pkg.pkg_name);
	    		}
	    		else
	    		{
	    			fprintf(stderr, "Curl had error %d: '%s'\n", res_meta, curl_easy_strerror(res_meta));
	    		}

	    		remove(out_meta);
    
	    		exit(EXIT_FAILURE);
		    }
        }

		pkg_meta meta = parse_meta(out_meta);
		metas = realloc(metas, sizeof(metas) + sizeof(meta));
		metas[meta_num] = meta;

		if(metas[meta_num].deps != NULL)
		{
			for(int j = 0; j < lengthof(metas[meta_num].deps); j++)
			{
				if(is_pkg_marked((Pkg){ metas[meta_num].deps[j], true }))
					continue;

				pkgs = realloc(pkgs, sizeof(pkgs) + sizeof(metas[meta_num].deps[j]));
				pkgs[pkg_num] = (Pkg){ metas[meta_num].deps[j], true };
				pkg_num++;
			}
		}

		total_size += meta.size;

		printf("  * Name:'%s'    Developer:'%s'    Version:'%s'    Size:%lu bytes\n", pkg.pkg_name, meta.dev, meta.ver, meta.size);
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
		download_pkg(pkgs[i].pkg_name);
	}

	for(int i = 0; i < pkg_num; i++)
	{
		install_pkg(pkgs[i]);
	}
}

int main(int argc, char *argv[])
{
    setlocale(LC_ALL, "");

    if(access("/var/cache/folder/", F_OK) != 0)
        mkdir("/var/cache/folder/", 0007);
    else
        chmod("/var/cache/folder/", 0007);

    if(access("/var/cache/folder/data/", F_OK) != 0)
        mkdir("/var/cache/folder/data/", 0007);
    else
        chmod("/var/cache/folder/data/", 0007);

    if(access("/var/cache/folder/meta/", F_OK) != 0)
        mkdir("/var/cache/folder/meta/", 0007);
    else
        chmod("/var/cache/folder/meta/", 0007);

    if(access("/etc/folder/", F_OK) != 0)
        mkdir("/etc/folder/", 0007);
    else
        chmod("/etc/folder/", 0007);

    if(access("/etc/folder/installed", F_OK) != 0)
        creat("/etc/folder/installed", 0007);
    else
        chmod("/etc/folder/installed", 0007);

	if(geteuid() != 0)
	{
		fprintf(stderr, "You need root privileges to run folder\n");
		exit(EXIT_FAILURE);
	}

	atexit(free_ptrs);

	int opt;

	enum mode { NO_MODE, INSTALL_PKGS, REMOVE_PKGS, UNCACHE_PKGS } mode = NO_MODE;

	for(int i = 1; i < argc; i++)
	{
		if(argv[i][0] == '-' && strlen(argv[i]) >= 2)
		{
			switch(argv[i][1])
			{
			case 'i':
				if(mode == NO_MODE)
				{
					mode = INSTALL_PKGS;
				}
				else
				{
					
					fprintf(stderr, "Cannot have multiple modes\n");
					exit(EXIT_FAILURE);
				}
				break;

            case 'r':
				if(mode == NO_MODE)
				{
					mode = REMOVE_PKGS;
				}
				else
				{
					
					fprintf(stderr, "Cannot have multiple modes\n");
					exit(EXIT_FAILURE);
				}
				break;

            case 'u':
				if(mode == NO_MODE)
				{
					mode = UNCACHE_PKGS;
				}
				else
				{
					
					fprintf(stderr, "Cannot have multiple modes\n");
					exit(EXIT_FAILURE);
				}
				break;

			case 'y': yes = true; break;

			case 'h':
			default:
				fprintf(stdout, "Usage: '%s [-y, -h, -i | install | -r | remove | -u | uncache] {package...}'\n", argv[0]);
				exit(EXIT_FAILURE);
			}
		}
		else if(strcmp(argv[i], "install") == 0)
		{
			if(mode != INSTALL_PKGS)
			{
				mode = INSTALL_PKGS;
			}
			else
			{
				fprintf(stderr, "Cannot have multiple install options\n");
				
				exit(EXIT_FAILURE);
			}
		}
        else if(strcmp(argv[i], "remove") == 0)
        {
            if(mode == NO_MODE)
			{
				mode = REMOVE_PKGS;
			}
			else
			{
				
				fprintf(stderr, "Cannot have multiple modes\n");
				exit(EXIT_FAILURE);
			}
        }
        else if(strcmp(argv[i], "uncache") == 0)
        {
            if(mode == NO_MODE)
			{
				mode = UNCACHE_PKGS;
			}
			else
			{
				
				fprintf(stderr, "Cannot have multiple modes\n");
				exit(EXIT_FAILURE);
			}
        }
		else
		{
			if(pkgs != NULL)
				if(is_pkg_marked((Pkg){ argv[i], false }))
					continue;

			pkgs = realloc(pkgs, sizeof(pkgs) + sizeof(argv[i]));
			pkgs[pkg_num] = (Pkg){ argv[i], false };
			pkg_num++;
		}
	}

	if(mode == INSTALL_PKGS && pkg_num > 0)
	{
		pre_install();
	}
    else if(mode == REMOVE_PKGS && pkg_num > 0)
	{
		pre_install();
	}
    else if(mode == UNCACHE_PKGS && pkg_num > 0)
	{
        for(int i = 0; i < pkg_num; i++)
        {
            char *pkg_name = pkgs[i].pkg_name;

            char data[23 + strlen(pkg_name) + 8];
	        strcpy(data, "/var/cache/folder/data/");
	        strcat(data, pkg_name);
	        strcat(data, ".tar.xz");

            if(access(data, F_OK) != 0)
            {
                fprintf(stdout, "Unable to access file '%s'\n", data);
			    exit(EXIT_FAILURE);
            }

            printf(" * %s\n", pkg_name);
        }

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
            char *pkg_name = pkgs[i].pkg_name;

            char data[23 + strlen(pkg_name) + 8];
	        strcpy(data, "/var/cache/folder/data/");
	        strcat(data, pkg_name);
	        strcat(data, ".tar.xz");

            char meta[23 + strlen(pkg_name) + 6];
	        strcpy(meta, "/var/cache/folder/meta/");
	        strcat(meta, pkg_name);
	        strcat(meta, ".json");

            remove(data);
            remove(meta);
        }
	}
    else
    {
		fprintf(stdout, "Usage: '%s [-y, -h, -i | install | -r | remove | -u | uncache] {package...}'\n", argv[0]);
		exit(EXIT_FAILURE);
	}
}
