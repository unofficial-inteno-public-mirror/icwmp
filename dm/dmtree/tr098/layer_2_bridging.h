/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2012-2015 PIVA SOFTWARE (www.pivasoftware.com)
 *		Author: Anis Ellouze <anis.ellouze@pivasoftware.com>
 *
 */

#ifndef __Layer_2_bridging_H
#define __Layer_2_bridging_H

extern DMOBJ tLayer2BridgingObj[];
extern DMOBJ tBridge_VlanObj[];
extern DMLEAF tbridge_vlanParam[];
extern DMLEAF tlayer2_bridgeParam[];
extern DMLEAF tlayer2_markingParam[];
extern DMLEAF tavailableinterfaceParam[];
bool check_init_layer2(struct dmctx *dmctx, void *data);

#endif
