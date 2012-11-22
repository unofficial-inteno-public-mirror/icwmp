/*
    log.c

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
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <time.h>
#include "cwmp.h"

#define DEFAULT_LOG_SEVERITY                    6
static char                                     *SEVERITY_NAMES[8] = {"[EMERG]  ","[ALERT]  ","[CRITIC] ","[ERROR]  ","[WARNING]","[NOTICE] ","[INFO]   ","[DEBUG]  "};
static int                                      log_severity = DEFAULT_LOG_SEVERITY;
static long int                                 log_max_size = DEFAULT_LOG_FILE_SIZE;
static char                                     log_file_name[256];
static bool                                     enable_log_file = TRUE;
static bool                                     enable_log_stdout = FALSE;
static pthread_mutex_t                          mutex_log;

int log_set_severity_idx (char *value)
{
    int i;
    for (i=0;i<8;i++)
    {
        if (strstr(SEVERITY_NAMES[i],value)!=NULL)
        {
            log_severity = i;
            return 0;
        }
    }
    return 1;
}

int log_set_log_file_name (char *value)
{
    if(value != NULL)
    {
        strcpy(log_file_name,value);
    }
    else
    {
        strcpy(log_file_name,DEFAULT_LOG_FILE_NAME);
    }
}
int log_set_file_max_size(char *value)
{
    if(value != NULL)
    {
        log_max_size  = atol(value);
    }
}
int log_set_on_console(char *value)
{
    if(strcmp(value,"enable") == 0)
    {
        enable_log_stdout = TRUE;
    }
    if(strcmp(value,"disable") == 0)
    {
        enable_log_stdout = FALSE;
    }
}
int log_set_on_file(char *value)
{
    if(strcmp(value,"enable") == 0)
    {
        enable_log_file = TRUE;
    }
    if(strcmp(value,"disable") == 0)
    {
        enable_log_file = FALSE;
    }
}

void puts_log(int severity, const char *fmt, ...)
{
    va_list         args;
    int             i;
    char            buf[1024];
    char            buf_file[1024];
    time_t          t;
    struct tm       *Tm;
    struct timeval  tv;
    FILE            *pLog;
    struct stat     st;
    long int        size = 0;
    char            log_file_name_bak[256];

    if (severity>log_severity)
    {
        return;
    }

    gettimeofday(&tv, 0);
    t   = time((time_t*)NULL);
    Tm= localtime(&tv.tv_sec);
    i   = sprintf(buf,"%02d-%02d-%4d, %02d:%02d:%02d.%03d %s ",
                    Tm->tm_mday,
                    Tm->tm_mon+1,
                    Tm->tm_year+1900,
                    Tm->tm_hour,
                    Tm->tm_min,
                    Tm->tm_sec,
                    (int)tv.tv_usec%1000,
                    SEVERITY_NAMES[severity]);

    if(strlen(log_file_name) == 0)
    {
        strcpy(log_file_name,DEFAULT_LOG_FILE_NAME);
    }

    if (stat(log_file_name, &st) == 0)
    {
        size = st.st_size;
    }

    if(enable_log_file)
    {
        pthread_mutex_lock (&mutex_log);
        if(size >= log_max_size)
        {
            sprintf(log_file_name_bak,"%s.1",log_file_name);
            rename(log_file_name,log_file_name_bak);
            pLog = fopen(log_file_name,"w");
        }
        else
        {
            pLog = fopen(log_file_name,"a+");
        }
    }

    va_start(args, fmt);
    i += vsprintf(buf+i, fmt, args);
    if(enable_log_file)
    {
        sprintf(buf_file,"%s\n",buf);
        fputs (buf_file, pLog);
    }
    va_end(args);
    if(enable_log_file)
    {
        fclose(pLog);
        pthread_mutex_unlock (&mutex_log);
    }
    if(enable_log_stdout)
    {
        puts(buf);
    }
}
