/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2015 Inteno Broadband Technology AB
 *	  Author MOHAMED Kallel <mohamed.kallel@pivasoftware.com>
 *	  Author Imen Bhiri <imen.bhiri@pivasoftware.com>
 *	  Author Feten Besbes <feten.besbes@pivasoftware.com>
 *
 */

#ifndef __DMUCI_H
#define __DMUCI_H
#include "uci_config.h"
#include <stdbool.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdint.h>
#include <uci.h>
#include <libubox/list.h>

#define DB_CONFIG		"/lib/db/config"
#define VARSTATE_CONFIG "/var/state"
#define ICWMPD_CONFIG "/etc/icwmpd"
#define ICWMPD_PATH "icwmpd"
#define ICWMPD_SAVEDIR "/tmp/.icwmpd"

extern struct uci_context *uci_ctx;
extern struct uci_context *uci_varstate_ctx;

enum dm_uci_cmp {
	CMP_SECTION,
	CMP_OPTION_EQUAL,
	CMP_OPTION_REGEX,
	CMP_OPTION_CONTAINING,
	CMP_OPTION_CONT_WORD,
	CMP_LIST_CONTAINING,
	CMP_FILTER_FUNC
};
enum dm_uci_walk {
	GET_FIRST_SECTION,
	GET_NEXT_SECTION
};

struct package_change {
	struct list_head list;
	char *package;
};

