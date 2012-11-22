/*
    dm_rpc.c

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
#include <stdarg.h>
#include "soapH.h"
#include "cwmp.h"
#include "list.h"
#include "dm.h"
#include "dm_rpc.h"

extern struct list_head     head_dm_tree;
extern struct FAULT_CPE     FAULT_CPE_ARRAY [FAULT_CPE_ARRAY_SIZE];
extern struct list_head     list_dm_set_handler;

int get_node_path_name (char *name,
                        struct dm_node *node,
                        struct dm_rpc *dm_rpc,
                        struct dm_node_path *node_path,
                        int pos_np,
                        struct dm_index_path *index_path,
                        int pos_xp)
{
    char                    *ret;
    int                     i;
    struct dm_node_path     *enp;
    struct dm_index_path    *exp;
    for (i=0;i<pos_np;i++)
    {
        enp = &node_path[i];
        exp = enp->index_path;
        if (enp->node== NULL)
        {
            break;
        }
        strcat(name,enp->node->name);
        if (enp->node->type == DM_PARAMETER)
        {
            continue;
        }
        strcat(name,".");
        if (enp->node->type==DM_INSTANCE && exp!=NULL && exp->index!=NULL)
        {
            strcat(name,exp->index);
            strcat(name,".");
        }
    }
    return DM_OK;
}

/**
 -------------------------------------------------------
|           GetParameter Value Data Model API           |
 -------------------------------------------------------
**/
int get_node_paramater_value (
    struct dm_node *node,
    struct dm_index_path *index_path,
    int pos_xp,
    char **ret
    )
{
    struct data_handler         **data_handler;
    __u8                        i,size_dh;
    char                        corr[256];

    data_handler    = node->data_handler;
    size_dh         = node->size_data_handler;
    *ret            = NULL;
    for (i=0;i<size_dh;i++)
    {
        if (data_handler[i]->type == DM_UCI)
        {
            struct dm_uci   *uci;
            uci             = (struct dm_uci *)data_handler[i]->handler;
            get_string_correspondence (corr,uci->cmd,index_path,pos_xp-1);
            if (*ret !=NULL)
            {
                free (*ret);
            }
            uci_get_value (corr,ret);
            break;
        }
        else if (data_handler[i]->type == DM_SYSTEM)
        {
            struct dm_system            *system;
            system  = (struct dm_system *)data_handler[i]->handler;
            if (system->type != DM_GET)
            {
                continue;
            }
            get_string_correspondence (corr,system->cmd,index_path,pos_xp-1);
            if (*ret !=NULL)
            {
                free (*ret);
            }
            dm_get_system_data(corr,ret);
            break;
        }
        else if (data_handler[i]->type == DM_STRING || data_handler[i]->type == DM_DEFAULT_VALUE)
        {
            char                        *value;
            value   = (char *)data_handler[i]->handler;
            get_string_correspondence (corr,value,index_path,pos_xp-1);
            *ret    = strdup(corr);
        }
    }
    if (*ret == NULL)
    {
        return DM_ERR;
    }
    return DM_OK;
}
int dm_list_add_ParameterValues(char *name,
                                char *value,
                                struct dm_node *node,
                                struct list_head *list)
{
    struct cwmp1__ParameterValueStruct          *ParameterValueStruct;
    struct handler_ParameterValueStruct         *handler_ParameterValueStruct;
    struct dm_node_leaf                         *parameter_node = (struct dm_node_leaf *)node;
    extern char *TYPE_VALUES_ARRAY [COUNT_TYPE_VALUES];

    ParameterValueStruct            = calloc(1,sizeof(struct cwmp1__ParameterValueStruct));
    handler_ParameterValueStruct    = calloc(1,sizeof(struct handler_ParameterValueStruct));
    if (ParameterValueStruct == NULL ||
        handler_ParameterValueStruct == NULL)
    {
        return FAULT_CPE_INTERNAL_ERROR_IDX;
    }
    ParameterValueStruct->Name                          = strdup(name);
    ParameterValueStruct->Value                         = value;
    ParameterValueStruct->Type                          = TYPE_VALUES_ARRAY [parameter_node->value_type];
    handler_ParameterValueStruct->ParameterValueStruct  = ParameterValueStruct;
    list_add_tail(&handler_ParameterValueStruct->list,list);
    return FAULT_CPE_NO_FAULT_IDX;
}

int dm_node_getParameterValues (struct dm_node *node,
                                struct dm_rpc *dm_rpc,
                                struct dm_node_path *node_path,
                                int pos_np,
                                struct dm_index_path *index_path,
                                int pos_xp)
{

    struct dm_input_getParameterValues          *input;
    char                                        name[512],*value;
    int                                         len,error;

    input = (struct dm_input_getParameterValues *)dm_rpc->input;

    if (node == NULL || node->type == DM_OBJECT || node->type == DM_INSTANCE)
    {
        dm_rpc->subtree = TRUE;
        return FAULT_CPE_NO_FAULT_IDX;
    }
    dm_rpc->subtree = FALSE;
    name[0] = 0;
    if (error = get_node_path_name(name, node, dm_rpc, node_path, pos_np, index_path, pos_xp))
    {
        return FAULT_CPE_INVALID_PARAMETER_NAME_IDX;
    }
    if (error = get_node_paramater_value(node,index_path,pos_xp,&value))
    {
        char t[1];
        t[0] = 0;
        value = strdup(t);
    }

    dm_list_add_ParameterValues(name, value, node, input->list);
    *(input->n) +=1;

    return FAULT_CPE_NO_FAULT_IDX;
}

int cwmp_dm_getParameterValues(struct cwmp *cwmp, char *path, struct list_head *list, int *n)
{
    struct dm_rpc                               dm_rpc;
    struct dm_input_getParameterValues          input;
    int                                         error;

    memset(&dm_rpc,0,sizeof(struct dm_rpc));

    *n              = 0;
    input.list      = list;
    input.n         = n;
    dm_rpc.input    = &input;
    dm_rpc.method   = dm_node_getParameterValues;

    error = dm_browse_tree (path, &dm_rpc);

    return error;
}


/**
 -------------------------------------------------------
|           GetParameterName Data Model API             |
 -------------------------------------------------------
**/
int dm_list_add_ParameterName ( char *name,
                                struct dm_node *node,
                                struct list_head *list)
{
    struct cwmp1__ParameterInfoStruct           *ParameterInfoStruct;
    struct handler_ParameterInfoStruct          *handler_ParameterInfoStruct;
    int inode;

    ParameterInfoStruct         = calloc(1,sizeof(struct cwmp1__ParameterInfoStruct));
    handler_ParameterInfoStruct = calloc(1,sizeof(struct handler_ParameterInfoStruct));
    if (ParameterInfoStruct == NULL ||
        handler_ParameterInfoStruct == NULL)
    {
        return FAULT_CPE_INTERNAL_ERROR_IDX;
    }
    ParameterInfoStruct->Name                           = strdup(name);
    ParameterInfoStruct->Writable                       = (node->permission == DM_READ_WRITE || node->permission == DM_CREATE) ? xsd__boolean__true_ : xsd__boolean__false_;
    handler_ParameterInfoStruct->ParameterInfoStruct    = ParameterInfoStruct;
    list_add_tail(&handler_ParameterInfoStruct->list,list);
    return FAULT_CPE_NO_FAULT_IDX;
}

