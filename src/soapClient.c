/*
    soapClient.c

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
#include "soapH.h"
#include "cwmp.h"
#include "cwmpBinding.nsmap"
#include "httpda.h"
#ifdef __cplusplus
extern "C" {
#endif

SOAP_SOURCE_STAMP("@(#) soapClient.c ver 2.7.12 2011-10-17 13:06:56 GMT")

static struct http_da_info http_da_info;

struct rpc_cpe *cwmp_add_session_rpc_cpe_getRPCMethods (struct session *session);
struct rpc_cpe *cwmp_add_session_rpc_cpe_reboot (struct session *session);
struct rpc_cpe *cwmp_add_session_rpc_cpe_download (struct session *session);
struct rpc_cpe *cwmp_add_session_rpc_cpe_getParameterValues (struct session *session);
struct rpc_cpe *cwmp_add_session_rpc_cpe_setParameterValues (struct session *session);
struct rpc_cpe *cwmp_add_session_rpc_cpe_getParameterNames (struct session *session);
struct rpc_cpe *cwmp_add_session_rpc_cpe_factoryReset (struct session *session);
struct rpc_cpe *cwmp_add_session_rpc_cpe_scheduleInform (struct session *session);
struct rpc_cpe *cwmp_add_session_rpc_cpe_getParameterAttributes (struct session *session);
struct rpc_cpe *cwmp_add_session_rpc_cpe_setParameterAttributes (struct session *session);
struct rpc_cpe *cwmp_add_session_rpc_cpe_addObject (struct session *session);
struct rpc_cpe *cwmp_add_session_rpc_cpe_deleteObject (struct session *session);

const struct CPE_METHOD_CONSTRUCTORS    CPE_METHOD_CONSTRUCTORS_ARRAY [] = {
    {"GetRPCMethods",           cwmp_add_session_rpc_cpe_getRPCMethods},
    {"SetParameterValues",      cwmp_add_session_rpc_cpe_setParameterValues},
    {"GetParameterValues",      cwmp_add_session_rpc_cpe_getParameterValues},
    {"GetParameterNames",       cwmp_add_session_rpc_cpe_getParameterNames},
    {"SetParameterAttributes",  cwmp_add_session_rpc_cpe_setParameterAttributes},
    {"GetParameterAttributes",  cwmp_add_session_rpc_cpe_getParameterAttributes},
    {"AddObject",               cwmp_add_session_rpc_cpe_addObject},
    {"DeleteObject",            cwmp_add_session_rpc_cpe_deleteObject},
    {"Reboot",                  cwmp_add_session_rpc_cpe_reboot},
    {"Download",                cwmp_add_session_rpc_cpe_download},
    {"Upload",                  NULL},
    {"FactoryReset",            cwmp_add_session_rpc_cpe_factoryReset},
    {"GetQueuedTransfers",      NULL},
    {"GetAllQueuedTransfers",   NULL},
    {"ScheduleInform",          cwmp_add_session_rpc_cpe_scheduleInform},
    {"SetVouchers",             NULL},
    {"GetOptions",              NULL}
};

struct rpc_cpe *cwmp_add_session_rpc_cpe_Fault (struct session *session, int idx);

void cwmp_session_soap_init (struct session *session)
{
    soap_init2(&session->soap, SOAP_IO_KEEPALIVE, SOAP_IO_KEEPALIVE);
    soap_set_omode(&session->soap, SOAP_XML_INDENT);
    session->soap.recv_timeout      = SOAP_TIMEOUT;
    session->soap.send_timeout      = SOAP_TIMEOUT;
    session->soap.connect_timeout   = SOAP_TIMEOUT;
    session->soap.accept_timeout    = SOAP_TIMEOUT;
    if (soap_register_plugin(&session->soap, http_da))
    {
        CWMP_LOG (ERROR,"Failed to register HTTP authentication plugin");
    }
    CWMP_LOG (INFO,"Open session");
    CWMP_LOG (INFO,"ACS URL: %s",session->acs_url);
}

void cwmp_session_done (struct session *session)
{
    soap_done(&session->soap);
    http_da_release(&session->soap, &http_da_info);
    CWMP_LOG (INFO,"Close session");
}

void cwmp_session_soap_destroy_end (struct session *session)
{
    soap_destroy(&session->soap);
    soap_end(&session->soap);
    if (session->digest_auth == TRUE)
    {
        http_da_restore(&session->soap, &http_da_info);
    }
}

int cwmp_soap_set_header_rpc_acs (struct soap *soap)
{
    static int                          id=MIN_INT_ID;
    static char                         buf_id[16];
    static struct SOAP_ENV__Header      soap_header;
    memset(&soap_header, 0, sizeof(struct SOAP_ENV__Header));
    if(id>=MAX_INT_ID)
    {
        id=MIN_INT_ID;
    }
    id++;
    sprintf(buf_id,"%d",id);
    soap_header.ID.__item                   = buf_id;
    soap_header.ID.SOAP_ENV__mustUnderstand = "1";
    soap->header                            = &soap_header;
    return CWMP_OK;
}

int cwmp_soap_get_header_rpc_cpe (struct soap *soap)
{
    static struct SOAP_ENV__Header      soap_header;
    static char                         *buf_id=NULL;

    if (soap->header == NULL)
    {
        soap->header = &soap_header;
        return CWMP_OK;
    }

    memset(&soap_header, 0, sizeof(struct SOAP_ENV__Header));
    if (buf_id!=NULL)
    {
        free(buf_id);
        buf_id = NULL;
    }
    buf_id = strdup(soap->header->ID.__item);
    soap_header.ID.__item                   = buf_id;
    soap_header.ID.SOAP_ENV__mustUnderstand = "1";
    soap->header                            = NULL;
    return CWMP_OK;
}

SOAP_FMAC3 char * SOAP_FMAC4 soap_get_fault_code(struct soap *soap)
{
    struct SOAP_ENV__Fault              *soap_fault;
    struct _cwmp1__Fault                *fault=NULL;
    struct SOAP_ENV__Detail             *detail=NULL;

    soap_fault = soap->fault;

    if (soap_fault == NULL)
    {
        return NULL;
    }

    if (soap_fault->detail!=NULL)
    {
        detail = soap_fault->detail;
    }
    else
    {
        detail = soap_fault->SOAP_ENV__Detail;
    }
    if (detail!=NULL)
    {
        fault = (struct _cwmp1__Fault *) detail->fault;
        if (fault!=NULL)
        {
            if (fault->FaultCode!=NULL)
            {
                return fault->FaultCode;
            }
        }
    }
    return NULL;
}

SOAP_FMAC5 int SOAP_FMAC6 cwmp_soap_call_rpc_acs
    (
    struct cwmp     *cwmp,
    struct session  *session,
    struct rpc_acs  *rpc_acs
    )
{
    struct soap                     *soap;
    struct soap_cwmp1_methods__rpc  *soap_methods;
    char                            *soap_fault_code;
    int                             u=0,r=0;

    soap_methods        = &(rpc_acs->soap_methods);
    soap                = &(session->soap);
    soap->encodingStyle = ENCODING_STYLE_URL;

    while (1)
    {
        session->error = CWMP_OK;
        cwmp_soap_set_header_rpc_acs(soap);
        soap_begin(soap);
        cwmp_soap_send_rpc_acs (cwmp, session, rpc_acs, soap, soap_methods);
        if (rpc_acs->error != CWMP_SUCCESS_RPC)
        {
            session->error = CWMP_RETRY_SESSION;
            break;
        }
        if (soap_begin_recv(soap) ||
            cwmp_soap_receive_rpc_acs_response (cwmp, session, rpc_acs, soap, soap_methods))
        {
            if (soap->error == 401) // challenge: HTTP authentication required
            {
                CWMP_LOG(INFO,"Receive http 401: need authentication");
                if(u>0)
                {
                    session->error = CWMP_RETRY_SESSION;
                    break;
                }
                u++;
                // to store userid and passwd
                http_da_save(soap, &http_da_info, soap->authrealm, cwmp->conf.acs_userid, cwmp->conf.acs_passwd); // set userid and passwd for this realm
                session->digest_auth = TRUE;
                cwmp_session_soap_destroy_end(session);
            }
            else if ((soap_fault_code = soap_get_fault_code(soap))!=NULL)
            {

                if (strcmp(soap_fault_code,"8005") == 0)
                {
                    CWMP_LOG(INFO,"Receive soap 8005: need to re-send");
                    if (r>5)
                    {
                        session->error = CWMP_RETRY_SESSION;
                        break;
                    }
                    r++;
                    cwmp_session_soap_destroy_end(session);
                }
                else if (rpc_acs->type != RPC_ACS_INFORM_IDX)
                {
                    rpc_acs->error = CWMP_SUCCESS_RPC;
                    session->error = CWMP_CONTINUE_SESSION;
                    break;
                }
                else
                {
                    session->error = CWMP_RETRY_SESSION;
                    break;
                }
            }
            else
            {
                session->error = CWMP_RETRY_SESSION;
                break;
            }
        }
        else
        {
            break;
        }
    }
    return rpc_acs->error;
}

SOAP_FMAC5 int SOAP_FMAC6 cwmp_soap_send_rpc_acs
    (
    struct cwmp     *cwmp,
    struct session  *session,
    struct rpc_acs  *rpc_acs,
    struct soap     *soap,
    struct soap_cwmp1_methods__rpc *soap_methods
    )
{
    char                *soap_endpoint;
    char                *soap_action = "";

    soap_endpoint = session->acs_url;

    soap_serializeheader(soap);
    soap_methods->soap_serialize_cwmp1__send_data (soap, rpc_acs->method_data);
    if (soap_begin_count(soap))
    {
        rpc_acs->error = CWMP_FAIL_RPC;
        return soap->error;
    }
    if (soap->mode & SOAP_IO_LENGTH)
    {   if (soap_envelope_begin_out(soap)
         || soap_putheader(soap)
         || soap_body_begin_out(soap)
         || soap_methods->soap_put_cwmp1__send_data(soap, rpc_acs->method_data, soap_methods->envelope,"")
         || soap_body_end_out(soap)
         || soap_envelope_end_out(soap))
        {
            rpc_acs->error = CWMP_FAIL_RPC;
            return soap->error;
        }
    }
    if (soap_end_count(soap))
    {
        rpc_acs->error = CWMP_FAIL_RPC;
        return soap->error;
    }

    if (soap_connect(soap, soap_endpoint, soap_action))
    {
        rpc_acs->error = CWMP_RETRY_RPC;
        return soap->error;
    }
    if (soap_envelope_begin_out(soap)
     || soap_putheader(soap)
     || soap_body_begin_out(soap)
     || soap_methods->soap_put_cwmp1__send_data(soap, rpc_acs->method_data, soap_methods->envelope,"")
     || soap_body_end_out(soap)
     || soap_envelope_end_out(soap))
    {
        rpc_acs->error = CWMP_FAIL_RPC;
        return soap->error;
    }

    if (soap_end_send(soap))
    {
        rpc_acs->error = CWMP_RETRY_RPC;
        CWMP_LOG (ERROR,"Problem when trying to send SOAP RPC to the ACS");
        return soap->error;
    }
    CWMP_LOG (INFO,"Send SOAP RPC to the ACS");
    rpc_acs->error = CWMP_SUCCESS_RPC;
    return soap->error;
}
SOAP_FMAC5 int SOAP_FMAC6 cwmp_soap_receive_rpc_acs_response
    (
    struct cwmp     *cwmp,
    struct session  *session,
    struct rpc_acs  *rpc_acs,
    struct soap     *soap,
    struct soap_cwmp1_methods__rpc *soap_methods
    )
{

    if (soap_envelope_begin_in(soap)
     || soap_recv_header(soap)  /* KMD IN */
     || soap_body_begin_in(soap))
    {
        rpc_acs->error = CWMP_RETRY_RPC;
        CWMP_LOG (ERROR,"Problem when receiving SOAP RPC response from the ACS");
        return soap->error;
    }
    soap_methods->soap_get_cwmp1__rpc_received_data (soap, rpc_acs->method_response_data, soap_methods->envelope_response,"");
    CWMP_LOG (INFO,"Receive SOAP RPC response from the ACS");
    if (soap->error)
    {
        rpc_acs->error = CWMP_RETRY_RPC;
        if (soap->error == SOAP_TAG_MISMATCH && soap->level == 2)
            return soap_recv_fault(soap);
        return soap->error;
    }
    if (soap_body_end_in(soap)
     || soap_envelope_end_in(soap)
     || soap_end_recv(soap))
    {
        return soap->error;
    }
    session->hold_request = (soap->header==NULL) ?  FALSE : soap->header->HoldRequests.__item;
    rpc_acs->error = CWMP_SUCCESS_RPC;
    return soap->error;
}