#define uci_path_foreach_sections(path, package, stype, section) \
	for (section = dmuci_walk_section_##path(package, stype, NULL, NULL, CMP_SECTION, NULL, NULL, GET_FIRST_SECTION); \
		section != NULL; \
		section = dmuci_walk_section_##path(package, stype, NULL, NULL, CMP_SECTION, NULL, section, GET_NEXT_SECTION))

#define uci_path_foreach_option_eq(path, package, stype, option, val, section) \
	for (section = dmuci_walk_section_##path(package, stype, option, val, CMP_OPTION_EQUAL, NULL, NULL, GET_FIRST_SECTION); \
		section != NULL; \
		section = dmuci_walk_section_##path(package, stype, option, val, CMP_OPTION_EQUAL, NULL, section, GET_NEXT_SECTION))

#define uci_path_foreach_option_cont(path, package, stype, option, val, section) \
	for (section = dmuci_walk_section_##path(package, stype, option, val, CMP_OPTION_CONTAINING, NULL, NULL, GET_FIRST_SECTION); \
		section != NULL; \
		section = dmuci_walk_section_##path(package, stype, option, val, CMP_OPTION_CONTAINING, NULL, section, GET_NEXT_SECTION))

#define uci_foreach_sections(package, stype, section) \
	for (section = dmuci_walk_section(package, stype, NULL, NULL, CMP_SECTION, NULL, NULL, GET_FIRST_SECTION); \
		section != NULL; \
		section = dmuci_walk_section(package, stype, NULL, NULL, CMP_SECTION, NULL, section, GET_NEXT_SECTION))

#define uci_foreach_option_eq(package, stype, option, val, section) \
	for (section = dmuci_walk_section(package, stype, option, val, CMP_OPTION_EQUAL, NULL, NULL, GET_FIRST_SECTION); \
		section != NULL; \
		section = dmuci_walk_section(package, stype, option, val, CMP_OPTION_EQUAL, NULL, section, GET_NEXT_SECTION))

#define uci_foreach_option_cont(package, stype, option, val, section) \
	for (section = dmuci_walk_section(package, stype, option, val, CMP_OPTION_CONTAINING, NULL, NULL, GET_FIRST_SECTION); \
		section != NULL; \
		section = dmuci_walk_section(package, stype, option, val, CMP_OPTION_CONTAINING, NULL, section, GET_NEXT_SECTION))

#define uci_foreach_option_cont_word(package, stype, option, val, section) \
	for (section = dmuci_walk_section(package, stype, option, val, CMP_OPTION_CONT_WORD, NULL, NULL, GET_FIRST_SECTION); \
		section != NULL; \
		section = dmuci_walk_section(package, stype, option, val, CMP_OPTION_CONT_WORD, NULL, section, GET_NEXT_SECTION))

#define uci_foreach_list_cont(package, stype, option, val, section) \
	for (section = dmuci_walk_section(package, stype, option, val, CMP_LIST_CONTAINING, NULL, NULL, GET_FIRST_SECTION); \
		section != NULL; \
		section = dmuci_walk_section(package, stype, option, val, CMP_LIST_CONTAINING, NULL, section, GET_NEXT_SECTION))

#define uci_foreach_filter_func(package, stype, arg, func, section) \
	for (section = dmuci_walk_section(package, stype, arg, NULL, CMP_FILTER_FUNC, func, NULL, GET_FIRST_SECTION); \
		section != NULL; \
		section = dmuci_walk_section(package, stype, arg, NULL, CMP_FILTER_FUNC, func, section, GET_NEXT_SECTION))

#define section_name(s) (s)->e.name

static inline void uci_list_insert(struct uci_list *list, struct uci_list *ptr)
{
	list->next->prev = ptr;
	ptr->prev = list;
	ptr->next = list->next;
	list->next = ptr;
}

static inline void uci_list_add(struct uci_list *head, struct uci_list *ptr)
{
	uci_list_insert(head->prev, ptr);
}

static inline void uci_list_init(struct uci_list *ptr)
{
	ptr->prev = ptr;
	ptr->next = ptr;
}
char *dmuci_list_to_string(struct uci_list *list, char *delimitor);
void add_list_package_change(struct list_head *clist, char *package);
void free_all_list_package_change(struct list_head *clist);
int dmuci_lookup_ptr(struct uci_context *ctx, struct uci_ptr *ptr, char *package, char *section, char *option, char *value);
int dmuci_get_section_type(char *package, char *section,char **value);
int dmuci_get_option_value_string(char *package, char *section, char *option, char **value);
int dmuci_get_option_value_list(char *package, char *section, char *option, struct uci_list **value);
struct uci_option *dmuci_get_option_ptr(char *cfg_path, char *package, char *section, char *option);
int db_get_value_string(char *package, char *section, char *option, char **value);
int db_get_value_list(char *package, char *section, char *option, struct uci_list **value);
int dmuci_get_varstate_string(char *package, char *section, char *option, char **value);
int dmuci_get_varstate_list(char *package, char *section, char *option, struct uci_list **value);
int dmuci_commit_package(char *package);
int dmuci_commit(void);
int dmuci_revert_package(char *package);
int dmuci_revert(void);
int dmuci_change_packages(struct list_head *clist);
char *dmuci_set_value(char *package, char *section, char *option, char *value);
int dmuci_add_list_value(char *package, char *section, char *option, char *value);
int dmuci_del_list_value(char *package, char *section, char *option, char *value);
int dmuci_add_section(char *package, char *stype, struct uci_section **s, char **value);
int dmuci_delete(char *package, char *section, char *option, char *value);
int dmuci_lookup_ptr_by_section(struct uci_context *ctx, struct uci_ptr *ptr, struct uci_section *s, char *option, char *value);
int dmuci_get_value_by_section_string(struct uci_section *s, char *option, char **value);
int dmuci_get_value_by_section_list(struct uci_section *s, char *option, struct uci_list **value);
char *dmuci_set_value_by_section(struct uci_section *s, char *option, char *value);
int dmuci_delete_by_section(struct uci_section *s, char *option, char *value);
int dmuci_add_list_value_by_section(struct uci_section *s, char *option, char *value);
int dmuci_del_list_value_by_section(struct uci_section *s, char *option, char *value);
struct uci_section *dmuci_walk_section(char *package, char *stype, void *arg1, void *arg2, int cmp , int (*filter)(struct uci_section *s, void *value), struct uci_section *prev_section, int walk);
struct uci_section *dmuci_walk_state_section(char *package, char *stype, void *arg1, void *arg2, int cmp , int (*filter)(struct uci_section *s, void *value), struct uci_section *prev_section, int walk);
struct uci_section *dmuci_walk_section_icwmpd(char *package, char *stype, void *arg1, void *arg2, int cmp , int (*filter)(struct uci_section *s, void *value), struct uci_section *prev_section, int walk);
char *dmuci_set_value_by_section_icwmpd(struct uci_section *s, char *option, char *value);

int dmuci_add_section_icwmpd(char *package, char *stype, struct uci_section **s, char **value);
int dmuci_add_state_section(char *package, char *stype, struct uci_section **s, char **value);
char *dmuci_set_varstate_value(char *package, char *section, char *option, char *value);
char *dmuci_set_value_icwmpd(char *package, char *section, char *option, char *value);
int dmuci_delete_by_section_icwmpd(struct uci_section *s, char *option, char *value);
int dmuci_rename_section_by_section(struct uci_section *s, char *value);
int dmuci_exit_icwmpd(void);
int dmuci_init_icwmpd(void);
#define NEW_UCI_PATH(UCI_PATH, CPATH, DPATH)		\
struct uci_context *uci_ctx_##UCI_PATH;			\
const char *uci_savedir_##UCI_PATH = DPATH; \
const char *uci_confdir_##UCI_PATH = CPATH; \
int dmuci_get_section_type_##UCI_PATH(char *package, char *section,char **value)	\
{\
	struct uci_context *save_uci_ctx;	\
	save_uci_ctx = uci_ctx;			\
	uci_ctx = uci_ctx_##UCI_PATH;	\
	dmuci_get_section_type(package, section, value);	\
	uci_ctx = save_uci_ctx;			\
}\
int dmuci_init_##UCI_PATH(void)		\
{\
	uci_ctx_##UCI_PATH = uci_alloc_context();	\
	if (!uci_ctx_##UCI_PATH) {					\
		return -1;								\
	}											\
	uci_add_delta_path(uci_ctx_##UCI_PATH, uci_ctx_##UCI_PATH->savedir);	\
	uci_set_savedir(uci_ctx_##UCI_PATH, uci_savedir_##UCI_PATH);					\
	uci_set_confdir(uci_ctx_##UCI_PATH, strdup(uci_confdir_##UCI_PATH));					\
	return 0;	\
}\
int dmuci_exit_##UCI_PATH(void)		\
{\
	if (uci_ctx_##UCI_PATH) uci_free_context(uci_ctx_##UCI_PATH);\
	uci_ctx_##UCI_PATH = NULL; \
	return 0;	\
}\
int dmuci_get_option_value_string_##UCI_PATH(char *package, char *section, char *option, char **value)	\
{\
	struct uci_context *save_uci_ctx;	\
	save_uci_ctx = uci_ctx;			\
	uci_ctx = uci_ctx_##UCI_PATH;	\
	dmuci_get_option_value_string(package, section, option, value); \
	uci_ctx = save_uci_ctx;			\
}\
int dmuci_get_option_value_list_##UCI_PATH(char *package, char *section, char *option, struct uci_list **value) \
{\
	struct uci_context *save_uci_ctx;	\
	save_uci_ctx = uci_ctx;			\
	uci_ctx = uci_ctx_##UCI_PATH;	\
	dmuci_get_option_value_list(package, section, option, value); \
	uci_ctx = save_uci_ctx;			\
}\
char *dmuci_set_value_##UCI_PATH(char *package, char *section, char *option, char *value) \
{\
	struct uci_context *save_uci_ctx;	\
	save_uci_ctx = uci_ctx;			\
	uci_ctx = uci_ctx_##UCI_PATH;	\
	dmuci_set_value(package, section, option, value); \
	uci_ctx = save_uci_ctx;			\
}\
int dmuci_add_list_value_##UCI_PATH(char *package, char *section, char *option, char *value) \
{\
	struct uci_context *save_uci_ctx;	\
	save_uci_ctx = uci_ctx;			\
	uci_ctx = uci_ctx_##UCI_PATH;	\
	dmuci_add_list_value(package, section, option, value); \
	uci_ctx = save_uci_ctx;			\
}\
int dmuci_del_list_value_##UCI_PATH(char *package, char *section, char *option, char *value) \
{\
	struct uci_context *save_uci_ctx;	\
	save_uci_ctx = uci_ctx;			\
	uci_ctx = uci_ctx_##UCI_PATH;	\
	dmuci_del_list_value(package, section, option, value); \
	uci_ctx = save_uci_ctx;			\
}\
int dmuci_add_section_##UCI_PATH(char *package, char *stype, struct uci_section **s, char **value)\
{\
	struct uci_context *save_uci_ctx;	\
	save_uci_ctx = uci_ctx;			\
	uci_ctx = uci_ctx_##UCI_PATH;	\
	dmuci_add_section(package, stype, s, value); \
	uci_ctx = save_uci_ctx;			\
}\
int dmuci_delete_##UCI_PATH(char *package, char *section, char *option, char *value) \
{\
	struct uci_context *save_uci_ctx;	\
	save_uci_ctx = uci_ctx;			\
	uci_ctx = uci_ctx_##UCI_PATH;	\
	dmuci_delete(package, section, option, value); \
	uci_ctx = save_uci_ctx;			\
}\
char *dmuci_set_value_by_section_##UCI_PATH(struct uci_section *s, char *option, char *value)\
{\
	struct uci_context *save_uci_ctx;	\
	save_uci_ctx = uci_ctx;			\
	uci_ctx = uci_ctx_##UCI_PATH;	\
	dmuci_set_value_by_section(s, option, value); \
	uci_ctx = save_uci_ctx;			\
}\
int dmuci_delete_by_section_##UCI_PATH(struct uci_section *s, char *option, char *value)\
{\
	struct uci_context *save_uci_ctx;	\
	save_uci_ctx = uci_ctx;			\
	uci_ctx = uci_ctx_##UCI_PATH;	\
	dmuci_delete_by_section(s, option, value); \
	uci_ctx = save_uci_ctx;			\
}\
struct uci_section *dmuci_walk_section_##UCI_PATH(char *package, char *stype, void *arg1, void *arg2, int cmp , int (*filter)(struct uci_section *s, void *value), struct uci_section *prev_section, int walk)\
{\
	struct uci_context *save_uci_ctx;	\
	save_uci_ctx = uci_ctx;			\
	uci_ctx = uci_ctx_##UCI_PATH;	\
	dmuci_walk_section(package, stype, arg1, arg2, cmp ,filter, prev_section, walk); \
	uci_ctx = save_uci_ctx;			\
}\
int dmuci_commit_package_##UCI_PATH(char *package) \
{\
	struct uci_context *save_uci_ctx;	\
		save_uci_ctx = uci_ctx;			\
		uci_ctx = uci_ctx_##UCI_PATH;	\
		dmuci_commit_package(package); \
		uci_ctx = save_uci_ctx;			\
}\

#define DMUCI_GET_SECTION_TYPE(UCI_PATH, package, section, value) dmuci_get_section_type_##UCI_PATH(package, section, value)
#define DMUCI_GET_OPTION_VALUE_STRING(UCI_PATH, package, section, option, value) dmuci_get_option_value_string_##UCI_PATH(package, section, option, value)
#define DMUCI_GET_OPTION_VALUE_LIST(UCI_PATH, package, section, option, value) dmuci_get_option_value_list_##UCI_PATH(package, section, option, value)
#define DMUCI_SET_VALUE(UCI_PATH, package, section, option, value) dmuci_set_value_##UCI_PATH(package, section, option, value)
#define DMUCI_ADD_LIST_VALUE(UCI_PATH, package, section, option, value) dmuci_add_list_value_##UCI_PATH(package, section, option, value)
#define DMUCI_DEL_LIST_VALUE(UCI_PATH, package, section, option, value) dmuci_del_list_value_##UCI_PATH(package, section, option, value)
#define DMUCI_ADD_SECTION(UCI_PATH, package, stype, s, value) dmuci_add_section_##UCI_PATH(package, stype, s, value)
#define DMUCI_DEL_SECTION(UCI_PATH, package, section, option, value) dmuci_del_section_##UCI_PATH(package, section, option, value)
#define DMUCI_SET_VALUE_BY_SECTION(UCI_PATH, s, option, value) dmuci_set_value_by_section_##UCI_PATH(s, option, value)
#define DMUCI_DELETE_BY_SECTION(UCI_PATH, s, option, value) dmuci_delete_by_section_##UCI_PATH(s, option, value)
#define DMUCI_WALK_SECTION(UCI_PATH, package, stype, arg1, arg2, cmp , filter, value), struct uci_section *prev_section, int walk)\) dmuci_walk_section_##UCI_PATH(package, stype, arg1, arg2, cmp , filter, value)

#define DMUCI_COMMIT_PACKAGE(UCI_PATH, package) dmuci_commit_package_##UCI_PATH(package)
#define DMUCI_INIT(UCI_PATH) dmuci_init_##UCI_PATH()
#define DMUCI_EXIT(UCI_PATH) dmuci_exit_##UCI_PATH()
#endif

