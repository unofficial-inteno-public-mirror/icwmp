/*
    connectionRequest.c

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

#include "cwmp.h"
#include "httpda.h"

struct event_container *cwmp_add_event_container (struct cwmp *cwmp, int event_idx, char *command_key);

static bool get_connection_request = FALSE;

void *thread_connection_request_listener (void *v)
{
    struct cwmp             *cwmp = (struct cwmp *) v;
    struct event_container  *event_container;
    struct soap             soap;
    SOAP_SOCKET             m,s;
    int                     port,valid_socket;

    port = cwmp->conf.connection_request_port;
    soap_init(&soap);
    soap_register_plugin(&soap, http_da);


    soap.send_timeout   = 30;
    soap.recv_timeout   = 30;
    soap.accept_timeout = 864000;

    m = soap_bind(&soap, NULL, port, 100);
    if (!soap_valid_socket(m))
    {
        CWMP_LOG(ERROR,"Soap socket bind error in the connection request listener thread. Quit the thread!");
        soap_destroy(&soap);
        soap_end(&soap);
        soap_done(&soap);
        return NULL;
    }
    CWMP_LOG(INFO,"Bind the connection request listener, port=%d.",port);
    for (;;)
    {
        get_connection_request = FALSE;
        s = soap_accept(&soap);
        soap_valid_socket(s);
        soap_serve(cwmp,&soap);
        soap_closesock(&soap);
        soap_end(&soap);
        if (!get_connection_request)
        {
            continue;
        }
        CWMP_LOG(INFO,"Connection Request thread: add connection request event in the queue");
        pthread_mutex_lock (&(cwmp->mutex_session_queue));
        event_container = cwmp_add_event_container (cwmp, EVENT_IDX_6CONNECTION_REQUEST, "");
        pthread_mutex_unlock (&(cwmp->mutex_session_queue));
        pthread_cond_signal(&(cwmp->threshold_session_send));
    }
    soap_done(&soap);
    return CWMP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 soap_serve_request(struct cwmp *cwmp, struct soap *soap)
{
    char    *userid,*passwd;
    char    *authrealm  = "Inteno CWMP";

    userid  = cwmp->conf.cpe_userid;
    passwd  = cwmp->conf.cpe_passwd;
    if (userid==NULL || passwd==NULL)
    {
        return soap->error=503;
    }
    if (soap->authrealm && soap->userid)
    {
        if (!strcmp(soap->authrealm, authrealm) && !strcmp(soap->userid, userid))
        {
            if (!http_da_verify_get(soap, passwd)) // HTTP POST DA verification
            {
                soap_end_send(soap);
                get_connection_request = TRUE;
                return SOAP_OK;
            }
        }
    }
    soap->authrealm = authrealm; // realm to send to client
    return soap->error=401; // Not authorized, challenge with digest authentication
}

SOAP_FMAC5 int SOAP_FMAC6 soap_serve(struct cwmp *cwmp, struct soap *soap)
{
    static int      cr_request = 0;
    static time_t   restrict_start_time = 0;
    time_t          current_time;

    current_time = time(NULL);
    if ((restrict_start_time==0) || ((current_time-restrict_start_time) > CONNECTION_REQUEST_RESTRICT_PERIOD))
    {
        restrict_start_time = current_time;
        cr_request          = 1;
    }
    else
    {
        cr_request++;
        if (cr_request>CONNECTION_REQUEST_RESTRICT_REQUEST)
        {
            restrict_start_time = current_time;
            soap->error = 503;
            return soap_send_fault(soap);
        }
    }

    soap_begin_recv(soap);

    if (soap_serve_request(cwmp,soap) ||
        soap_response(soap, SOAP_OK)  ||
        soap_end_send(soap))
    {
        return soap_send_fault(soap);
    }

    return SOAP_OK;
}