int dm_node_getParameterNames(  struct dm_node *node,
                                struct dm_rpc *dm_rpc,
                                struct dm_node_path *node_path,
                                int pos_np,
                                struct dm_index_path *index_path,
                                int pos_xp)
{

    struct dm_input_getParameterNames           *input;
    struct list_head                            *ilist,*list;
    struct dm_node                              *inode;
    char                                        name[512];
    int                                         len;

    input = (struct dm_input_getParameterNames *)dm_rpc->input;

    if (node == NULL)
    {
        if (input->NextLevel == xsd__boolean__false_)
        {
            dm_rpc->subtree = TRUE;
            return FAULT_CPE_NO_FAULT_IDX;
        }
        else
        {
            list    = &head_dm_tree;
            name[0] = 0;
            len     = 0;
            dm_rpc->subtree = FALSE;
        }
    }
    else
    {
        if (input->NextLevel == xsd__boolean__true_ &&
            node->type == DM_PARAMETER)
        {
            return FAULT_CPE_INVALID_ARGUMENTS_IDX;
        }

        if (input->NextLevel == xsd__boolean__false_ &&
            (node->type == DM_OBJECT ||
            node->type == DM_INSTANCE))
        {
            dm_rpc->subtree = TRUE;
        }
        else
        {
            dm_rpc->subtree = FALSE;
        }
        name[0] = 0;
        get_node_path_name(name, node, dm_rpc, node_path, pos_np, index_path, pos_xp);

        len = strlen(name);
        list = &(node->head_child);
    }
    if (node!=NULL && input->NextLevel == xsd__boolean__false_)
    {
        dm_list_add_ParameterName(name, node, input->list);
        *(input->n) +=1;
    }
    else if (node!=NULL && node->type==DM_INSTANCE && index_path[pos_xp-1].index==NULL)
    {
        struct dm_data                              *data=NULL;
        unsigned short                              i,ilen,ic;
        if (get_data_array_indexes (index_path,pos_xp-1,&data,&ic))
        {
            return FAULT_CPE_NO_FAULT_IDX;
        }
        for (i=0;i<data->line_size;i++)
        {
            ilen = strlen(data->data[i][ic]);
            memcpy(name+len,data->data[i][ic],ilen);
            name[len+ilen] = '.';
            name[len+ilen+1] = 0;
            dm_list_add_ParameterName(name, node, input->list);
            *(input->n) +=1;
        }
    }
    else
    {
        struct dm_node  *inode;
        int             i,ilen;
        __list_for_each(ilist,list)
        {
            inode = list_entry(ilist, struct dm_node, list);
            DM_XML_GET_HANDLER_DEBUG(inode,index_path,pos_xp);
            ilen = strlen(inode->name);
            memcpy(name+len,inode->name,ilen);
            if (inode->type == DM_PARAMETER)
            {
                name[len+ilen] = 0;
            }
            else
            {
                name[len+ilen] = '.';
                name[len+ilen+1] = 0;
            }
            dm_list_add_ParameterName(name, inode, input->list);
            *(input->n) +=1;
        }
    }

    return FAULT_CPE_NO_FAULT_IDX;
}

int cwmp_dm_getParameterNames(struct cwmp *cwmp, char *path, struct list_head *list, int *n, enum xsd__boolean NextLevel)
{
    struct dm_rpc                               dm_rpc;
    struct dm_input_getParameterNames           input;
    int                                         error;

    memset(&dm_rpc,0,sizeof(struct dm_rpc));

    *n              = 0;
    input.list      = list;
    input.n         = n;
    input.NextLevel = NextLevel;
    dm_rpc.input    = &input;
    dm_rpc.method   = dm_node_getParameterNames;

    error = dm_browse_tree (path, &dm_rpc);

    return error;
}

/**
 -------------------------------------------------------
|  get parameter paths by correspondence Data Model API |
 -------------------------------------------------------
**/

static int check_index_correspondence ( struct dm_node_path *node_path,
                                        int pos_np,
                                        struct dm_index_path *index_path,
                                        int pos_xp,
                                        struct dm_input_getParameterPaths_by_correspondence *input,
                                        int n)
{
    struct dm_indice            *dm_indice;
    struct dm_node_path         *enp;
    struct dm_index_path        *exp;


    dm_indice   = &(input->sub_path[n].dm_indice);
    if (dm_indice->indice==NULL)
    {
        return DM_OK;
    }
    enp = &node_path[pos_np-1];
    exp = enp->index_path;
    if (exp->index!=NULL)
    {
        if (dm_indice->type==DM_INDICE_INDEX)
        {
            if(strcmp(dm_indice->indice,exp->index)!=0)
            {
                return DM_ERR;
            }
        }
        else if (dm_indice->type==DM_INDICE_CORRESPONDENCE)
        {
            struct dm_data                              *data=NULL;
            unsigned short                              cc;
            if (get_data_array_correspondence_column(index_path,pos_xp-1,&data,&cc))
            {
                return DM_ERR;
            }
            if ((exp->indice<0) || strcmp(dm_indice->indice,data->data[exp->indice][cc])!=0)
            {
                return DM_ERR;
            }
        }
    }
    return DM_OK;
}

