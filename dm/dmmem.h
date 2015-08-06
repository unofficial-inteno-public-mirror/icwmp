#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <libubox/list.h>
#ifndef __DMMEM_H
#define __DMMEM_H

static inline void dm_empty_func()
{
}

#define WITH_MEMLEACKSEC 1
//#define WITH_MEMTRACK 1

#ifndef WITH_MEMLEACKSEC
#undef WITH_MEMTRACK
#endif

#ifdef WITH_MEMLEACKSEC
struct dmmem {
	struct list_head list;
#ifdef WITH_MEMTRACK
	char *file;
	char *func;
	int line;
#endif /*WITH_MEMTRACK*/
	char mem[0];
};

inline void *__dmmalloc
(
#ifdef WITH_MEMTRACK
const char *file, const char *func, int line,
#endif /*WITH_MEMTRACK*/
size_t size
);

inline void *__dmcalloc
(
#ifdef WITH_MEMTRACK
const char *file, const char *func, int line,
#endif /*WITH_MEMTRACK*/
int n, size_t size
);

char *__dmstrdup
(
#ifdef WITH_MEMTRACK
const char *file, const char *func, int line,
#endif /*WITH_MEMTRACK*/
const char *s
);
inline void dmfree(void *m);
void dmcleanmem();
#endif /*WITH_MEMLEACKSEC*/
int __dmasprintf
(
#ifdef WITH_MEMTRACK
const char *file, const char *func, int line,
#endif /*WITH_MEMTRACK*/
char **s, const char *format, ...
);

int __dmastrcat
(
#ifdef WITH_MEMTRACK
const char *file, const char *func, int line,
#endif /*WITH_MEMTRACK*/
char **s, char *obj, char *lastname
);

#ifdef WITH_MEMLEACKSEC
#ifdef WITH_MEMTRACK
#define dmmalloc(x) __dmmalloc(__FILE__, __func__, __LINE__, x)
#define dmcalloc(n, x) __dmcalloc(__FILE__, __func__, __LINE__, n, x)
#define dmstrdup(x) __dmstrdup(__FILE__, __func__, __LINE__, x)
#define dmasprintf(s, format, ...) __dmasprintf(__FILE__, __func__, __LINE__, s, format, ## __VA_ARGS__)
#define dmastrcat(s, b, m) __dmastrcat(__FILE__, __func__, __LINE__, s, b, m)
#else
#define dmmalloc(x) __dmmalloc(x)
#define dmcalloc(n, x) __dmcalloc(n, x)
#define dmstrdup(x) __dmstrdup(x)
#define dmasprintf(s, format, ...) __dmasprintf(s, format, ## __VA_ARGS__)
#define dmastrcat(s, b, m) __dmastrcat(s, b, m)
#endif /*WITH_MEMTRACK*/
#else
#define dmmalloc(x) malloc(x)
#define dmcalloc(n, x) calloc(n, x)
#define __dmstrdup(x) strdup(x)
#define dmstrdup(x) strdup(x)
#define dmasprintf(s, format, ...) __dmasprintf(s, format, ## __VA_ARGS__)
#define dmastrcat(s, b, m) __dmastrcat(s, b, m)
#define dmfree(x) free(x)
#define dmcleanmem() dm_empty_func()
#endif /*WITH_MEMLEACKSEC*/

#define DMFREE(x) do { dmfree(x); x = NULL; } while (0);
#endif
