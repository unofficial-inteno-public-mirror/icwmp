/*
    Fault_cpe.c

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
#include "list.h"

#define FAULT_CPE_TYPE_CLIENT   0
#define FAULT_CPE_TYPE_SERVER   1

char *FAULT_CPE_ARRAY_TYPE [] = {
    [FAULT_CPE_TYPE_CLIENT] = "Client",
    [FAULT_CPE_TYPE_SERVER] = "Server"
};

struct FAULT_CPE FAULT_CPE_ARRAY [] = {
    [FAULT_CPE_NO_FAULT_IDX]                            = {NULL,  0,                     NULL},
    [FAULT_CPE_METHOD_NOT_SUPPORTED_IDX]                = {"9000",FAULT_CPE_TYPE_SERVER, "Method not supported"},
    [FAULT_CPE_REQUEST_DENIED_IDX]                      = {"9001",FAULT_CPE_TYPE_SERVER, "Request denied (no reason specified)"},
    [FAULT_CPE_INTERNAL_ERROR_IDX]                      = {"9002",FAULT_CPE_TYPE_SERVER, "Internal error"},
    [FAULT_CPE_INVALID_ARGUMENTS_IDX]                   = {"9003",FAULT_CPE_TYPE_CLIENT, "Invalid arguments"},
    [FAULT_CPE_RESOURCES_EXCEEDED_IDX]                  = {"9004",FAULT_CPE_TYPE_SERVER, "Resources exceeded"},
    [FAULT_CPE_INVALID_PARAMETER_NAME_IDX]              = {"9005",FAULT_CPE_TYPE_CLIENT, "Invalid parameter name"},
    [FAULT_CPE_INVALID_PARAMETER_TYPE_IDX]              = {"9006",FAULT_CPE_TYPE_CLIENT, "Invalid parameter type"},
    [FAULT_CPE_INVALID_PARAMETER_VALUE_IDX]             = {"9007",FAULT_CPE_TYPE_CLIENT, "Invalid parameter value"},
    [FAULT_CPE_NON_WRITABLE_PARAMETER_IDX]              = {"9008",FAULT_CPE_TYPE_CLIENT, "Attempt to set a non-writable parameter"},
    [FAULT_CPE_NOTIFICATION_REJECTED_IDX]               = {"9009",FAULT_CPE_TYPE_SERVER, "Notification request rejected"},
    [FAULT_CPE_DOWNLOAD_FAILURE_IDX]                    = {"9010",FAULT_CPE_TYPE_SERVER, "Download failure"},
    [FAULT_CPE_UPLOAD_FAILURE_IDX]                      = {"9011",FAULT_CPE_TYPE_SERVER, "Upload failure"},
    [FAULT_CPE_FILE_TRANSFER_AUTHENTICATION_FAILURE_IDX]= {"9012",FAULT_CPE_TYPE_SERVER, "File transfer server authentication failure"},
    [FAULT_CPE_FILE_TRANSFER_UNSUPPORTED_PROTOCOL_IDX]  = {"9013",FAULT_CPE_TYPE_SERVER, "Unsupported protocol for file transfer"},
    [FAULT_CPE_DOWNLOAD_FAIL_MULTICAST_GROUP_IDX]       = {"9014",FAULT_CPE_TYPE_SERVER, "Download failure: unable to join multicast group"},
    [FAULT_CPE_DOWNLOAD_FAIL_CONTACT_SERVER_IDX]        = {"9015",FAULT_CPE_TYPE_SERVER, "Download failure: unable to contact file server"},
    [FAULT_CPE_DOWNLOAD_FAIL_ACCESS_FILE_IDX]           = {"9016",FAULT_CPE_TYPE_SERVER, "Download failure: unable to access file"},
    [FAULT_CPE_DOWNLOAD_FAIL_COMPLETE_DOWNLOAD_IDX]     = {"9017",FAULT_CPE_TYPE_SERVER, "Download failure: unable to complete download"},
    [FAULT_CPE_DOWNLOAD_FAIL_FILE_CORRUPTED_IDX]        = {"9018",FAULT_CPE_TYPE_SERVER, "Download failure: file corrupted"},
    [FAULT_CPE_DOWNLOAD_FAIL_FILE_AUTHENTICATION_IDX]   = {"9019",FAULT_CPE_TYPE_SERVER, "Download failure: file authentication failure"}
};

struct rpc_cpe *cwmp_add_session_rpc_cpe (struct session *session);

int cwmp_rpc_cpe_fault (struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
int cwmp_rpc_cpe_fault_response_data_init (struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
int cwmp_rpc_cpe_fault_response (struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
int cwmp_rpc_cpe_fault_end (struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
int cwmp_rpc_cpe_fault_destructor (struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);

SOAP_FMAC3 void SOAP_FMAC4 soap_serialize_SOAP_ENV__Fault(struct soap *soap, const struct SOAP_ENV__Fault *a);
SOAP_FMAC3 int SOAP_FMAC4 soap_put_SOAP_ENV__Fault(struct soap *soap, const struct SOAP_ENV__Fault *a, const char *tag, const char *type);

struct rpc_cpe *cwmp_add_session_rpc_cpe_Fault (struct session *session, int idx)
{
    struct rpc_cpe                  *rpc_cpe;
    struct soap_cwmp1_methods__rpc  *soap_methods;
    struct fault                    *fault;

    rpc_cpe = cwmp_add_session_rpc_cpe (session);
    if (rpc_cpe == NULL)
    {
        return NULL;
    }
    soap_methods                                    = &(rpc_cpe->soap_methods);
    fault                                           = &(rpc_cpe->fault);
    rpc_cpe->method_data                            = NULL;
    rpc_cpe->method                                 = cwmp_rpc_cpe_fault;
    rpc_cpe->method_response_data                   = (void *) calloc (1,sizeof(struct SOAP_ENV__Fault));
    rpc_cpe->method_response_data_init              = cwmp_rpc_cpe_fault_response_data_init;
    rpc_cpe->method_response                        = cwmp_rpc_cpe_fault_response;
    rpc_cpe->method_end                             = cwmp_rpc_cpe_fault_end;
    rpc_cpe->destructor                             = cwmp_rpc_cpe_fault_destructor;
    soap_methods->envelope                          = "";
    soap_methods->envelope_response                 = "SOAP-ENV:Fault";
    soap_methods->soap_serialize_cwmp1__send_data   = (void *) soap_serialize_SOAP_ENV__Fault;
    soap_methods->soap_put_cwmp1__send_data         = (void *) soap_put_SOAP_ENV__Fault;
    soap_methods->soap_get_cwmp1__rpc_received_data = NULL;
    fault->code_idx                                 = idx;
    return rpc_cpe;
}

int cwmp_rpc_cpe_fault (struct cwmp *cwmp, struct session *session, struct rpc_cpe *this)
{
    return CWMP_OK;
}

int cwmp_rpc_cpe_fault_response_data_init (struct cwmp *cwmp, struct session *session, struct rpc_cpe *this)
{

    struct SOAP_ENV__Fault              *p_soap_Fault;
    struct _cwmp1__Fault                *fault;
    struct fault                        *rpc_fault;
    struct SOAP_ENV__Detail             *detail;
    int                                 type_idx;

    if (session==NULL || this==NULL)
    {
        return CWMP_GEN_ERR;
    }

    p_soap_Fault                = (struct SOAP_ENV__Fault *) this->method_response_data;
    rpc_fault                   = &(this->fault);
    type_idx                    = FAULT_CPE_ARRAY[rpc_fault->code_idx].TYPE;
    p_soap_Fault->faultcode     = FAULT_CPE_ARRAY_TYPE[type_idx];
    p_soap_Fault->faultstring   = "CWMP fault";
    p_soap_Fault->detail        = (struct SOAP_ENV__Detail *) calloc(1, sizeof(struct SOAP_ENV__Detail));
    if (p_soap_Fault->detail == NULL)
    {
        return CWMP_MEM_ERR;
    }
    detail                      = p_soap_Fault->detail;
    detail->__type              = SOAP_TYPE__cwmp1__Fault;
    detail->fault               = (struct _cwmp1__Fault *) calloc(1, sizeof(struct _cwmp1__Fault));
    if (detail->fault == NULL)
    {
        return CWMP_MEM_ERR;
    }
    fault                       = (struct _cwmp1__Fault *) detail->fault;
    fault->FaultCode            = FAULT_CPE_ARRAY[rpc_fault->code_idx].CODE;
    fault->FaultString          = FAULT_CPE_ARRAY[rpc_fault->code_idx].DESCRIPTION;
    if (rpc_fault->parameter_cause!=NULL)
    {
    	fault->SetParameterValuesFault	= (struct _cwmp1__Fault_SetParameterValuesFault *) calloc(1, sizeof(struct _cwmp1__Fault_SetParameterValuesFault));
    	fault->SetParameterValuesFault->FaultCode		= strdup(FAULT_CPE_ARRAY[rpc_fault->code_idx].CODE);
    	fault->SetParameterValuesFault->FaultString		= strdup(FAULT_CPE_ARRAY[rpc_fault->code_idx].DESCRIPTION);
    	fault->SetParameterValuesFault->ParameterName	= rpc_fault->parameter_cause;
    	fault->__sizeSetParameterValuesFault			= 1;
    }

    return CWMP_OK;
}

int cwmp_rpc_cpe_fault_response (struct cwmp *cwmp, struct session *session, struct rpc_cpe *this)
{
    int error;
    CWMP_LOG (INFO,"Trying to send Fault response to the ACS");
    error = cwmp_soap_send_rpc_cpe_response (cwmp, session, this);
    return error;
}

int cwmp_rpc_cpe_Fault_parameter_cause (struct rpc_cpe *this, char *parameter)
{
	this->fault.parameter_cause = strdup(parameter);
	return CWMP_OK;
}

int cwmp_rpc_cpe_fault_end (struct cwmp *cwmp, struct session *session, struct rpc_cpe *this)
{
    struct SOAP_ENV__Fault              *p_soap_Fault;
    struct _cwmp1__Fault                *fault;
    struct SOAP_ENV__Detail             *detail;

    p_soap_Fault = (struct SOAP_ENV__Fault *) this->method_response_data;
    if (p_soap_Fault!=NULL)
    {
        detail = p_soap_Fault->detail;
        if (detail!=NULL)
        {
            fault = (struct _cwmp1__Fault *)detail->fault;
            if (fault->SetParameterValuesFault!=NULL)
            {
            	if (fault->SetParameterValuesFault->FaultCode!=NULL)
            	{
            		free (fault->SetParameterValuesFault->FaultCode);
            	}
            	if (fault->SetParameterValuesFault->FaultString!=NULL)
				{
					free (fault->SetParameterValuesFault->FaultString);
				}
            	free (fault->SetParameterValuesFault);
            }
            if (fault!=NULL)
            {
                free (fault);
            }
            free (detail);
        }
    }

    return CWMP_OK;
}

int cwmp_rpc_cpe_fault_destructor (struct cwmp *cwmp, struct session *session, struct rpc_cpe *this)
{
    struct SOAP_ENV__Fault              *p_soap_Fault;

    p_soap_Fault = (struct SOAP_ENV__Fault *) this->method_response_data;
    if (p_soap_Fault!=NULL)
    {
        free (p_soap_Fault);
    }
    if (this->fault.parameter_cause!=NULL)
    {
    	free (this->fault.parameter_cause);
    }
    list_del(&(this->list));
    free (this);
    return CWMP_OK;
}
