/*
    SetParameterValues.c

    cwmp service client in C

--------------------------------------------------------------------------------
cwmp service client
Copyright (C) 20011-2012, Inteno, Inc. All Rights Reserved.

Any distribution, dissemination, modification, conversion, integral or partial
reproduction, can not be made without the prior written permission of Inteno.
--------------------------------------------------------------------------------
Author contact information:

--------------------------------------------------------------------------------
*/

#include "soapH.h"
#include "cwmp.h"
#include "external.h"


struct rpc_cpe *cwmp_add_session_rpc_cpe (struct session *session);
struct rpc_cpe *cwmp_add_session_rpc_cpe_setParameterValues (struct session *session);
int cwmp_rpc_cpe_setParameterValues (struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
int cwmp_rpc_cpe_setParameterValues_response_data_init(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
int cwmp_rpc_cpe_setParameterValues_response(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
int cwmp_rpc_cpe_setParameterValues_end(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
struct rpc_cpe *cwmp_add_session_rpc_cpe_Fault (struct session *session, int idx);
int cwmp_session_rpc_cpe_destructor (struct cwmp *cwmp, struct session *session, struct rpc_cpe *rpc_cpe);
static int check_duplicated_parameter_name (struct cwmp1__ParameterValueStruct **ptr_ParameterValueStruct, int size);
extern struct FAULT_CPE FAULT_CPE_ARRAY [FAULT_CPE_ARRAY_SIZE];
extern struct list_head external_list_parameter;

struct rpc_cpe *cwmp_add_session_rpc_cpe_setParameterValues (struct session *session)
{
    struct rpc_cpe                  *rpc_cpe;
    struct soap_cwmp1_methods__rpc  *soap_methods;

    rpc_cpe = cwmp_add_session_rpc_cpe (session);
    if (rpc_cpe == NULL)
    {
        return NULL;
    }
    soap_methods                                    = &(rpc_cpe->soap_methods);
    rpc_cpe->method_data                            = (void *) calloc (1,sizeof(struct _cwmp1__SetParameterValues));
    rpc_cpe->method                                 = cwmp_rpc_cpe_setParameterValues;
    rpc_cpe->method_response_data                   = (void *) calloc (1,sizeof(struct _cwmp1__SetParameterValuesResponse));
    rpc_cpe->method_response_data_init              = cwmp_rpc_cpe_setParameterValues_response_data_init;
    rpc_cpe->method_response                        = cwmp_rpc_cpe_setParameterValues_response;
    rpc_cpe->method_end                             = cwmp_rpc_cpe_setParameterValues_end;
    rpc_cpe->destructor                             = cwmp_session_rpc_cpe_destructor;
    soap_methods->envelope                          = "cwmp:SetParameterValues";
    soap_methods->envelope_response                 = "cwmp:SetParameterValuesResponse";
    soap_methods->soap_serialize_cwmp1__send_data   = (void *) soap_serialize__cwmp1__SetParameterValuesResponse;
    soap_methods->soap_put_cwmp1__send_data         = (void *) soap_put__cwmp1__SetParameterValuesResponse;
    soap_methods->soap_get_cwmp1__rpc_received_data = (void *) soap_get__cwmp1__SetParameterValues;
    return rpc_cpe;
}

int cwmp_rpc_cpe_setParameterValues (struct cwmp *cwmp, struct session *session, struct rpc_cpe *this)
{
    struct _cwmp1__SetParameterValues           *p_soap_cwmp1__SetParameterValues;
    struct _cwmp1__SetParameterValuesResponse   *p_soap_cwmp1__SetParameterValuesResponse;
    struct cwmp1ParameterValueList              *ParameterList;
    struct cwmp1__ParameterValueStruct          **ptr_ParameterValueStruct,*ParameterValueStruct;
    struct list_head 							*list;
    struct external_parameter					*external_parameter;
    char                                        *parameter_key,buf[128],*status=NULL;
    int                                         i,n,size,error;
    struct rpc_cpe                  			*rpc_cpe_fault;

    p_soap_cwmp1__SetParameterValues            = (struct _cwmp1__SetParameterValues *)this->method_data;
    p_soap_cwmp1__SetParameterValuesResponse    = (struct _cwmp1__SetParameterValuesResponse *)this->method_response_data;
    size                                        = p_soap_cwmp1__SetParameterValues->ParameterList->__size;
    ptr_ParameterValueStruct                    = p_soap_cwmp1__SetParameterValues->ParameterList->__ptrParameterValueStruct;
    parameter_key                               = p_soap_cwmp1__SetParameterValues->ParameterKey;

    if (error = check_duplicated_parameter_name (ptr_ParameterValueStruct, size))
    {
        if (cwmp_add_session_rpc_cpe_Fault(session,FAULT_CPE_INVALID_ARGUMENTS_IDX)==NULL)
        {
            return CWMP_GEN_ERR;
        }
        return CWMP_FAULT_CPE;
    }

    for (i=0;i<size;i++,ptr_ParameterValueStruct++)
    {
        ParameterValueStruct = *ptr_ParameterValueStruct;
        CWMP_LOG(INFO,"param[%d] Name = %s, Value = %s",i,ParameterValueStruct->Name,(strlen(ParameterValueStruct->Value)<1000)?ParameterValueStruct->Value:"(Unable to display big stings)");

        if (external_set_action_write("value",ParameterValueStruct->Name, ParameterValueStruct->Value, NULL))
		{
        	external_free_list_parameter();
        	if (cwmp_add_session_rpc_cpe_Fault(session,FAULT_CPE_INTERNAL_ERROR_IDX)==NULL)
			{
				return CWMP_GEN_ERR;
			}
			return CWMP_FAULT_CPE;
		}
    }
    if (external_set_action_execute("value"))
	{
    	external_free_list_parameter();
    	if (cwmp_add_session_rpc_cpe_Fault(session,FAULT_CPE_INTERNAL_ERROR_IDX)==NULL)
		{
			return CWMP_GEN_ERR;
		}
		return CWMP_FAULT_CPE;
	}
    list_for_each(list,&external_list_parameter) {
		external_parameter = list_entry(list, struct external_parameter, list);
		if (external_parameter->fault_code &&
				external_parameter->fault_code[0]=='9')
		{
			error = FAULT_CPE_INTERNAL_ERROR_IDX;
			for (i=1; i<FAULT_CPE_ARRAY_SIZE; i++)
			{
				if (strcmp(external_parameter->fault_code, FAULT_CPE_ARRAY[i].CODE)==0)
				{
					error = i;
					break;
				}
			}
			if ((rpc_cpe_fault = cwmp_add_session_rpc_cpe_Fault(session,error))==NULL)
			{
				return CWMP_GEN_ERR;
			}
			cwmp_rpc_cpe_Fault_parameter_cause(rpc_cpe_fault,external_parameter->name);
			external_free_list_parameter();
			return CWMP_FAULT_CPE;
		}
	}

	external_fetch_setParamValRespStatus(&status);

	if (!status)
	{
		if ((rpc_cpe_fault = cwmp_add_session_rpc_cpe_Fault(session,FAULT_CPE_INTERNAL_ERROR_IDX))==NULL)
		{
			return CWMP_GEN_ERR;
		}
		return CWMP_FAULT_CPE;
	}

	sprintf(buf,"%s=%s",UCI_ACS_PARAMETERKEY_PATH,parameter_key);
	uci_set_value (buf);
	uci_commit_value();

	p_soap_cwmp1__SetParameterValuesResponse->Status = atoi(status);

	free(status);

    return CWMP_OK;
}

int cwmp_rpc_cpe_setParameterValues_response_data_init(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this)
{
    return CWMP_OK;
}
int cwmp_rpc_cpe_setParameterValues_response(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this)
{
    int error;
    CWMP_LOG (INFO,"Trying to send SetParameterValues response to the ACS");
    error = cwmp_soap_send_rpc_cpe_response (cwmp, session, this);
    return error;
}
int cwmp_rpc_cpe_setParameterValues_end(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this)
{
    return CWMP_OK;
}

static int check_duplicated_parameter_name (struct cwmp1__ParameterValueStruct **ptr_ParameterValueStruct, int size)
{

    struct cwmp1__ParameterValueStruct **p, **q, *pvs, *qvs;
    int                                 i,j;
    for (i=0,p=ptr_ParameterValueStruct;i<(size-1);i++,p++)
    {
        pvs = *p;
        for (j=i+1,q=p+1;j<size;j++,q++)
        {
            qvs = *q;
            if (strcmp(pvs->Name,qvs->Name)==0)
            {
                return CWMP_GEN_ERR;
            }
        }
    }
    return CWMP_OK;
}