int dm_node_getParameterPaths_by_correspondence(struct dm_node *node,
                                                struct dm_rpc *dm_rpc,
                                                struct dm_node_path *node_path,
                                                int pos_np,
                                                struct dm_index_path *index_path,
                                                int pos_xp)
{

    struct dm_input_getParameterPaths_by_correspondence         *input;
    char                                                        name[512];
    int                                                         i,error;
    char                                                        *value=NULL;
    bool                                                        add_parameter=FALSE;
    struct list_head                                            *ilist;

    input = (struct dm_input_getParameterPaths_by_correspondence *)dm_rpc->input;

    if (node == NULL)
    {
        return FAULT_CPE_INVALID_PARAMETER_NAME_IDX;
    }

    for (i=0;i<input->sub_path_size;i++)
    {
        if (input->sub_path[i].node==node)
        {
            if (node->type == DM_PARAMETER)
            {
                add_parameter = TRUE;
                break;
            }
            else
            {
                if (check_index_correspondence(node_path,pos_np,index_path,pos_xp,input,i))
                {
                    dm_rpc->subtree = FALSE;
                    return FAULT_CPE_NO_FAULT_IDX;
                }
                dm_rpc->subtree = TRUE;
                return FAULT_CPE_NO_FAULT_IDX;
            }
        }
        else if (input->sub_path[i].node==NULL)
        {
            if (i==0)
            {
                input->sub_path[i].name = node->name;
                input->sub_path[i].node = node;
                dm_rpc->subtree     = TRUE;
                return FAULT_CPE_NO_FAULT_IDX;
            }
            else if (strcmp(input->sub_path[i].name,node->name)==0)
            {
                input->sub_path[i].node = node;
                if (node->type != DM_PARAMETER)
                {
                    if (check_index_correspondence(node_path,pos_np,index_path,pos_xp,input,i))
                    {
                        dm_rpc->subtree = FALSE;
                        return FAULT_CPE_NO_FAULT_IDX;
                    }
                    dm_rpc->subtree = TRUE;
                    return FAULT_CPE_NO_FAULT_IDX;
                }
                else
                {
                    add_parameter = TRUE;
                    break;
                }
            }
            else
            {
                dm_rpc->subtree = FALSE;
                return FAULT_CPE_NO_FAULT_IDX;
            }
        }
    }

    if (add_parameter==FALSE)
    {
        if (input->sub_path_size == 0)
        {
            if (node->type==DM_PARAMETER)
            {
                add_parameter=TRUE;
            }
            else
            {
                dm_rpc->subtree = TRUE;
                return FAULT_CPE_NO_FAULT_IDX;
            }
        }
        else if (input->sub_path[input->sub_path_size-1].node!=NULL)
        {
            i = pos_np;
            while (i>0)
            {
                i--;
                if (input->sub_path[input->sub_path_size-1].node==node_path[i].node)
                {
                    if (node->type==DM_PARAMETER)
                    {
                        add_parameter = TRUE;
                        break;
                    }
                    else
                    {
                        dm_rpc->subtree = TRUE;
                        return FAULT_CPE_NO_FAULT_IDX;
                    }
                }
            }
        }
    }
    if (add_parameter==FALSE)
    {
        dm_rpc->subtree = FALSE;
        return FAULT_CPE_NO_FAULT_IDX;
    }

    dm_rpc->subtree = FALSE;
    name[0] = 0;
    if (error = get_node_path_name(name, node, dm_rpc, node_path, pos_np, index_path, pos_xp))
    {
        return FAULT_CPE_INVALID_PARAMETER_NAME_IDX;
    }

    if (input->get_attribute)
    {
        int attribute;
        attribute   = dm_getParameterAttributes(name);
        switch (attribute)
        {
            case _cwmp1__SetParameterAttributesStruct_Notification__0:
                return FAULT_CPE_NO_FAULT_IDX;
                break;
            case _cwmp1__SetParameterAttributesStruct_Notification__2:
                input->is_actif = TRUE;
                break;
            default:
                break;
        }
    }

    for (ilist = input->list->next; ilist != input->elist->next; ilist = ilist->next)
    {
        struct handler_ParameterValueStruct         *handler_ParameterValueStruct;
        handler_ParameterValueStruct    = list_entry(ilist,struct handler_ParameterValueStruct,list);
        if (strcmp(handler_ParameterValueStruct->ParameterValueStruct->Name,name)==0)
        {
            return FAULT_CPE_NO_FAULT_IDX;
        }
    }

    if (input->get_value)
    {
        if (error = get_node_paramater_value(node,index_path,pos_xp,&value))
        {
            char t[1];
            t[0] = 0;
            value = strdup(t);
        }
    }

    dm_list_add_ParameterValues(name, value, node, input->list);
    *(input->n) +=1;

    return FAULT_CPE_NO_FAULT_IDX;
}

int cwmp_dm_get_sub_indice_path (int argc, char **argv, char **prefix_path, struct sub_path *sub_path, int *sub_path_size)
{
    int         i=1,len,size=0;
    char        *s=NULL,*t=NULL,*buf_path,*prefix_end;

    buf_path        = strdup(argv[0]);
    *prefix_path    = buf_path;
    prefix_end      = strstr(buf_path,"{i}.");
    if (prefix_end!=NULL && (prefix_end + 4)!=0)
    {
        sub_path[size].dm_indice.indice = (i>=argc)?NULL:strdup(argv[i]);
        sub_path[size].dm_indice.type   = DM_INDICE_CORRESPONDENCE;
        size++;
        i++;
        s = prefix_end + 4; /* 4 = length of "{i}." */
        t = s;
        len = strlen(t);

        *prefix_end = 0;
        s           = strtok(s,".");

        while (s!=NULL && (s-t)<len)
        {
            if (is_numeric(s))
            {
                if (sub_path[size-1].dm_indice.type!=0)
                {
                    return FAULT_CPE_INVALID_PARAMETER_NAME_IDX;
                }
                sub_path[size-1].dm_indice.indice   = strdup(s);
                sub_path[size-1].dm_indice.type     = DM_INDICE_INDEX;
            }
            else if (strcmp(s,"{i}")!=0)
            {
                sub_path[size].name = s;
                size++;
            }
            else
            {
                if (sub_path[size-1].dm_indice.type!=0)
                {
                    return FAULT_CPE_INVALID_PARAMETER_NAME_IDX;
                }
                sub_path[size-1].dm_indice.indice   = (i>=argc)?NULL:strdup(argv[i]);
                sub_path[size-1].dm_indice.type     = DM_INDICE_CORRESPONDENCE;
                i++;
            }
            s = strtok(s+strlen(s)+1,".");
        }
    }
    *sub_path_size  = size;
    return CWMP_OK;
}

int cwmp_dm_getParameterPaths_by_correspondence(struct cwmp *cwmp, char *path, struct sub_path *sub_path, int sub_path_size, struct list_head *list, int *n, bool get_value, bool get_attribute, bool *is_actif)
{
    struct dm_rpc                                           dm_rpc;
    struct dm_input_getParameterPaths_by_correspondence     input;
    int                                                     error;

    memset(&dm_rpc,0,sizeof(struct dm_rpc));
    memset(&input,0,sizeof(struct dm_input_getParameterPaths_by_correspondence));
    *n                  = 0;
    input.list          = list;
    input.elist         = list->prev;
    input.n             = n;
    input.get_value     = get_value;
    input.get_attribute = get_attribute;
    input.sub_path      = sub_path;
    input.sub_path_size = sub_path_size;

    dm_rpc.input        = &input;
    dm_rpc.method       = dm_node_getParameterPaths_by_correspondence;

    error = dm_browse_tree (path, &dm_rpc);

    *is_actif           = input.is_actif;

    return error;
}

/**
 -------------------------------------------------------
|           SetParameterValue Data Model API            |
 -------------------------------------------------------
**/

