#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wepkey.h"

void wepkey64(char *passphrase, char strk64[4][11])
{
	unsigned char k64[4][5];
	unsigned char pseed[4] = {0};
    unsigned int randNumber, tmp;
    int i, j;

    for(i = 0; i < strlen(passphrase); i++)
    {
        pseed[i%4] ^= (unsigned char) passphrase[i];
    }

    randNumber = pseed[0] | (pseed[1] << 8) | (pseed[2] << 16) | (pseed[3] << 24);

    for (i = 0; i < 4; i++)
    {
        for (j = 0; j < 5; j++)
        {
            randNumber = (randNumber * 0x343fd + 0x269ec3) & 0xffffffff;
            tmp = (randNumber >> 16) & 0xff;
            k64[i][j] = (unsigned char) tmp;
        }
		sprintf(strk64[i], "%02X%02X%02X%02X%02X", k64[i][0], k64[i][1], k64[i][2], k64[i][3], k64[i][4]);
    }
}

void wepkey128(char *passphrase, char strk128[27])
{
    int i=0;
	char dup[65] = {0};
	unsigned char out[MD5_DIGEST_SIZE];
	struct MD5Context c;

	while (i<64) {
		strncpy(dup + i, passphrase, 64 - i);
    	i = strlen(dup);
	}

	MD5Init(&c);
	MD5Update(&c, dup, 64);
	MD5Final(out, &c);
	for(i=0; i<13; i++)
		sprintf(strk128 + 2 * i, "%02X", out[i]);
}

