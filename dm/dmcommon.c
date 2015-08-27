/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2012-2014 PIVA SOFTWARE (www.pivasoftware.com)
 *		Author: Imen Bhiri <imen.bhiri@pivasoftware.com>
 *		Author: Feten Besbes <feten.besbes@pivasoftware.com>
 */

#include <glob.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#include "dmcwmp.h"

void compress_spaces(char *str) //REMOVE TO DMCOMMON
{
    char *dst = str;
    for (; *str; ++str) {
        *dst++ = *str;        
        if (isspace(*str)) {
            do ++str; 
            while (isspace(*str));
            --str;
        }
    }
    *dst = '\0';
}
char *cut_fx(char *str, char *delimiter, int occurence)
{
	int i = 1;
	char *pch;
	pch = strtok (str, delimiter);
	while (pch != NULL && i<occurence) {
		i++;
		pch = strtok(NULL, delimiter);
	}
	return pch;
}

pid_t get_pid(char *pname)
{
    DIR* dir;
    struct dirent* ent;
    char* endptr;
    char buf[512];

    if (!(dir = opendir("/proc"))) {
        return -1;
    }

    while((ent = readdir(dir)) != NULL) {
        /* if endptr is not a null character, the directory is not
         * entirely numeric, so ignore it */
        long lpid = strtol(ent->d_name, &endptr, 10);
        if (*endptr != '\0') {
            continue;
        }

        /* try to open the cmdline file */
        snprintf(buf, sizeof(buf), "/proc/%ld/cmdline", lpid);
        FILE* fp = fopen(buf, "r");

        if (fp) {
            if (fgets(buf, sizeof(buf), fp) != NULL) {
                /* check the first token in the file, the program name */
                char* first = strtok(buf, " ");
                if (strstr(first, name)) {
                    fclose(fp);
                    closedir(dir);
                    return (pid_t)lpid;
                }
            }
            fclose(fp);
        }
    }

    closedir(dir);
    return -1;
}

int check_file(char *path) 
{
	glob_t globbuf;
	if(glob(path, 0, NULL, &globbuf) == 0) {
		globfree(&globbuf);
		return 1;
	}
	return 0;
}