static int bind_service_handler_queue_by_uci_conf (struct dm_set_handler *dm_set_handler, char *conf)
{
    struct service_handler  *service_handler;
    struct dm_set_handler   *dm_sh;
    struct list_head        *ilist,*jlist;
    struct list_head        list_uci_affect;
    struct config_uci_list  *affect;
    char                    uci_path[256];

    if (strcmp(conf,UCI_TRACK_CONF_CWMP)!=0)
    {
        __list_for_each(ilist,&list_dm_set_handler)
        {
            dm_sh = list_entry(ilist, struct dm_set_handler, list);
            __list_for_each(jlist,&(dm_sh->service_list))
            {
                service_handler = list_entry(jlist, struct service_handler, list);
                if (strcmp(conf,service_handler->service)==0)
                {
                    return DM_OK;
                }
            }

        }
        service_handler             = calloc(1,sizeof(struct service_handler));
        service_handler->service    = strdup(conf);
        list_add_tail(&(service_handler->list),&(dm_set_handler->service_list));
    }
    else
    {
        if (dm_set_handler->cwmp_reload==TRUE)
        {
            return DM_OK;
        }
        dm_set_handler->cwmp_reload=TRUE;
    }
    INIT_LIST_HEAD (&list_uci_affect);
    sprintf(uci_path,"%s.@%s[%d].%s",UCI_TRACK_CONF,conf,0,UCI_TRACK_AFFECTS);
    uci_get_list_value (uci_path, &list_uci_affect);
    while(list_uci_affect.next!=&list_uci_affect)
    {
        affect = list_entry (list_uci_affect.next, struct config_uci_list, list);
        bind_service_handler_queue_by_uci_conf (dm_set_handler,affect->value);
        list_del(list_uci_affect.next);
        if(affect->value != NULL)
        {
            free(affect->value);
        }
        free(affect);
    }
    return DM_OK;
}

static int bind_service_handler_queue (struct dm_set_handler *dm_set_handler, char *uci_cmd)
{
    struct service_handler  *service_handler;
    struct dm_set_handler   *dm_sh;
    struct list_head        *ilist,*jlist;
    char                    *s,conf[128];
    int                     len;
    s = strstr(uci_cmd,".");
    if (s==NULL)
    {
        return  DM_OK;
    }
    len = s - uci_cmd;
    memcpy(conf,uci_cmd,len);
    conf[len] = 0;
    bind_service_handler_queue_by_uci_conf (dm_set_handler,conf);
    return DM_OK;
}

static int bind_cmd_handler_queue(struct dm_set_handler *dm_set_handler, char *cmd, enum param_type type, __u8 end_session)
{
    struct cmd_handler      *cmd_handler;
    struct dm_set_handler   *dm_sh;
    struct list_head        *ilist,*jlist;

    __list_for_each(ilist,&(dm_set_handler->cmd_list))
    {
        cmd_handler = list_entry(ilist, struct cmd_handler, list);
        if (strcmp(cmd,cmd_handler->cmd)==0)
        {
            return DM_OK;
        }
    }
    if (end_session==TRUE)
    {
        __list_for_each(ilist,&list_dm_set_handler)
        {
            dm_sh = list_entry(ilist, struct dm_set_handler, list);
            __list_for_each(jlist,&(dm_sh->cmd_list))
            {
                cmd_handler = list_entry(jlist, struct cmd_handler, list);
                if (strcmp(cmd,cmd_handler->cmd)==0)
                {
                    return DM_OK;
                }
            }
        }
    }

    cmd_handler = calloc(1,sizeof(struct cmd_handler));
    if (cmd_handler==NULL)
    {
        return DM_ERR;
    }
    cmd_handler->cmd            = strdup(cmd);
    cmd_handler->type           = type;
    cmd_handler->end_session    = end_session;
    list_add(&(cmd_handler->list),&(dm_set_handler->cmd_list));
    return DM_OK;
}

static int bind_cancel_handler_queue(struct dm_set_handler *dm_set_handler, char *cmd)
{
    struct cancel_handler   *cancel_handler;
    struct list_head        *ilist;

    __list_for_each(ilist,&(dm_set_handler->cancel_list))
    {
        cancel_handler = list_entry(ilist, struct cancel_handler, list);
        if (strcmp(cmd,cancel_handler->cmd)==0)
        {
            return DM_OK;
        }
    }

    cancel_handler = calloc(1,sizeof(struct cancel_handler));
    if (cancel_handler==NULL)
    {
        return DM_ERR;
    }
    cancel_handler->cmd = strdup(cmd);
    list_add(&(cancel_handler->list),&(dm_set_handler->cancel_list));
    return DM_OK;
}

int dm_run_queue_cancel_handler(struct dm_set_handler *dm_set_handler)
{
    struct list_head        *ilist;
    struct cancel_handler   *cancel_handler;
    int                     error=DM_OK;
    ilist = &(dm_set_handler->cancel_list);
    while (ilist->next!=&(dm_set_handler->cancel_list))
    {
        cancel_handler = list_entry(ilist->next,struct cancel_handler, list);
        CWMP_LOG(INFO,"RUN cancel handler function %s",cancel_handler->cmd); /* TODO to be removed*/
        if (dm_call_system_cmd(cancel_handler->cmd))
        {
            error = DM_ERR;
        }
        if (cancel_handler->cmd!=NULL)
        {
            free(cancel_handler->cmd);
        }
        list_del(ilist->next);
        free(cancel_handler);
    }
    return DM_OK;
}

int dm_run_queue_cmd_handler(struct dm_set_handler *dm_set_handler,__u8 end_session)
{
    struct list_head        *ilist;
    struct cmd_handler      *cmd_handler;
    int                     error=DM_OK;
    bool                    uci=FALSE;
    ilist = &(dm_set_handler->cmd_list);
    while (ilist->next!=&(dm_set_handler->cmd_list))
    {
        cmd_handler = list_entry(ilist->next,struct cmd_handler, list);
        if (cmd_handler->end_session != end_session)
        {
            ilist = ilist->next;
            continue;
        }
        if (cmd_handler->type == DM_UCI)
        {
            CWMP_LOG(INFO,"RUN uci set cmd handler %s",cmd_handler->cmd); /* TODO to be removed*/
            if (uci_set_value (cmd_handler->cmd))
            {
                error = DM_ERR;
            }
            uci = TRUE;
        }
        else if (cmd_handler->type == DM_SYSTEM)
        {
            CWMP_LOG(INFO,"RUN system cmd handler %s",cmd_handler->cmd); /* TODO to be removed*/
            if (dm_call_system_cmd(cmd_handler->cmd))
            {
                error = DM_ERR;
            }
        }
        if (cmd_handler->cmd!=NULL)
        {
            free(cmd_handler->cmd);
        }
        list_del(ilist->next);
        free(cmd_handler);
    }
    if (uci == TRUE)
    {
        uci_commit_value();
    }
    return DM_OK;
}

int dm_run_queue_service_handler(struct dm_set_handler *dm_set_handler)
{
    struct list_head        *ilist;
    struct service_handler  *service_handler;
    char                    buf[256],*init;

    ilist = &(dm_set_handler->service_list);
    while (ilist->next!=&(dm_set_handler->service_list))
    {
        service_handler = list_entry(ilist->next,struct service_handler, list);

        sprintf(buf,"%s.@%s[%d].%s",UCI_TRACK_CONF,service_handler->service,0,UCI_TRACK_INIT);
        if(uci_get_value(buf,&init)==CWMP_OK && init!=NULL)
        {
            CWMP_LOG(INFO,"Restart service %s",init); /* TODO to be removed*/
            sprintf(buf,"/etc/init.d/%s restart",init);
            dm_call_system_cmd(buf);
        }

        if (service_handler->service!=NULL)
        {
            free(service_handler->service);
        }
        list_del(ilist->next);
        free(service_handler);
    }

    return DM_OK;
}

