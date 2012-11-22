/*
    cwmp.c

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
#include <linux/netlink.h>
#include <stdarg.h>
#include "soapH.h"
#include "cwmp.h"
#include "dm.h"
#include "dm_rpc.h"
#include "cwmp_kernel.h"
#include "cwmp_lib.h"

struct event_container *cwmp_add_event_container (struct cwmp *cwmp, int event_idx, char *command_key);

extern struct cwmp cwmp_main;

struct api_value_change_thread_input {
    struct cwmp         *cwmp;
    char                *path;
    struct sub_path     *sub_path;
    int                 sub_path_size;
};

void *thread_api_cwmp_value_change_parameters (void *v)
{
    struct api_value_change_thread_input    *t_input;
    struct cwmp                             *cwmp;
    struct event_container                  *event_container;
    int                                     error,i,n=0;
    bool                                    is_actif;


    t_input = (struct api_value_change_thread_input *)v;
    cwmp    = t_input->cwmp;
    pthread_mutex_lock (&(cwmp->api_value_change.mutex));
    error = cwmp_dm_getParameterPaths_by_correspondence(cwmp,t_input->path,t_input->sub_path,t_input->sub_path_size,&(cwmp->api_value_change.parameter_list),&n,FALSE,TRUE,&is_actif);
    cwmp->api_value_change.parameter_size += n;

    if (error==FAULT_CPE_NO_FAULT_IDX && is_actif)
    {
        pthread_mutex_lock (&(cwmp->mutex_session_queue));
        event_container = cwmp_add_event_container (cwmp, EVENT_IDX_4VALUE_CHANGE, "");
        if (event_container == NULL)
        {
            pthread_mutex_unlock (&(cwmp->mutex_session_queue));
            return NULL;
        }
        cwmp_save_event_container (cwmp,event_container);
        pthread_mutex_unlock (&(cwmp->mutex_session_queue));
        pthread_cond_signal(&(cwmp->threshold_session_send));
    }

    for (i=0;i<t_input->sub_path_size;i++)
    {
        if (t_input->sub_path[i].dm_indice.indice!=NULL)
        {
            free(t_input->sub_path[i].dm_indice.indice);
        }
    }
    free(t_input->sub_path);
    if (t_input->path!=NULL)
    {
        free(t_input->path);
    }

    free(t_input);
    pthread_mutex_unlock (&(cwmp->api_value_change.mutex));
    return NULL;
}

int api_cwmp_value_change_parameters (int argc, char **argv)
{
    struct cwmp                             *cwmp = &cwmp_main;
    struct api_value_change_thread_input    *t_input;
    va_list                                 args;
    pthread_t                               thread;
    int                                     error;

    if (argc < 1)
    {
        return CWMP_OK;
    }
    t_input             = calloc (1,sizeof(struct api_value_change_thread_input));
    t_input->sub_path   = calloc (DM_MAX_INDICE,sizeof(struct sub_path));
    t_input->cwmp       = cwmp;
    cwmp_dm_get_sub_indice_path(argc,argv,&(t_input->path),t_input->sub_path,&(t_input->sub_path_size));

    error = pthread_create(&thread, NULL, &thread_api_cwmp_value_change_parameters, (void *)t_input);
    if (error<0)
    {
        free (t_input->sub_path);
        free (t_input);
        CWMP_LOG(ERROR,"Error when creating a value change api thread!");
    }
    return CWMP_OK;
}


/* kernel api */

