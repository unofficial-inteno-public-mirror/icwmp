/*
    cwmp_value_change.c

    cwmp service client in C

--------------------------------------------------------------------------------
cwmp service client
Copyright (C) 2011-2012, Inteno, Inc. All Rights Reserved.

Any distribution, dissemination, modification, conversion, integral or partial
reproduction, can not be made without the prior written permission of Inteno.
--------------------------------------------------------------------------------
Author contact information:

--------------------------------------------------------------------------------
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdarg.h>
#include "cwmp_lib.h"

int main (int argc, char **argv)
{
    FILE                *fp;
    register int        i, s, len;
    struct sockaddr_un  saun;
    char                buf[256];


    if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
    {
        printf("[CWMP] Error when creating process from api CLI: Quit\r\n");
        return 1;
    }

    saun.sun_family = AF_UNIX;
    strcpy(saun.sun_path, AF_UNIX_ADDRESS);

    len = sizeof(saun.sun_family) + strlen(saun.sun_path);

    if (connect(s, (const struct sockaddr *) &saun, len) < 0)
    {
        printf("[CWMP] Error when creating socket from api CLI: Quit\r\n");
        return 1;
    }
    printf("[CWMP] Send Value change to the cwmp client from api CLI\r\n");
    for (i=1;i<argc;i++)
    {
        sprintf(buf,"%s\n",argv[i]);
        send(s, buf, strlen(buf), 0);
    }
    sprintf(buf,"%s\n",AF_UNIX_END_DATA);
    send(s, buf, strlen(buf), 0);

    close(s);

    return 0;
}