int dm_free_dm_set_handler_queues (struct dm_set_handler *dm_set_handler)
{
    struct list_head        *ilist;
    struct cancel_handler   *cancel_handler;
    struct cmd_handler      *cmd_handler;
    struct service_handler  *service_handler;
    int                     error=DM_OK;
    ilist = &(dm_set_handler->cancel_list);
    while (ilist->next!=ilist)
    {
        cancel_handler = list_entry(ilist->next,struct cancel_handler, list);
        if (cancel_handler->cmd!=NULL)
        {
            free(cancel_handler->cmd);
        }
        list_del(ilist->next);
        free(cancel_handler);
    }
    ilist = &(dm_set_handler->cmd_list);
    while (ilist->next!=ilist)
    {
        cmd_handler = list_entry(ilist->next,struct cmd_handler, list);
        if (cmd_handler->cmd!=NULL)
        {
            free(cmd_handler->cmd);
        }
        list_del(ilist->next);
        free(cmd_handler);
    }
    ilist = &(dm_set_handler->service_list);
    while (ilist->next!=ilist)
    {
        service_handler = list_entry(ilist->next,struct service_handler, list);
        if (service_handler->service!=NULL)
        {
            free(service_handler->service);
        }
        list_del(ilist->next);
        free(service_handler);
    }
    return DM_OK;
}

int dm_run_queue_cmd_handler_at_end_session (struct cwmp *cwmp, struct dm_set_handler *dm_set_handler)
{
    int error;
    CWMP_LOG(INFO,"RUN End Sessions Functions"); /* TODO to be removed*/
    error = dm_run_queue_cmd_handler(dm_set_handler,TRUE);
    if (dm_set_handler->reboot_required==FALSE)
    {
        dm_run_queue_service_handler(dm_set_handler);
    }
    dm_free_dm_set_handler_queues(dm_set_handler);
    if (dm_set_handler!=NULL)
    {
        list_del(&(dm_set_handler->list));
        free(dm_set_handler);
    }
    return error;
}
int dm_cwmp_config_reload (struct cwmp *cwmp, void *v )
{
    CWMP_LOG(INFO,"Reload CWMP Config"); /* TODO to be removed*/
    if (cwmp_apply_acs_changes(cwmp))
    {
        return DM_GEN_ERR;
    }
    return DM_ERR;
}
static int enqueue_apply_cancel_handlers(struct dm_set_handler *dm_set_handler,
                                        struct dm_node_path *node_path,
                                        int pos_np,
                                        struct dm_index_path *index_path,
                                        int pos_xp)
{
    struct data_handler         **data_handler;
    unsigned short              error,k;
    __u8                        i,size_dh;
    struct dm_node              *node;
    char                        corr[256];
    for (k=0;k<(pos_np);k++)
    {
        node            = node_path[k].node;
        data_handler    = node->data_handler;
        size_dh         = node->size_data_handler;
        for (i=0;i<size_dh;i++)
        {
            if (data_handler[i]->type == DM_SYSTEM)
            {
                struct dm_system    *system;
                system  = (struct dm_system *)data_handler[i]->handler;
                if (system->type == DM_APPLY)
                {
                    get_string_correspondence(corr,system->cmd,index_path,pos_xp-1);
                    if (system->reboot_required)
                    {
                        dm_set_handler->reboot_required = TRUE;
                    }
                    if (bind_cmd_handler_queue(dm_set_handler,corr,DM_SYSTEM,system->end_session))
                    {
                        return DM_ERR;
                    }
                }
                else if (system->type == DM_CANCEL)
                {
                    get_string_correspondence(corr,system->cmd,index_path,pos_xp-1);
                    if (system->reboot_required)
                    {
                        dm_set_handler->reboot_required = TRUE;
                    }
                    if (bind_cancel_handler_queue(dm_set_handler,corr))
                    {
                        return DM_ERR;
                    }
                }
            }
        }
    }
    return DM_OK;
}

static int set_node_paramater_value (struct dm_node *node,
                                    struct dm_index_path *index_path,
                                    int pos_xp,
                                    char *value,
                                    struct dm_set_handler *dm_set_handler)
{
    struct data_handler         **data_handler;
    unsigned short              error,j;
    __u8                        size_dh,i;
    char                        corr[256],*serr=NULL;

    data_handler    = node->data_handler;
    size_dh         = node->size_data_handler;
    for (i=0;i<size_dh;i++)
    {
        if (data_handler[i]->type == DM_UCI)
        {
            struct dm_uci       *uci;
            uci = (struct dm_uci *)data_handler[i]->handler;
            get_string_correspondence (corr,uci->cmd,index_path,pos_xp-1);

            bind_service_handler_queue(dm_set_handler,corr);

            sprintf(corr,"%s=%s",corr,value);

            if (uci->reboot_required)
            {
                dm_set_handler->reboot_required = TRUE;
            }

            if (uci->end_session)
            {
                if (bind_cmd_handler_queue(dm_set_handler,corr,DM_UCI,TRUE))
                {
                    return FAULT_CPE_INTERNAL_ERROR_IDX;
                }
                return FAULT_CPE_NO_FAULT_IDX;
            }

            if (error = uci_set_value (corr))
            {
                return FAULT_CPE_INTERNAL_ERROR_IDX;
            }
            dm_set_handler->uci = TRUE;
            return FAULT_CPE_NO_FAULT_IDX;
        }
        else if (data_handler[i]->type == DM_SYSTEM)
        {
            struct dm_system    *system;
            system  = (struct dm_system *)data_handler[i]->handler;
            if (system->type != DM_SET)
            {
                continue;
            }
            get_string_correspondence(corr,system->cmd,index_path,pos_xp-1);

            sprintf(corr,"%s %s",corr,value);

            if (system->reboot_required)
            {
                dm_set_handler->reboot_required = TRUE;
            }

            if (system->end_session)
            {
                if (bind_cmd_handler_queue(dm_set_handler,corr,DM_SYSTEM,TRUE))
                {
                    return FAULT_CPE_INTERNAL_ERROR_IDX;
                }
                return FAULT_CPE_NO_FAULT_IDX;
            }

            if (error = dm_get_system_data(corr,&serr))
            {
                if (serr!=NULL)
                {
                    free(serr);
                }
                return FAULT_CPE_INTERNAL_ERROR_IDX;
            }
            if (serr!=NULL)
            {
                char    buf[16];
                for (j=1;j<FAULT_CPE_ARRAY_SIZE;j++)
                {
                    sprintf(buf,"e-%s",FAULT_CPE_ARRAY[j].CODE);
                    if (strcmp(serr,buf)==0)
                    {
                        free(serr);
                        return j;
                    }
                }
                free(serr);
            }
            return FAULT_CPE_NO_FAULT_IDX;
        }
        else if (data_handler[i]->type == DM_REGEXP)
        {
            char            *regexp;
            regexp = (char *)data_handler[i]->handler;
            if (regexp!=NULL && (error = dm_match_patterns(value,regexp)))
            {
                return FAULT_CPE_INVALID_PARAMETER_VALUE_IDX;
            }
        }
    }
    return FAULT_CPE_NO_FAULT_IDX;
}


