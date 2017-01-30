/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2015 Inteno Broadband Technology AB
 *	  Author Imen Bhiri <imen.bhiri@pivasoftware.com>
 *
 */


#include <uci.h>
#include <ctype.h>
#include "dmcwmp.h"
#include "dmuci.h"
#include "dmubus.h"
#include "dmcommon.h"
#include "x_inteno_se_buttons.h"

struct button_args cur_button_args = {0};
inline int entry_xinteno_button_instance(struct dmctx *ctx, char *ibutton);
inline int entry_xinteno_buttons(struct dmctx *ctx);

inline int init_args_button(struct dmctx *ctx, struct uci_section *s)
{
	struct button_args *args = &cur_button_args;
	ctx->args = (void *)args;
	args->button_section = s;
	return 0;
}

/************************************************************************************* 
**** function related to button ****
**************************************************************************************/

int get_x_inteno_button_name(char *refparam, struct dmctx *ctx, char **value)
{
	struct uci_list *val;

	dmuci_get_value_by_section_list(cur_button_args.button_section, "button", &val);
	if (val)
		*value = dmuci_list_to_string(val, " ");
	else
		*value = "";
	return 0;
}

int get_x_inteno_button_hotplug(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_value_by_section_string(cur_button_args.button_section, "hotplug", value);
	return 0;
}

int get_x_inteno_button_hotplug_long(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_value_by_section_string(cur_button_args.button_section, "hotplug_long", value);
	return 0;
}

int get_x_inteno_button_minpress(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_value_by_section_string(cur_button_args.button_section, "minpress", value);
	return 0;
}

int set_x_inteno_button_minpress(char *refparam, struct dmctx *ctx, int action, char *value)
{

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(cur_button_args.button_section, "minpress", value);
			return 0;
	}
	return 0;
}

int get_x_inteno_button_longpress(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_value_by_section_string(cur_button_args.button_section, "longpress", value);
	return 0;
}

int set_x_inteno_button_longpress(char *refparam, struct dmctx *ctx, int action, char *value)
{

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(cur_button_args.button_section, "longpress", value);
			return 0;
	}
	return 0;
}


int get_x_inteno_button_enable(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_value_by_section_string(cur_button_args.button_section, "enable", value);
	if ((*value)[0] == '\0') {
		*value = "1";
	}
	return 0;
}

int set_x_inteno_button_enable(char *refparam, struct dmctx *ctx, int action, char *value)
{
	bool b;

	switch (action) {
		case VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		case VALUESET:
			string_to_bool(value, &b);
			if(b)
				dmuci_set_value_by_section(cur_button_args.button_section, "enable", "");
			else
				dmuci_set_value_by_section(cur_button_args.button_section, "enable", "0");
			return 0;
	}
	return 0;
}
////////////////////////SET AND GET ALIAS/////////////////////////////////

int get_x_inteno_button_alias(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_value_by_section_string(cur_button_args.button_section, "buttonalias", value);
	return 0;
}

int set_x_inteno_button_alias(char *refparam, struct dmctx *ctx, int action, char *value)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(cur_button_args.button_section, "buttonalias", value);
			return 0;
	}
	return 0;
}

/////////////SUB ENTRIES///////////////
inline int entry_xinteno_buttons(struct dmctx *ctx)
{
	char *ibutton = NULL, *ibutton_last = NULL;
	struct uci_section *s = NULL;

	uci_foreach_sections("buttons", "button", s) {
		init_args_button(ctx, s);
		ibutton =  handle_update_instance(1, ctx, &ibutton_last, update_instance_alias, 3, s, "buttoninstance", "buttonalias");
		SUBENTRY(entry_xinteno_button_instance, ctx, ibutton);
	}
	return 0;
}

//////////////////////////////////////

int entry_method_root_X_INTENO_SE_BUTTONS(struct dmctx *ctx)
{
	IF_MATCH(ctx, DMROOT"X_INTENO_SE_Buttons.") {
		DMOBJECT(DMROOT"X_INTENO_SE_Buttons.", ctx, "0", 1, NULL, NULL, NULL);
		SUBENTRY(entry_xinteno_buttons, ctx);
		return 0;
	}
	return FAULT_9005;
}

inline int entry_xinteno_button_instance(struct dmctx *ctx, char *ibutton)
{
	IF_MATCH(ctx, DMROOT"X_INTENO_SE_Buttons.%s.", ibutton) {
		DMOBJECT(DMROOT"X_INTENO_SE_Buttons.%s.", ctx, "0", 1, NULL, NULL, NULL, ibutton);
		DMPARAM("Alias", ctx, "1", get_x_inteno_button_alias, set_x_inteno_button_alias, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("button", ctx, "0", get_x_inteno_button_name, NULL, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("hotplug", ctx, "0", get_x_inteno_button_hotplug, NULL, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("hotplug_long", ctx, "0", get_x_inteno_button_hotplug_long, NULL, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("minpress", ctx, "1", get_x_inteno_button_minpress, set_x_inteno_button_minpress, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		DMPARAM("longpress", ctx, "1", get_x_inteno_button_longpress, set_x_inteno_button_longpress, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		DMPARAM("enable", ctx, "1", get_x_inteno_button_enable, set_x_inteno_button_enable, "xsd:boolean", 0, 1, UNDEF, NULL);
		return 0;
	}
	return FAULT_9005;
}

