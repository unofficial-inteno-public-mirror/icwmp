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

#include "dmuci.h"
#include "dmcwmp.h"
#include "root.h"

int entry_method_root(struct dmctx *ctx)
{
	char *val = NULL;
	IF_MATCH(ctx, DMROOT) {
		DMOBJECT(DMROOT, ctx, "0", 0, NULL, NULL, NULL);
		return 0;
	}
	return FAULT_9005;
}