int dm_node_setParameterValues (struct dm_node *node,
                                struct dm_rpc *dm_rpc,
                                struct dm_node_path *node_path,
                                int pos_np,
                                struct dm_index_path *index_path,
                                int pos_xp)
{

    struct dm_input_setParameterValues          *input;
    char                                        name[512];
    int                                         len,error;

    if (node == NULL || node->type == DM_OBJECT || node->type == DM_INSTANCE)
    {
        return FAULT_CPE_INVALID_PARAMETER_NAME_IDX;
    }

    if (node->permission != DM_READ_WRITE)
    {
        return FAULT_CPE_NON_WRITABLE_PARAMETER_IDX;
    }

    input = (struct dm_input_setParameterValues *)dm_rpc->input;
    dm_rpc->subtree = FALSE;

    error = set_node_paramater_value(node,index_path,pos_xp,input->value,input->dm_set_handler);

    enqueue_apply_cancel_handlers(input->dm_set_handler,node_path,pos_np,index_path,pos_xp);

    return error;
}

int cwmp_dm_setParameterValues(struct cwmp *cwmp, struct dm_set_handler *dm_set_handler, char *path, char *value)
{
    struct dm_rpc                               dm_rpc;
    struct dm_input_setParameterValues          input;
    int                                         error;

    memset(&dm_rpc,0,sizeof(struct dm_rpc));

    input.value             = value;
    input.dm_set_handler    = dm_set_handler;
    dm_rpc.input            = &input;
    dm_rpc.method           = dm_node_setParameterValues;

    error = dm_browse_tree (path, &dm_rpc);

    return error;
}
/**
 -------------------------------------------------------------
|           GetParameterAttributes Data Model API             |
 -------------------------------------------------------------
**/
int dm_list_add_ParameterAttributes(char *name,
                                    struct dm_node *node,
                                    struct list_head *list)
{
    struct cwmp1__ParameterAttributeStruct          *ParameterAttributeStruct;
    struct handler_ParameterAttributeStruct         *handler_ParameterAttributeStruct;
    struct list_head                                *ilist;
    int                                             is_active,is_passive,i;
    char                                            **ptr_AccessList;
    extern const struct ACCESSLIST_CONST_STRUCT ACCESSLIST_CONST_ARRAY [COUNT_ACCESSLIST];

    ParameterAttributeStruct            = calloc(1,sizeof(struct cwmp1__ParameterAttributeStruct));
    handler_ParameterAttributeStruct    = calloc(1,sizeof(struct handler_ParameterAttributeStruct));

    if (ParameterAttributeStruct == NULL ||
        handler_ParameterAttributeStruct == NULL)
    {
        return FAULT_CPE_INTERNAL_ERROR_IDX;
    }

    ParameterAttributeStruct->Name = strdup(name);

    if((is_passive = cwmp_check_notification(name, UCI_NOTIFICATION_PASSIVE_PATH)) != CWMP_OK)
    {
        ParameterAttributeStruct->Notification = _cwmp1__ParameterAttributeStruct_Notification__1;
    }
    if((is_active = cwmp_check_notification(name, UCI_NOTIFICATION_ACTIVE_PATH)) != CWMP_OK)
    {
        ParameterAttributeStruct->Notification = _cwmp1__ParameterAttributeStruct_Notification__2;
    }
    if (!is_active && !is_passive)
    {
        ParameterAttributeStruct->Notification = _cwmp1__ParameterAttributeStruct_Notification__0;
    }
    ParameterAttributeStruct->AccessList = calloc(1,sizeof(struct cwmp1AccessList *));
    ptr_AccessList = calloc(sizearray(ACCESSLIST_CONST_ARRAY),sizeof(struct cwmp1AccessList *));
    if(ParameterAttributeStruct->AccessList==NULL ||
       ptr_AccessList == NULL)
    {
        return FAULT_CPE_INTERNAL_ERROR_IDX;
    }
    ParameterAttributeStruct->AccessList->__ptrstring = ptr_AccessList;
    for(i=0;i<sizearray(ACCESSLIST_CONST_ARRAY);i++,ptr_AccessList++)
    {
        if(cwmp_check_notification(name,ACCESSLIST_CONST_ARRAY[i].UCI_ACCESSLIST_PATH) != CWMP_OK)
        {
            *ptr_AccessList = strdup(ACCESSLIST_CONST_ARRAY[i].NAME);
            ParameterAttributeStruct->AccessList->__size++;
        }
    }
    handler_ParameterAttributeStruct->ParameterAttributeStruct  = ParameterAttributeStruct;
    list_add_tail(&handler_ParameterAttributeStruct->list,list);
    return FAULT_CPE_NO_FAULT_IDX;
}

int dm_node_getParameterAttributes (struct dm_node *node,
                                    struct dm_rpc *dm_rpc,
                                    struct dm_node_path *node_path,
                                    int pos_np,
                                    struct dm_index_path *index_path,
                                    int pos_xp)
{
    struct dm_input_getParameterAttributes      *input;
    char                                        name[512];
    int                                         len,error;

    input = (struct dm_input_getParameterAttributes *)dm_rpc->input;

    if (node == NULL || node->type == DM_OBJECT || node->type == DM_INSTANCE)
    {
        dm_rpc->subtree = TRUE;
        return FAULT_CPE_NO_FAULT_IDX;
    }
    dm_rpc->subtree = FALSE;
    name[0] = 0;
    if (error = get_node_path_name(name, node, dm_rpc, node_path, pos_np, index_path, pos_xp))
    {
        return FAULT_CPE_INVALID_PARAMETER_NAME_IDX;
    }

    dm_list_add_ParameterAttributes(name, node, input->list);
    *(input->n) +=1;

    return FAULT_CPE_NO_FAULT_IDX;
}

int cwmp_dm_getParameterAttributes(struct cwmp *cwmp, char *path, struct list_head *list, int *n)
{
    struct dm_rpc                               dm_rpc;
    struct dm_input_getParameterAttributes      input;
    int                                         error;

    memset(&dm_rpc,0,sizeof(struct dm_rpc));

    *n              = 0;
    input.list      = list;
    input.n         = n;
    dm_rpc.input    = &input;
    dm_rpc.method   = dm_node_getParameterAttributes;

    error = dm_browse_tree (path, &dm_rpc);

    return error;
}

