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

char *get_pid(char *pname)
{
	FILE* f = NULL;
	char str[TAILLE_MAX] = "";
	char *v;
	f = popen(pname, "r");
	if (f != NULL) {
		fgets(str, TAILLE_MAX, f);
		if (str[0] == '\0') {
			pclose(f);
			return "";
		}
		pid_t pid = strtoul(str, NULL, 10);
		pclose(f);
		dmasprintf(&v, "%d", pid); // MEM WILL BE FREED IN DMMEMCLEAN
		return v;
	}
	return "";
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
