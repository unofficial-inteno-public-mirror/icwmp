/*
    SetParameterAttributes.c

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
#include "list.h"
#include "dm.h"
#include "dm_rpc.h"

struct rpc_cpe *cwmp_add_session_rpc_cpe (struct session *session);
struct rpc_cpe *cwmp_add_session_rpc_cpe_setParameterAttributes (struct session *session);
int cwmp_rpc_cpe_setParameterAttributes (struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
int cwmp_rpc_cpe_setParameterAttributes_response_data_init(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
int cwmp_rpc_cpe_setParameterAttributes_response(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
int cwmp_rpc_cpe_setParameterAttributes_end(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
int cwmp_session_rpc_cpe_destructor (struct cwmp *cwmp, struct session *session, struct rpc_cpe *rpc_cpe);
struct rpc_cpe *cwmp_add_session_rpc_cpe_Fault (struct session *session, int idx);
int cwmp_dm_setParameterAttributes(struct cwmp *cwmp, struct cwmp1__SetParameterAttributesStruct *ParameterAttributesStruct);
int cwmp_delete_notification(char *type,char *name);
int cwmp_add_notification(char *type,char *name);
int cwmp_check_notification(char *name, char *type);
int cwmp_off_notification(struct dm_node *node,char *name, enum xsd__boolean notification_change);
int cwmp_apply_passive_notification(struct dm_node *node,char *name, enum xsd__boolean notification_change);
int cwmp_apply_active_notification(struct dm_node *node,char *name, enum xsd__boolean notification_change);
int cwmp_add_in_accesslist(char *name, enum xsd__boolean access_list_change, char *accessList);
int uci_delete_value(char *cmd);
int uci_revert_value ();
int uci_commit_value ();

extern const struct CPE_METHOD_CONSTRUCTORS CPE_METHOD_CONSTRUCTORS_ARRAY [COUNT_RPC_CPE];

const struct ACCESSLIST_CONST_STRUCT    ACCESSLIST_CONST_ARRAY [] = {
    {"Subscriber","cwmp.accesslist.subscriber"}
};

struct rpc_cpe *cwmp_add_session_rpc_cpe_setParameterAttributes (struct session *session)
{
    struct rpc_cpe                  *rpc_cpe;
    struct soap_cwmp1_methods__rpc  *soap_methods;

    rpc_cpe = cwmp_add_session_rpc_cpe (session);
    if (rpc_cpe == NULL)
    {
        return NULL;
    }
    soap_methods                                    = &(rpc_cpe->soap_methods);
    rpc_cpe->method_data                            = (void *) calloc (1,sizeof(struct _cwmp1__SetParameterAttributes));
    rpc_cpe->method                                 = cwmp_rpc_cpe_setParameterAttributes;
    rpc_cpe->method_response_data                   = (void *) calloc (1,sizeof(struct _cwmp1__SetParameterAttributesResponse));
    rpc_cpe->method_response_data_init              = cwmp_rpc_cpe_setParameterAttributes_response_data_init;
    rpc_cpe->method_response                        = cwmp_rpc_cpe_setParameterAttributes_response;
    rpc_cpe->method_end                             = cwmp_rpc_cpe_setParameterAttributes_end;
    rpc_cpe->destructor                             = cwmp_session_rpc_cpe_destructor;
    soap_methods->envelope                          = "cwmp:SetParameterAttributes";
    soap_methods->envelope_response                 = "cwmp:SetParameterAttributesResponse";
    soap_methods->soap_serialize_cwmp1__send_data   = (void *) soap_serialize__cwmp1__SetParameterAttributesResponse;
    soap_methods->soap_put_cwmp1__send_data         = (void *) soap_put__cwmp1__SetParameterAttributesResponse;
    soap_methods->soap_get_cwmp1__rpc_received_data = (void *) soap_get__cwmp1__SetParameterAttributes;
    return rpc_cpe;
}

int cwmp_rpc_cpe_setParameterAttributes (struct cwmp *cwmp, struct session *session, struct rpc_cpe *this)
{

    struct _cwmp1__SetParameterAttributes           *p_soap_cwmp1__SetParameterAttributes;
    int                                             i,size,error;
    struct cwmp1__SetParameterAttributesStruct      *ParameterAttributesStruct,**ptrParameterAttributeStruct;

    p_soap_cwmp1__SetParameterAttributes        = (struct _cwmp1__SetParameterAttributes *)this->method_data;
    size                                        = p_soap_cwmp1__SetParameterAttributes->ParameterList->__size;
    ptrParameterAttributeStruct                 = p_soap_cwmp1__SetParameterAttributes->ParameterList->__ptrSetParameterAttributesStruct;

    for (i=0;i<size;i++,ptrParameterAttributeStruct++)
    {
        ParameterAttributesStruct = *ptrParameterAttributeStruct;
        if(error = cwmp_dm_setParameterAttributes(cwmp,ParameterAttributesStruct))
        {
            uci_revert_value ();
            if (cwmp_add_session_rpc_cpe_Fault(session,error) == NULL)
            {
                return CWMP_GEN_ERR;
            }
            return CWMP_FAULT_CPE;
        }
    }

    uci_commit_value();
    return CWMP_OK;
}

int cwmp_rpc_cpe_setParameterAttributes_response_data_init(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this)
{
    return CWMP_OK;
}

int cwmp_rpc_cpe_setParameterAttributes_response(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this)
{
    int error;
    CWMP_LOG (INFO,"Trying to send SetParameterAttributes response to the ACS");
    error = cwmp_soap_send_rpc_cpe_response (cwmp, session, this);
    return error;
}

int cwmp_rpc_cpe_setParameterAttributes_end(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this)
{
    return CWMP_OK;
}

int cwmp_add_notification(char *type,char *name)
{
    char                                            cmd[256];
    int                                             error;

    sprintf(cmd,"%s=%s",type,name);
    if(error = uci_add_list_value(cmd))
    {
        return error;
    }
    return CWMP_OK;
}

int cwmp_delete_notification(char *type,char *name)
{
    struct config_uci_list                          *notification;
    struct list_head                                *list;
    LIST_HEAD(notification_list);

    uci_get_list_value(type, &notification_list);
    uci_delete_value(type);

    while(!list_empty(&notification_list))
    {
        notification = list_entry (notification_list.next, struct config_uci_list, list);
        if((notification != NULL)&&(strcmp(name,notification->value) != 0)&&(strstr(notification->value,name) == NULL))
        {
            cwmp_add_notification(type,notification->value);
        }
        list_del(notification_list.next);
        if(notification != NULL)
        {
            if(notification->value != NULL)
            {
                free(notification->value);
            }
            free(notification);
            notification = NULL;
        }
    }
    return CWMP_OK;
}

int cwmp_check_notification(char *name, char *type)
{
    struct config_uci_list                          *notification;
    bool                                            partial_found = FALSE;
    bool                                            found = FALSE;
    int                                             is_parent = 0;
    LIST_HEAD(notification_list);

    uci_get_list_value(type, &notification_list);

    while(!list_empty(&notification_list))
    {
        notification = list_entry (notification_list.next, struct config_uci_list, list);
        if((notification != NULL))
        {
            if(strstr(notification->value,name) != NULL)
            {
                partial_found = TRUE;
                is_parent = 1;
                if(strcmp(notification->value,name) == 0)
                {
                    found = TRUE;
                }
            }
            if(strstr(name,notification->value) != NULL)
            {
                partial_found = TRUE;
            }
            list_del(notification_list.next);
            free(notification);
            notification = NULL;
        }
    }
    if(!partial_found)
    {
        return CWMP_OK;
    }
    else
    {
        if(found)
        {
            return CWMP_EXACT_FOUND;
        }
        else
        {
            if(is_parent == 1)
            {
                return CWMP_CHILD_FOUND;
            }
            else
            {
                return CWMP_PARENT_FOUND;
            }
        }
    }
}

int cwmp_apply_active_notification(struct dm_node *node,char *name, enum xsd__boolean notification_change)
{
    int                 error = FAULT_CPE_NO_FAULT_IDX,i;

    if(node != NULL)
    {
        if(node->active_notify.can_deny)
        {
            error = FAULT_CPE_NOTIFICATION_REJECTED_IDX;
        }
        else
        {
            if(notification_change == xsd__boolean__true_)
            {
                if(cwmp_check_notification(name,UCI_NOTIFICATION_PASSIVE_PATH) == CWMP_CHILD_FOUND ||
                   cwmp_check_notification(name,UCI_NOTIFICATION_PASSIVE_PATH) == CWMP_EXACT_FOUND  )
                {
                    cwmp_delete_notification(UCI_NOTIFICATION_PASSIVE_PATH,name);
                }
                if(cwmp_check_notification(name,UCI_NOTIFICATION_DENIED_PATH) == CWMP_CHILD_FOUND||
                   cwmp_check_notification(name,UCI_NOTIFICATION_DENIED_PATH) == CWMP_EXACT_FOUND)
                {
                    cwmp_delete_notification(UCI_NOTIFICATION_DENIED_PATH,name);
                }
                if(cwmp_check_notification(name,UCI_NOTIFICATION_ACTIVE_PATH) == CWMP_OK)
                {
                    cwmp_add_notification(UCI_NOTIFICATION_ACTIVE_PATH,name);
                }
                else if(cwmp_check_notification(name,UCI_NOTIFICATION_ACTIVE_PATH) == CWMP_CHILD_FOUND)
                {
                    cwmp_delete_notification(UCI_NOTIFICATION_ACTIVE_PATH,name);
                    cwmp_add_notification(UCI_NOTIFICATION_ACTIVE_PATH,name);
                }
            }
        }
    }
    else
    {
        error = FAULT_CPE_INVALID_PARAMETER_NAME_IDX;
    }
    return error;
}

int cwmp_apply_passive_notification(struct dm_node *node,char *name, enum xsd__boolean notification_change)
{
    int                 error = FAULT_CPE_NO_FAULT_IDX,i;

    if(node != NULL)
    {
        if(node->active_notify.force_enabled)
        {
            error = FAULT_CPE_NOTIFICATION_REJECTED_IDX;
        }
        else
        {
            if(notification_change == xsd__boolean__true_)
            {
                if(cwmp_check_notification(name,UCI_NOTIFICATION_ACTIVE_PATH) == CWMP_CHILD_FOUND ||
                   cwmp_check_notification(name,UCI_NOTIFICATION_ACTIVE_PATH) == CWMP_EXACT_FOUND   )
                {
                    cwmp_delete_notification(UCI_NOTIFICATION_ACTIVE_PATH,name);
                }
                if(cwmp_check_notification(name,UCI_NOTIFICATION_DENIED_PATH) == CWMP_CHILD_FOUND||
                   cwmp_check_notification(name,UCI_NOTIFICATION_DENIED_PATH) == CWMP_EXACT_FOUND)
                {
                    cwmp_delete_notification(UCI_NOTIFICATION_DENIED_PATH,name);
                }
                if(cwmp_check_notification(name,UCI_NOTIFICATION_PASSIVE_PATH) == CWMP_OK)
                {
                    cwmp_add_notification(UCI_NOTIFICATION_PASSIVE_PATH,name);
                }
                else if(cwmp_check_notification(name,UCI_NOTIFICATION_PASSIVE_PATH) == CWMP_CHILD_FOUND)
                {
                    cwmp_delete_notification(UCI_NOTIFICATION_PASSIVE_PATH,name);
                    cwmp_add_notification(UCI_NOTIFICATION_PASSIVE_PATH,name);
                }
            }
        }
    }
    else
    {
        error = FAULT_CPE_INVALID_PARAMETER_NAME_IDX;
    }
    return error;
}

int cwmp_off_notification(struct dm_node *node,char *name, enum xsd__boolean notification_change)
{
    int                 error = FAULT_CPE_NO_FAULT_IDX,i;

    if(node != NULL)
    {
        if(node->active_notify.force_enabled)
        {
            error = FAULT_CPE_NOTIFICATION_REJECTED_IDX;
        }
        else
        {
            if(notification_change == xsd__boolean__true_)
            {
                switch(cwmp_check_notification(name,UCI_NOTIFICATION_PASSIVE_PATH))
                {
                    case CWMP_EXACT_FOUND:
                    case CWMP_CHILD_FOUND:
                        cwmp_delete_notification(UCI_NOTIFICATION_PASSIVE_PATH,name);
                        break;
                    case CWMP_PARENT_FOUND:
                        if(cwmp_check_notification(name,UCI_NOTIFICATION_DENIED_PATH) == CWMP_OK)
                        {
                            cwmp_add_notification(UCI_NOTIFICATION_DENIED_PATH,name);
                        }
                        break;
                }
                switch(cwmp_check_notification(name,UCI_NOTIFICATION_ACTIVE_PATH))
                {
                    case CWMP_EXACT_FOUND:
                    case CWMP_CHILD_FOUND:
                        cwmp_delete_notification(UCI_NOTIFICATION_ACTIVE_PATH,name);
                        break;
                    case CWMP_PARENT_FOUND:
                        if(cwmp_check_notification(name,UCI_NOTIFICATION_DENIED_PATH) == CWMP_OK)
                        {
                            cwmp_add_notification(UCI_NOTIFICATION_DENIED_PATH,name);
                        }
                        break;
                }
                for(i=0;i<COUNT_ACCESSLIST;i++)
                {
                    switch(cwmp_check_notification(name,ACCESSLIST_CONST_ARRAY[i].UCI_ACCESSLIST_PATH))
                    {
                        case CWMP_EXACT_FOUND:
                        case CWMP_CHILD_FOUND:
                            cwmp_delete_notification(ACCESSLIST_CONST_ARRAY[i].UCI_ACCESSLIST_PATH,name);
                            break;
                        case CWMP_PARENT_FOUND:
                            break;
                    }
                }
            }
        }
    }
    else
    {
        error = FAULT_CPE_INVALID_PARAMETER_NAME_IDX;
    }
    return error;
}

int cwmp_add_in_accesslist(char *name, enum xsd__boolean access_list_change, char *accessList)
{
    int                 i;
    bool                found = FALSE;

    for(i=0;i<COUNT_ACCESSLIST;i++)
    {
        if(strcmp(accessList,ACCESSLIST_CONST_ARRAY[i].NAME) == 0)
        {
            found = TRUE;
            break;
        }
    }

    if(!found)
    {
        return FAULT_CPE_INVALID_ARGUMENTS_IDX;
    }

    if(access_list_change == xsd__boolean__true_)
    {
        if(cwmp_check_notification(name,ACCESSLIST_CONST_ARRAY[i].UCI_ACCESSLIST_PATH) == CWMP_OK)
        {
            cwmp_add_notification(ACCESSLIST_CONST_ARRAY[i].UCI_ACCESSLIST_PATH,name);
            return FAULT_CPE_NO_FAULT_IDX;
        }
        if(cwmp_check_notification(name,ACCESSLIST_CONST_ARRAY[i].UCI_ACCESSLIST_PATH) == CWMP_CHILD_FOUND)
        {
            cwmp_delete_notification(ACCESSLIST_CONST_ARRAY[i].UCI_ACCESSLIST_PATH,name);
            cwmp_add_notification(ACCESSLIST_CONST_ARRAY[i].UCI_ACCESSLIST_PATH,name);
            return FAULT_CPE_NO_FAULT_IDX;
        }
    }

    return FAULT_CPE_NO_FAULT_IDX;
}