int dm_getParameterAttributes (char *path)
{
    int                         n = 0;
    int                         len_passive,len_active,len_denied,len1;
    struct config_uci_list      *notification;
    LIST_HEAD(notification_list);

    len1 = 0;
    len_denied = 0;
    len_passive = 0;
    len_active = 0;

    n = strlen(path);

    uci_get_list_value(UCI_NOTIFICATION_DENIED_PATH, &notification_list);
    while(!list_empty(&notification_list))
    {
        notification = list_entry (notification_list.next, struct config_uci_list, list);
        if((notification != NULL))
        {
            len1 = strlen(notification->value);
            if(strncmp(path,notification->value,len1) == 0)
            {
                if(len_denied<len1)
                {
                    len_denied = len1;
                }
            }
            list_del(notification_list.next);
            free(notification);
            notification = NULL;
        }
    }
    if(n == len_denied)
    {
        return _cwmp1__SetParameterAttributesStruct_Notification__0;
    }

    uci_get_list_value(UCI_NOTIFICATION_PASSIVE_PATH, &notification_list);
    while(!list_empty(&notification_list))
    {
        notification = list_entry (notification_list.next, struct config_uci_list, list);
        if((notification != NULL))
        {
            len1 = strlen(notification->value);
            if(strncmp(path,notification->value,len1) == 0)
            {
                if(len_passive<len1)
                {
                    len_passive = len1;
                }
            }
            list_del(notification_list.next);
            free(notification);
            notification = NULL;
        }
    }
    if(n == len_passive)
    {
        return _cwmp1__SetParameterAttributesStruct_Notification__1;
    }

    uci_get_list_value(UCI_NOTIFICATION_ACTIVE_PATH, &notification_list);
    while(!list_empty(&notification_list))
    {
        notification = list_entry (notification_list.next, struct config_uci_list, list);
        if((notification != NULL))
        {
            len1 = strlen(notification->value);
            if(strncmp(path,notification->value,len1) == 0)
            {
                if(len_active<len1)
                {
                    len_active = len1;
                }
            }
            list_del(notification_list.next);
            free(notification);
            notification = NULL;
        }
    }
    if(n == len_active)
    {
        return _cwmp1__SetParameterAttributesStruct_Notification__2;
    }

    if(((len_passive == 0)&&(len_active == 0))||((len_denied>len_active)&&(len_denied>len_passive)))
    {
        return _cwmp1__SetParameterAttributesStruct_Notification__0;
    }
    if((len_passive>len_active)&&(len_passive>len_denied))
    {
        return _cwmp1__SetParameterAttributesStruct_Notification__1;
    }
    if((len_active>len_passive)&&(len_active>len_denied))
    {
        return _cwmp1__SetParameterAttributesStruct_Notification__2;
    }
    return _cwmp1__SetParameterAttributesStruct_Notification__0;
}
/**
 -------------------------------------------------------------
|           SetParameterAttributes Data Model API             |
 -------------------------------------------------------------
**/
int dm_node_setParameterAttributes (struct dm_node *node,
                                    struct dm_rpc *dm_rpc,
                                    struct dm_node_path *node_path,
                                    int pos_np,
                                    struct dm_index_path *index_path,
                                    int pos_xp)
{

    struct dm_input_setParameterAttributes      *input;
    int                                         len,error,i;
    struct cwmp1__SetParameterAttributesStruct  *ParameterAttributesStruct;
    char                                        **AccessList;
    int                                         size_AccessList = 0;

    input = (struct dm_input_setParameterAttributes *)dm_rpc->input;

    if (node == NULL)
    {
        return FAULT_CPE_INVALID_PARAMETER_NAME_IDX;
    }

    if (node != NULL)
    {
        dm_rpc->subtree = FALSE;
        ParameterAttributesStruct = input->ParameterAttributesStruct;
        switch(ParameterAttributesStruct->Notification)
        {
            case _cwmp1__SetParameterAttributesStruct_Notification__0:
                if(error = cwmp_off_notification(node,*(ParameterAttributesStruct->Name), ParameterAttributesStruct->NotificationChange))
                {
                    uci_revert_value ();
                    return error;
                }
                break;
            case _cwmp1__SetParameterAttributesStruct_Notification__1:
                if(error = cwmp_apply_passive_notification(node,*(ParameterAttributesStruct->Name), ParameterAttributesStruct->NotificationChange))
                {
                    uci_revert_value ();
                    return error;
                }
                AccessList              = ParameterAttributesStruct->AccessList->__ptrstring;
                size_AccessList         = ParameterAttributesStruct->AccessList->__size;

                for(i=0;i<size_AccessList;i++)
                {
                    if(error = cwmp_add_in_accesslist(*(ParameterAttributesStruct->Name), ParameterAttributesStruct->AccessListChange, *AccessList))
                    {
                        uci_revert_value ();
                        return error;
                    }
                    AccessList++;
                }
                break;
            case _cwmp1__SetParameterAttributesStruct_Notification__2:
                if(error = cwmp_apply_active_notification(node,*(ParameterAttributesStruct->Name), ParameterAttributesStruct->NotificationChange))
                {
                    uci_revert_value ();
                    return error;
                }
                AccessList              = ParameterAttributesStruct->AccessList->__ptrstring;
                size_AccessList         = ParameterAttributesStruct->AccessList->__size;

                for(i=0;i<size_AccessList;i++)
                {
                    if(error = cwmp_add_in_accesslist(*(ParameterAttributesStruct->Name), ParameterAttributesStruct->AccessListChange, *AccessList))
                    {
                        uci_revert_value ();
                        return error;
                    }
                    AccessList++;
                }
                break;
        }
    }

    return FAULT_CPE_NO_FAULT_IDX;
}

int cwmp_dm_setParameterAttributes(struct cwmp *cwmp, struct cwmp1__SetParameterAttributesStruct *ParameterAttributesStruct)
{
    struct dm_rpc                               dm_rpc;
    struct dm_input_setParameterAttributes      input;
    int                                         error;
    char                                        *path;

    path = *(ParameterAttributesStruct->Name);

    memset(&dm_rpc,0,sizeof(struct dm_rpc));

    input.ParameterAttributesStruct     = ParameterAttributesStruct;
    dm_rpc.input                        = &input;
    dm_rpc.method                       = dm_node_setParameterAttributes;

    error = dm_browse_tree (path, &dm_rpc);

    return error;
}
/**
 ------------------------------------------------
|           AddObject Data Model API             |
 ------------------------------------------------
**/
static int add_node_add_object (struct dm_node *node,
                                    struct dm_index_path *index_path,
                                    int pos_xp,
                                    int *InstanceNumber,
                                    char *path,
                                    struct dm_set_handler *dm_set_handler)
{
    struct data_handler         **data_handler;
    unsigned short              error,j;
    __u8                        size_dh,i;
    char                        corr[256],*dm_system_output=NULL;
    char                        instanceName[256];
    extern const struct ACCESSLIST_CONST_STRUCT ACCESSLIST_CONST_ARRAY [COUNT_ACCESSLIST];

