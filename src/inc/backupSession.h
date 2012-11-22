/*
    backupSession.h

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

#define RPC_NO_STATUS   -1
#define RPC_QUEUE       0
#define RPC_SEND        1

typedef struct attribute
{
    long int        id;
    char            *name;
    char            *status;
    char            *commandKey;
    char            *url;
} attribute;

typedef enum backup_loading {
    ALL,
    ACS
} backup_loading;