void *thread_kernel_api_cwmp_value_change_parameters (void *v)
{
    int                     i, argc, sock_fd;
    char                    **argv;
    struct sockaddr_nl      src_addr, dest_addr;
    struct nlmsghdr         *nlh = NULL;
    struct iovec            iov;
    struct msghdr           msg;


    sock_fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_USER);
    if(sock_fd<0)
    {
        CWMP_LOG(ERROR,"Error when creating NetLink socket to kernel: Quit kernel value change thread!");
        return NULL;
    }

    memset(&src_addr, 0, sizeof(src_addr));
    src_addr.nl_family  = AF_NETLINK;
    src_addr.nl_pid     = getpid();
    bind(sock_fd, (struct sockaddr*)&src_addr, sizeof(src_addr));

    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.nl_family = AF_NETLINK;
    dest_addr.nl_pid    = 0; /* For Linux Kernel */
    dest_addr.nl_groups = 0; /* unicast */

    nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(NETLINK_MAX_PAYLOAD));


    argv = calloc ((DM_MAX_INDICE+1),sizeof(char *));

    while (1)
    {
        memset(nlh, 0, NLMSG_SPACE(NETLINK_MAX_PAYLOAD));
        nlh->nlmsg_len      = NLMSG_SPACE(NETLINK_MAX_PAYLOAD);
        nlh->nlmsg_pid      = getpid();
        nlh->nlmsg_flags    = 0;

        iov.iov_base        = (void *)nlh;
        iov.iov_len         = nlh->nlmsg_len;
        msg.msg_name        = (void *)&dest_addr;
        msg.msg_namelen     = sizeof(dest_addr);
        msg.msg_iov         = &iov;
        msg.msg_iovlen      = 1;
        strcpy(NLMSG_DATA(nlh), NETLINK_CWMP_ID);
        sendmsg(sock_fd,&msg,0);
        argc = 0;
        while (argc<(DM_MAX_INDICE+1))
        {

            memset(nlh, 0, NLMSG_SPACE(NETLINK_MAX_PAYLOAD));
            nlh->nlmsg_len      = NLMSG_SPACE(NETLINK_MAX_PAYLOAD);
            nlh->nlmsg_pid      = getpid();
            nlh->nlmsg_flags    = 0;

            iov.iov_base        = (void *)nlh;
            iov.iov_len         = nlh->nlmsg_len;
            msg.msg_name        = (void *)&dest_addr;
            msg.msg_namelen     = sizeof(dest_addr);
            msg.msg_iov         = &iov;
            msg.msg_iovlen      = 1;
            /* Read message from kernel */
            recvmsg(sock_fd, &msg, 0);
            if (strcmp(NLMSG_DATA(nlh),NETLINK_END_DATA)==0)
            {
                break;
            }
            if (strcmp(NLMSG_DATA(nlh),NETLINK_NULL)==0)
            {
                argv[argc] = NULL;
            }
            else
            {
                argv[argc] = strdup (NLMSG_DATA(nlh));
            }

            argc++;
        }
        CWMP_LOG(INFO,"Receive Parameter Value change from Kernel space");
        api_cwmp_value_change_parameters(argc,argv);
        for (i=0;i<argc;i++)
        {
            free(argv[i]);
        }
    }
    free(argv);
    close(sock_fd);
    return NULL;
}

void *thread_lib_api_cwmp_value_change_parameters (void *v)
{
    char                    buffer[1024],**argv;
    FILE                    *fp;
    socklen_t				fromlen;
    int                     argc, size=1023;
    register int            i, s, ns, len;
    struct sockaddr_un      saun, fsaun;

    if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        CWMP_LOG(ERROR,"Error when creating process socket listener: Quit lib value change thread!");
        return NULL;
    }

    saun.sun_family = AF_UNIX;
    strcpy(saun.sun_path, AF_UNIX_ADDRESS);

    unlink(AF_UNIX_ADDRESS);
    len = sizeof(saun.sun_family) + strlen(saun.sun_path);

    if (bind(s, (const struct sockaddr *) &saun, len) < 0)
    {
        CWMP_LOG(ERROR,"Error when binding process socket: Quit lib value change thread!");
        return NULL;
    }

    /*
     * Listen on the socket.
     */
    if (listen(s, 5) < 0) {
        CWMP_LOG(ERROR,"Error when listening process socket: Quit lib value change thread!");
        return NULL;
    }

    argv = calloc ((DM_MAX_INDICE+1),sizeof(char *));

    while (1)
    {
    	fromlen = sizeof(fsaun);
    	if ((ns = accept(s, (struct sockaddr *) &fsaun, &fromlen)) < 0)
        {
            CWMP_LOG(ERROR,"Error when accepting process socket");
            continue;
        }
        fp = fdopen(ns, "r");
        argc = 0;

        while (fgets(buffer,sizeof(buffer),fp))
        {
            buffer[strlen(buffer)-1] = 0;
            if (strcmp(buffer,AF_UNIX_END_DATA)==0)
            {
                break;
            }
            if (strcmp(buffer,AF_UNIX_NULL)==0)
            {
                argv[argc] = NULL;
            }
            else
            {
                argv[argc] = strdup(buffer);
            }
            argc++;
        }
        CWMP_LOG(INFO,"Receive Parameter Value change from other process");
        api_cwmp_value_change_parameters(argc,argv);
        for (i=0;i<argc;i++)
        {
            if (argv[i]!=NULL)
            {
                free(argv[i]);
            }
        }
    }
    free(argv);
    close(s);
    return NULL;
}
