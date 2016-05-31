/*
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Copyright (C) 2014 Inteno Broadband Technology AB
 *    Author Mohamed Kallel <mohamed.kallel@pivasoftware.com>
 */

#include <stdio.h>
#include <zlib.h>
#include <stdlib.h>
#include <string.h>
#include "log.h"
#include "cwmp.h"

/* CHUNK is the size of the memory chunk used by the zlib routines. */

#define CHUNK 0x4000

/* These are parameters to deflateInit2. See
   http://zlib.net/manual.html for the exact meanings. */

#define windowBits 15
#define GZIP_ENCODING 16

static int strm_init (z_stream * strm, int type)
{
    int error = 0;
    strm->zalloc = Z_NULL;
    strm->zfree  = Z_NULL;
    strm->opaque = Z_NULL;
    switch (type) {
        case COMP_GZIP:
         error = deflateInit2(strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
                                 windowBits | GZIP_ENCODING, 8,
                                 Z_DEFAULT_STRATEGY);
        break;
        case COMP_DEFLATE:
            error = deflateInit(strm, Z_DEFAULT_COMPRESSION);
        break;
    }
    if (error < 0) {
        CWMP_LOG(ERROR, "error in zlib compress in deflateInit2");
        return -1;
    }
    return 0;
}

/* Example text to print out. */
int zlib_compress (char *message, unsigned char **zmsg, int *zlen, int type)
{
#if 0 /*test*/
    static int testi = 1;
    char tests[50];
    sprintf(tests, "/tmp/test%d", testi++);
    FILE *fp = fopen (tests, "wb");
#endif /*test*/
    unsigned char out[CHUNK];
    z_stream strm={0};
    unsigned char *rzmsg = NULL;
    *zmsg = NULL;
    *zlen=0;
    if (strm_init(&strm, type))
        return -1;
    strm.next_in = (unsigned char *) message;
    strm.avail_in = strlen(message);
    do {
        int have;
        strm.avail_out = CHUNK;
        strm.next_out = out;
        int error = deflate(&strm, Z_FINISH);
        if (error < 0) {
            CWMP_LOG(ERROR, "error in zlib compress in deflate");
            return -1;
        }

        have = CHUNK - strm.avail_out;
        int ozlen = *zlen;
        *zlen += have;
        rzmsg = realloc(*zmsg, *zlen * sizeof(unsigned char));
        if (rzmsg!=NULL) {
            *zmsg = rzmsg;
        }
        else {
            free(*zmsg);
            CWMP_LOG(ERROR, "Error (re)allocating memory");
            return -1;
        }
        memcpy(*zmsg + ozlen, out, have);
    }
    while (strm.avail_out == 0);
    deflateEnd(&strm);
#if 0 /*test*/
    fwrite(*zmsg, sizeof (char), *zlen, fp);
    fclose(fp);
#endif /*test*/

    return 0;
}