SOAP_FMAC5 int SOAP_FMAC6 cwmp_soap_send_http_empty (struct cwmp *cwmp, struct session *session)
{
    struct soap         *soap;
    char                *soap_action = "";
    char                *soap_endpoint;

    soap            = &(session->soap);
    soap_endpoint   = session->acs_url;
    soap_begin(soap);

    if (soap_connect(soap, soap_endpoint, soap_action) ||
        soap_send(soap, "") ||
        soap_end_send(soap))
    {
        session->error = CWMP_RETRY_SESSION;
        CWMP_LOG(ERROR,"Fail when trying to send HTTP empty");
        return soap->error;
    }

    CWMP_LOG(INFO,"Send HTTP empty");
    return CWMP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 cwmp_soap_recv_http_empty (struct cwmp *cwmp, struct session *session)
{
    struct soap         *soap;
    static int          uc=0;

    soap            = &(session->soap);

    soap_begin_recv(soap);

    if (soap->error == 401) // challenge: HTTP authentication required
    {
        if(uc>0)
        {
            session->error = CWMP_RETRY_SESSION;
            uc = 0;
            return CWMP_UNAUTHORIZED_401;
        }
        uc++;
        // to store userid and passwd
        http_da_save(soap, &http_da_info, soap->authrealm, cwmp->conf.acs_userid, cwmp->conf.acs_passwd); // set userid and passwd for this realm
        session->digest_auth = TRUE;
        return CWMP_UNAUTHORIZED_401;
    }
    uc = 0;
    if (soap->status == 204)
    {
        CWMP_LOG (INFO,"Receive HTTP 204 No Content");
        session->error          = CWMP_SUCCESS_SESSION;
        session->hold_request   = FALSE;
        return CWMP_OK;
    }
    else if (soap->status == 200)
    {
        CWMP_LOG (INFO,"Receive HTTP 200 OK");
        session->error = CWMP_CONTINUE_SESSION;
        return CWMP_OK;
    }

    return CWMP_OK;
}


struct rpc_cpe  *cwmp_soap_receive_rpc_cpe (
    struct cwmp     *cwmp,
    struct session  *session
    )
{
    struct soap                     *soap;
    int                             i;
    struct rpc_cpe                  *rpc_cpe=NULL;
    struct soap_cwmp1_methods__rpc  *soap_methods;
    char                            *method;
    struct rpc_cpe                  *(*constructor)(struct session *session);

    soap            = &(session->soap);

    if (soap_envelope_begin_in(soap)||
        soap_recv_header(soap)      ||
        soap_body_begin_in(soap))
    {
        CWMP_LOG (ERROR,"Problem when receiving SOAP RPC from ACS");
        return NULL;
    }
    soap_peek_element(soap);
    cwmp_soap_get_header_rpc_cpe (soap);
    CWMP_LOG (INFO,"Receive SOAP RPC from ACS: %s", soap->tag);
    for (i=0;i<sizearray(CPE_METHOD_CONSTRUCTORS_ARRAY);i++)
    {
        method      = CPE_METHOD_CONSTRUCTORS_ARRAY[i].METHOD;
        constructor = CPE_METHOD_CONSTRUCTORS_ARRAY[i].CONSTRUCTOR;
        if (!soap_match_tag(soap, soap->tag, method))
        {
            if (constructor!=NULL)
            {
                rpc_cpe = constructor(session);
                if (rpc_cpe == NULL)
                {
                    return NULL;
                }
                CWMP_LOG (INFO,"%s RPC is supported",soap->tag);
            }
            break;
        }
    }
    if (rpc_cpe == NULL)
    {
        CWMP_LOG (INFO,"%s RPC is not supported",soap->tag);
        rpc_cpe = cwmp_add_session_rpc_cpe_Fault(session,FAULT_CPE_METHOD_NOT_SUPPORTED_IDX);
        return rpc_cpe;
    }

    soap_methods = &(rpc_cpe->soap_methods);
    if (!soap_methods->soap_get_cwmp1__rpc_received_data(soap, rpc_cpe->method_data, soap_methods->envelope, NULL))
    {
        /*error*/
    }

    if (soap_body_end_in(soap)
        || soap_envelope_end_in(soap)
        || soap_end_recv(soap))
    {
        /*error*/
    }

    session->hold_request = (soap->header==NULL) ?  FALSE : soap->header->HoldRequests.__item;

    return rpc_cpe;
}

SOAP_FMAC5 int SOAP_FMAC6 cwmp_soap_send_rpc_cpe_response
    (
    struct cwmp     *cwmp,
    struct session  *session,
    struct rpc_cpe  *rpc_cpe
    )
{
    char                            *soap_endpoint;
    char                            *soap_action = "";
    struct soap_cwmp1_methods__rpc  *soap_methods;
    struct soap                     *soap;

    soap_endpoint           = session->acs_url;
    soap_methods            = &(rpc_cpe->soap_methods);
    soap                    = &(session->soap);

    soap_begin(soap);
    cwmp_soap_get_header_rpc_cpe (soap);
    soap_serializeheader(soap);
    soap_methods->soap_serialize_cwmp1__send_data (soap, rpc_cpe->method_response_data);
    if (soap_begin_count(soap))
    {
        rpc_cpe->error = CWMP_FAIL_RPC;
        return soap->error;
    }
    if (soap->mode & SOAP_IO_LENGTH)
    {   if (soap_envelope_begin_out(soap)
         || soap_putheader(soap)
         || soap_body_begin_out(soap)
         || soap_methods->soap_put_cwmp1__send_data(soap, rpc_cpe->method_response_data, soap_methods->envelope_response,"")
         || soap_body_end_out(soap)
         || soap_envelope_end_out(soap))
        {
            rpc_cpe->error = CWMP_FAIL_RPC;
            return soap->error;
        }
    }
    if (soap_end_count(soap))
    {
        rpc_cpe->error = CWMP_FAIL_RPC;
        return soap->error;
    }

    if (soap_connect(soap, soap_endpoint, soap_action))
    {
        rpc_cpe->error = CWMP_RETRY_RPC;
        return soap->error;
    }
    if (soap_envelope_begin_out(soap)
     || soap_putheader(soap)
     || soap_body_begin_out(soap)
     || soap_methods->soap_put_cwmp1__send_data(soap, rpc_cpe->method_response_data, soap_methods->envelope_response,"")
     || soap_body_end_out(soap)
     || soap_envelope_end_out(soap))
    {
        rpc_cpe->error = CWMP_FAIL_RPC;
        return soap->error;
    }

    if (soap_end_send(soap))
    {
        CWMP_LOG (ERROR,"Problem when trying to send SOAP RPC response to the ACS");
        rpc_cpe->error = CWMP_RETRY_RPC;
        return soap->error;
    }
    CWMP_LOG (INFO,"Send SOAP RPC response to the ACS");
    return CWMP_OK;
}

#ifdef __cplusplus
}
#endif
/* End of soapClient.c */

