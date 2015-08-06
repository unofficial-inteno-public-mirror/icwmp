#ifndef __WEPKEY_H__
#define __WEPKEY_H__
#include "md5.h"
void wepkey64(char *passphrase, char strk64[4][11]);
void wepkey128(char *passphrase, char strk128[27]);
#endif /*__WEPKEY_H__*/
