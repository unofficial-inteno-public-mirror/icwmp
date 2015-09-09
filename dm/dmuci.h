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
struct uci_section *dmuci_walk_section (char *package, char *stype, void *arg1, void *arg2, int cmp , int (*filter)(struct uci_section *s, void *value), struct uci_section *prev_section, int walk);
#endif