    data_handler    = node->data_handler;
    size_dh         = node->size_data_handler;
    for (i=0;i<size_dh;i++)
    {
        if (data_handler[i]->type == DM_SYSTEM)
        {
            struct dm_system    *system;
            system  = (struct dm_system *)data_handler[i]->handler;
            if (system->type != DM_ADD)
            {
                continue;
            }
            get_string_correspondence(corr,system->cmd,index_path,pos_xp-1);

            if (system->reboot_required)
            {
                dm_set_handler->reboot_required = TRUE;
            }

            if (error = dm_get_system_data(corr,&dm_system_output))
            {
                if (dm_system_output!=NULL)
                {
                    free(dm_system_output);
                }
                return FAULT_CPE_INTERNAL_ERROR_IDX;
            }

            if (dm_system_output!=NULL)
            {
                char    buf[16];
                for (j=1;j<FAULT_CPE_ARRAY_SIZE;j++)
                {
                    sprintf(buf,"e-%s",FAULT_CPE_ARRAY[j].CODE);
                    if (strcmp(dm_system_output,buf)==0)
                    {
                        free(dm_system_output);
                        return j;
                    }
                }
                *InstanceNumber = atoi(dm_system_output);
                sprintf(instanceName,"%s%d.",path,*InstanceNumber);
                cwmp_off_notification(node,instanceName,xsd__boolean__true_);
                for(j=0;j<COUNT_ACCESSLIST;j++)
                {
                    cwmp_add_in_accesslist(instanceName,xsd__boolean__true_,ACCESSLIST_CONST_ARRAY[j].NAME);
                }
                free(dm_system_output);
            }
            return FAULT_CPE_NO_FAULT_IDX;
        }
    }
    return FAULT_CPE_REQUEST_DENIED_IDX;
}

int dm_node_addObject (struct dm_node *node,
                                struct dm_rpc *dm_rpc,
                                struct dm_node_path *node_path,
                                int pos_np,
                                struct dm_index_path *index_path,
                                int pos_xp)
{

    struct dm_input_addObject                   *input;
    char                                        name[512];
    int                                         len,error;
    struct dm_node_path                         *enp;
    struct dm_index_path                        *exp;

    enp = &node_path[pos_np-1];
    exp = enp->index_path;

    if (node == NULL || node->type == DM_OBJECT || node->type == DM_PARAMETER || exp->index != NULL)
    {
        return FAULT_CPE_INVALID_PARAMETER_NAME_IDX;
    }

    if (node->permission != DM_CREATE)
    {
        return FAULT_CPE_REQUEST_DENIED_IDX;
    }

    input = (struct dm_input_addObject *)dm_rpc->input;
    dm_rpc->subtree = FALSE;

    error = add_node_add_object(node,index_path,pos_xp,input->n,input->path,input->dm_set_handler);

    enqueue_apply_cancel_handlers(input->dm_set_handler,node_path,pos_np,index_path,pos_xp);

    return error;
}

int cwmp_dm_addObject(struct cwmp *cwmp, struct dm_set_handler *dm_set_handler, char *path, int *n)
{
    struct dm_rpc                               dm_rpc;
    struct dm_input_addObject                   input;
    int                                         error;

    memset(&dm_rpc,0,sizeof(struct dm_rpc));

    *n                      = 0;
    input.path              = path;
    input.n                 = n;
    input.dm_set_handler    = dm_set_handler;
    dm_rpc.input            = &input;
    dm_rpc.method           = dm_node_addObject;

    error = dm_browse_tree (path, &dm_rpc);

    return error;
}
/**
 ------------------------------------------------
|           DeleteObject Data Model API             |
 ------------------------------------------------
**/
static int del_node_delete_object (struct dm_node *node,
                                    struct dm_index_path *index_path,
                                    int pos_xp,
                                    char *path,
                                    struct dm_set_handler *dm_set_handler)
{
    struct data_handler         **data_handler;
    unsigned short              error,j;
    __u8                        size_dh,i;
    char                        corr[256],*dm_system_output=NULL;

    data_handler    = node->data_handler;
    size_dh         = node->size_data_handler;
    for (i=0;i<size_dh;i++)
    {
        if (data_handler[i]->type == DM_SYSTEM)
        {
            struct dm_system    *system;
            system  = (struct dm_system *)data_handler[i]->handler;
            if (system->type != DM_DEL)
            {
                continue;
            }
            get_string_correspondence(corr,system->cmd,index_path,pos_xp-1);

            sprintf(corr,"%s %s",corr,path);

            if (system->reboot_required)
            {
                dm_set_handler->reboot_required = TRUE;
            }

            if (error = dm_get_system_data(corr,&dm_system_output))
            {
                if (dm_system_output!=NULL)
                {
                    free(dm_system_output);
                }
                return FAULT_CPE_INTERNAL_ERROR_IDX;
            }

            if (dm_system_output!=NULL)
            {
                char    buf[16];
                for (j=1;j<FAULT_CPE_ARRAY_SIZE;j++)
                {
                    sprintf(buf,"e-%s",FAULT_CPE_ARRAY[j].CODE);
                    if (strcmp(dm_system_output,buf)==0)
                    {
                        free(dm_system_output);
                        return j;
                    }
                }
                free(dm_system_output);
            }
            return FAULT_CPE_NO_FAULT_IDX;
        }
    }
    return FAULT_CPE_REQUEST_DENIED_IDX;
}

int dm_node_deleteObject (struct dm_node *node,
                                struct dm_rpc *dm_rpc,
                                struct dm_node_path *node_path,
                                int pos_np,
                                struct dm_index_path *index_path,
                                int pos_xp)
{

    struct dm_input_deleteObject                *input;
    char                                        name[512];
    int                                         len,error;
    struct dm_node_path                         *enp;
    struct dm_index_path                        *exp;

    enp = &node_path[pos_np-1];
    exp = enp->index_path;

    if (node == NULL || node->type == DM_OBJECT || node->type == DM_PARAMETER || exp->index == NULL)
    {
        return FAULT_CPE_INVALID_PARAMETER_NAME_IDX;
    }

    if (node->permission != DM_CREATE)
    {
        return FAULT_CPE_REQUEST_DENIED_IDX;
    }

    input = (struct dm_input_deleteObject *)dm_rpc->input;
    dm_rpc->subtree = FALSE;

    error = del_node_delete_object(node,index_path,pos_xp,input->path,input->dm_set_handler);

    enqueue_apply_cancel_handlers(input->dm_set_handler,node_path,pos_np,index_path,pos_xp);

    return error;
}

int cwmp_dm_deleteObject(struct cwmp *cwmp, struct dm_set_handler *dm_delete_handler, char *path)
{
    struct dm_rpc                               dm_rpc;
    struct dm_input_deleteObject                input;
    int                                         error;

    memset(&dm_rpc,0,sizeof(struct dm_rpc));

    input.path              = path;
    input.dm_set_handler    = dm_delete_handler;
    dm_rpc.input            = &input;
    dm_rpc.method           = dm_node_deleteObject;

    error = dm_browse_tree (path, &dm_rpc);

    return error;
}
