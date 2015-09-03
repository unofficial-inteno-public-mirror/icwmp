/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2015 PIVA SOFTWARE (www.pivasoftware.com)
 *		Author: Imen Bhiri <imen.bhiri@pivasoftware.com>
 *		Author: Feten Besbes <feten.besbes@pivasoftware.com>
 */

#ifndef __DM_COMMON_H
#define __DM_COMMON_H
#include <sys/types.h>

#define DM_ASSERT(X, Y) \
do { \
	if(!(X)) { \
		Y; \
		return -1; \
	} \
} while(0)

#define dmstrappendstr(dest, src) \
do { \
	int len = strlen(src); \
	memcpy(dest, src, len); \
	dest += len; \
} while(0)

#define dmstrappendchr(dest, c) \
do { \
	*dest = c; \
	dest += 1; \
} while(0)

#define dmstrappendend(dest) \
do { \
	*dest = '\0'; \
} while(0)



void compress_spaces(char *str);
char *cut_fx(char *str, char *delimiter, int occurence);
pid_t get_pid(char *pname);
int check_file(char *path);
char *cidr2netmask(int bits);
void remove_substring(char *s, const char *str_remove);
bool is_strword_in_optionvalue(char *optionvalue, char *str);

#endif
