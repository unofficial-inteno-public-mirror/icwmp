/*
    dm.c

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
#include <expat.h>
#include <assert.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include "cwmp.h"
#include "list.h"
#include "dm.h"
#include <regex.h>
#include <errno.h>

LIST_HEAD(head_dm_tree);
LIST_HEAD(parameter_handler_list);
LIST_HEAD(object_handler_list);
LIST_HEAD(forced_inform_parameter_list);
static int                              Depth;
static char                             last_content[512];
static char                             *last_object_name = NULL;
static char                             *last_tag_name = NULL;
static struct dm_node                   *node;
static struct dm_node                   *current_object;
static struct list_data_handler         *tmp_handler = NULL;
static struct forced_inform_parameter   *forced_inform_parameter = NULL;

char *TYPE_VALUES_ARRAY [] =
{
    [TYPE_VALUE_string_IDX]                     = "xsd:string",
    [TYPE_VALUE_int_IDX]                        = "xsd:int",
    [TYPE_VALUE_unsignedInt_IDX]                = "xsd:unsignedInt",
    [TYPE_VALUE_boolean_IDX]                    = "xsd:boolean",
    [TYPE_VALUE_dateTime_IDX]                   = "xsd:dateTime",
    [TYPE_VALUE_base64Binary_IDX]               = "xsd:base64Binary",
    [TYPE_VALUE_integer_IDX]                    = "xsd:integer",
    [TYPE_VALUE_unsignedByte_IDX]               = "xsd:unsignedByte",
    [TYPE_VALUE_unsignedLong_IDX]               = "xsd:unsignedLong",
    [TYPE_VALUE_unsignedShort_IDX]              = "xsd:unsignedShort",
    [TYPE_VALUE_anyURI_IDX]                     = "xsd:anyURI",
    [TYPE_VALUE_byte_IDX]                       = "xsd:byte",
    [TYPE_VALUE_date_IDX]                       = "xsd:date",
    [TYPE_VALUE_time_IDX]                       = "xsd:time",
    [TYPE_VALUE_decimal_IDX]                    = "xsd:decimal",
    [TYPE_VALUE_double_IDX]                     = "xsd:double",
    [TYPE_VALUE_duration_IDX]                   = "xsd:duration",
    [TYPE_VALUE_ENTITIES_IDX]                   = "xsd:ENTITIES",
    [TYPE_VALUE_ENTITY_IDX]                     = "xsd:ENTITY",
    [TYPE_VALUE_float_IDX]                      = "xsd:float",
    [TYPE_VALUE_gDay_IDX]                       = "xsd:gDay",
    [TYPE_VALUE_gMonth_IDX]                     = "xsd:gMonth",
    [TYPE_VALUE_gMonthDay_IDX]                  = "xsd:gMonthDay",
    [TYPE_VALUE_gYear_IDX]                      = "xsd:gYear",
    [TYPE_VALUE_gYearMonth_IDX]                 = "xsd:gYearMonth",
    [TYPE_VALUE_hexBinary_IDX]                  = "xsd:hexBinary",
    [TYPE_VALUE_ID_IDX]                         = "xsd:ID",
    [TYPE_VALUE_IDREF_IDX]                      = "xsd:IDREF",
    [TYPE_VALUE_IDREFS_IDX]                     = "xsd:IDREFS",
    [TYPE_VALUE_language_IDX]                   = "xsd:language",
    [TYPE_VALUE_long_IDX]                       = "xsd:long",
    [TYPE_VALUE_Name_IDX]                       = "xsd:Name",
    [TYPE_VALUE_NCName_IDX]                     = "xsd:NCName",
    [TYPE_VALUE_negativeInteger_IDX]            = "xsd:negativeInteger",
    [TYPE_VALUE_NMTOKEN_IDX]                    = "xsd:NMTOKEN",
    [TYPE_VALUE_NMTOKENS_IDX]                   = "xsd:NMTOKENS",
    [TYPE_VALUE_nonNegativeInteger_IDX]         = "xsd:nonNegativeInteger",
    [TYPE_VALUE_nonPositiveInteger_IDX]         = "xsd:nonPositiveInteger",
    [TYPE_VALUE_normalizedString_IDX]           = "xsd:normalizedString",
    [TYPE_VALUE_NOTATION_IDX]                   = "xsd:NOTATION",
    [TYPE_VALUE_positiveInteger_IDX]            = "xsd:positiveInteger",
    [TYPE_VALUE_QName_IDX]                      = "xsd:QName",
    [TYPE_VALUE_short_IDX]                      = "xsd:short",
    [TYPE_VALUE_token_IDX]                      = "xsd:token"
};

int dm_get_csv_data(char *data, int ns, char ***T)
{
    char **s, *p, *t, *pch;
    s       = calloc (ns,sizeof(char *));
    assert (s);
    *T      = s;
    p       = strdup (data);
    t       = p;
    pch     = strtok (p,",");
    while (pch != NULL)
    {
        *s  = strdup(pch);
        s++;
        pch = strtok (NULL, ",");
    }
    free(t);
    return DM_OK;
}

int dm_get_xml_map (char *data, struct dm_map *dm_map)
{

    char            **T, *p, *pch, *t, *t2;
    unsigned short  i,n=0;

    p       = strdup(data);
    t       = p;
    while (t=strchr(t+1,','))
    {
        n++;
    }
    if (n>0) n++;
    T = calloc(n,sizeof(char *));
    assert(T);
    dm_map->size    = n;
    dm_map->map     = T;
    t               = p;
    pch     = strtok (p,",");
    while (pch != NULL)
    {
        *T  = strdup(pch);
        T++;
        pch = strtok (NULL, ",");
    }
    free(t);
    return DM_OK;
}

int dm_get_xml_data (char *data, struct dm_data *dm_data)
{

    char            ***T, *p, *pch, *t, *s;
    unsigned short  i,ns=0,nT=0;

    if (data==NULL||data[0]==0)
    {
        return DM_OK;
    }

    p       = strdup(data);
    t       = p;

    while (t=strchr(t+1,';'))
    {
        nT++;
    }
    nT++;
    t                   = p;
    dm_data->line_size  = nT;
    T                   = calloc(nT,sizeof(char **));
    assert (T);
    dm_data->data       = T;
    pch                 = strtok (p,";");
    for (i=0;i<nT;i++,T++)
    {
        s   = pch;
        if (ns==0)
        {
            while (s=strchr(s+1,','))
            {
                ns++;
            }
            ns++;
            dm_data->column_size = ns;
        }
        dm_get_csv_data (pch,ns,T);
        pch = strtok (pch+strlen(pch)+1, ";");
    }
    free(t);
    return DM_OK;
}

int dm_get_system_data_array(char *cmd, struct dm_data *dm_data)
{

    char            ***T, **buf[256], *p, *t;
    unsigned short  i,ns=0,nT=0;
    FILE            *fp;
    char            line[256];
    char            error;
    fd_set          set;
    struct          timeval timeout;

    FD_ZERO(&set);
    timeout.tv_sec = SYSTEM_CMD_TIMEOUT;

    fp=popen(cmd,"r");
    if (fp==NULL)
    {
        return DM_ERR;
    }
    FD_SET(fileno(fp), &set);
    if (select(FD_SETSIZE, &set, NULL, NULL, &timeout)!=1)
    {
        fp=popen("\n","r");
        pclose(fp);
        return DM_ERR;
    }
    line[0] = 0;
    while(fgets(line,sizeof(line),fp))
    {
        t   = line;
        t   = strtok(t,"\r\n");
        if(line[0]==0)
        {
            continue;
        }
        if (ns==0)
        {
            while (t=strchr(t+1,','))
            {
                ns++;
            }
            ns++;
            dm_data->column_size = ns;
        }
        dm_get_csv_data (line,ns,&buf[nT]);
        nT++;
        line[0] = 0;
    }
    error = pclose(fp);
    if (nT!=0)
    {
        T = calloc(nT,sizeof(char **));
        assert (T);
        dm_data->data = T;
        dm_data->line_size = nT;
        memcpy(T,buf,nT*sizeof(char **));
    }

    if(error == 0)
    {
        return DM_OK;
    }
    else
    {
        return DM_ERR;
    }
}
int dm_free_data_array(struct dm_data *dm_data)
{
    char            ***T,**s;
    unsigned short  nT,ns,i,j;
    if(dm_data==NULL)
    {
        return DM_OK;
    }
    T   = dm_data->data;
    nT  = dm_data->line_size;
    ns  = dm_data->column_size;
    if (T==NULL)
    {
        return DM_OK;
    }
    for (i=0;i<nT;i++,T++)
    {
        s = *T;
        if (s!=NULL)
        {
            for (j=0;j<ns;j++,s++)
            {
                if(*s!=NULL)
                {
                    free(*s);
                }
            }
            free(*T);
        }
    }
    free(dm_data->data);
    free(dm_data);
    return DM_OK;
}

int dm_get_system_data(char *cmd, char **ret)
{

    FILE            *fp;
    int             size=8191;
    char            buffer[8192];
    int             error;
    fd_set          set;
    struct          timeval timeout;

    FD_ZERO(&set);
    timeout.tv_sec = SYSTEM_CMD_TIMEOUT;

    buffer[0]=0;
    fp=popen(cmd,"r");
    if (fp==NULL)
    {
        return DM_ERR;
    }
    FD_SET(fileno(fp), &set);
    if (select(FD_SETSIZE, &set, NULL, NULL, &timeout)!=1)
    {
        fp=popen("\n","r");
        pclose(fp);
        return DM_ERR;
    }
    size = fread (buffer,1,size,fp);
    if (size>1 && (buffer[size-2]=='\r' || buffer[size-2]=='\n'))
    {
        buffer[size-2]=0;
    }
    else if (size>0 && (buffer[size-1]=='\r' || buffer[size-1]=='\n'))
    {
        buffer[size-1]=0;
    }
    else
    {
        buffer[size]=0;
    }
    *ret = strdup(buffer);
    error = pclose(fp);

    if(error == 0)
    {
        return DM_OK;
    }
    else
    {
        return DM_ERR;
    }
}
int dm_call_system_cmd(char *cmd)
{

    FILE            *fp;
    int             error;

    fp=popen(cmd,"r");
    if (fp==NULL)
    {
        return DM_ERR;
    }
    error = pclose(fp);

    if(error == 0)
    {
        return DM_OK;
    }
    else
    {
        return DM_ERR;
    }
}


long int get_xml_file_size(FILE *pFile)
{
    long int size;

    if(pFile!=NULL)
    {
        fseek (pFile, 0, SEEK_END);

        size = ftell (pFile);
        rewind(pFile);
        return size;
    }
    return 0;
}

int dm_add_node(struct dm_node *node)
{
    struct list_head    *ilist;
    struct list_head    *jlist;
    char                *path = NULL;
    char                object_name[512];
    struct dm_node      *inode = NULL;
    struct dm_node      *jnode = NULL;
    int                 error = DM_OK;
    int                 size;

    if(last_object_name == NULL)
    {
        return DM_GEN_ERR;
    }
    if(node->type == DM_OBJECT)
    {
        size = strlen(last_object_name);
        strcpy(object_name,last_object_name);
        object_name[size] = '\0';
    }
    else
    {
        size = strlen(last_object_name) + strlen(node->name);
        strcpy(object_name,last_object_name);
        strcat(object_name,node->name);
        object_name[size] = '\0';
    }
    path = object_name;
    path = strtok (path,".");
    if(head_dm_tree.next != &head_dm_tree)
    {
        __list_for_each(ilist, &(head_dm_tree))
        {
            inode = (struct dm_node *) list_entry (ilist, struct dm_node, list);

            if((inode != NULL)&&((inode->type == DM_OBJECT)||(inode->type == DM_INSTANCE))&&(strcmp(inode->name,path) == 0))
            {
                do
                {
                    path  = strtok (NULL, ".");
                    if((path != NULL)&&(strcmp(path,"{i}") == 0))
                    {
                        path  = strtok (NULL, ".");
                    }
                    if(path == NULL)
                    {
                        return DM_GEN_ERR;
                    }

                    __list_for_each(jlist, &(inode->head_child))
                    {
                        jnode = (struct dm_node *) list_entry (jlist, struct dm_node, list);

                        if((jnode != NULL)&&((jnode->type == DM_OBJECT)||(jnode->type == DM_INSTANCE))&&(strcmp(path,jnode->name) == 0))
                        {
                            inode = jnode;
                            break;
                        }
                    }
                }
                while((jnode != NULL)&&(strcmp(path,jnode->name) == 0));

                if((inode->type == DM_OBJECT)||(inode->type == DM_INSTANCE))
                {
                    list_add_tail(&(node->list), &(inode->head_child));
                }
                if((path != NULL)&&(node->type == DM_OBJECT))
                {
                    node->name = strdup(path);
                }
                goto finally;
            }
        }
        list_add_tail(&(node->list), &(head_dm_tree));
        if((path != NULL)&&(node->type == DM_OBJECT))
        {
            node->name = strdup(path);
        }
    }
    else
    {
        list_add_tail(&(node->list), &(head_dm_tree));
        if((path != NULL)&&(node->type == DM_OBJECT))
        {
            node->name = strdup(path);
        }
    }

finally:
    if(path != NULL)
    {
        path  = strtok (NULL, ".");
        if(path != NULL)
        {
            if((node->type == DM_OBJECT)&&(strcmp(path,"{i}") == 0))
            {
                node->type = DM_INSTANCE;
            }
            else
            {
                error = DM_GEN_ERR;
            }
        }
    }
    else
    {
        error = DM_GEN_ERR;
    }
    return error;
}

int dm_free_node (struct dm_node *node)
{
    char                        *pstruct_dm_regexp;
    char                        *pstruct_dm_string;
    char                        *pstruct_dm_debug;
    char                        *pstruct_dm_default_value;
    struct dm_uci               *pstruct_dm_uci;
    struct dm_system            *pstruct_dm_system;
    struct dm_correspondence    *pstruct_dm_correspondence;
    struct data_handler         **data_handler;
    char                        **dm_map,**el;
    char                        ***dm_data;
    int                         k;
    unsigned short              i,j;

    if(node != NULL)
    {

        if(node->name != NULL)
        {
            free(node->name);
        }
        for(i=0; i<node->size_data_handler; i++)
        {
            if(data_handler[i] != NULL)
            {
                switch(data_handler[i]->type)
                {
                    case DM_CORRESPONDENCE:
                        pstruct_dm_correspondence = (struct dm_correspondence *)data_handler[i]->handler;
                        if(pstruct_dm_correspondence != NULL)
                        {
                            if(pstruct_dm_correspondence->dm_data != NULL)
                            {
                                dm_data = pstruct_dm_correspondence->dm_data->data;
                                for (j=0;j<pstruct_dm_correspondence->dm_data->line_size;j++)
                                {
                                    for (j=0;j<pstruct_dm_correspondence->dm_data->column_size;j++)
                                    {
                                        el = *dm_data;
                                        if(*el != NULL)
                                        {
                                            free(*el);
                                        }
                                        el++;
                                    }
                                }
                                free(pstruct_dm_correspondence->dm_data);
                            }
                            if(pstruct_dm_correspondence->dm_map != NULL)
                            {
                                dm_map = pstruct_dm_correspondence->dm_map->map;
                                for (j=0;j<pstruct_dm_correspondence->dm_map->size;j++)
                                {
                                    if(dm_map[j] != NULL)
                                    {
                                        free(dm_map[j]);
                                    }
                                }
                                free(pstruct_dm_correspondence->dm_map);
                            }
                            free(pstruct_dm_correspondence);
                        }
                    break;
                    case DM_SYSTEM:
                        pstruct_dm_system = (struct dm_system *)data_handler[i]->handler;
                        if(pstruct_dm_system != NULL)
                        {
                            if(pstruct_dm_system->cmd != NULL)
                            {
                                free(pstruct_dm_system->cmd);
                            }
                            if(pstruct_dm_system->dm_map != NULL)
                            {
                                dm_map = pstruct_dm_system->dm_map->map;
                                for (j=0;j<pstruct_dm_system->dm_map->size;j++)
                                {
                                    if(dm_map[j] != NULL)
                                    {
                                        free(dm_map[j]);
                                    }
                                }
                                free(pstruct_dm_system->dm_map);
                            }
                            free(pstruct_dm_system);
                        }
                        break;
                    case DM_UCI:
                        pstruct_dm_uci = (struct dm_uci *)data_handler[i]->handler;
                        if(pstruct_dm_uci != NULL)
                        {
                            if(pstruct_dm_uci->cmd != NULL)
                            {
                                free(pstruct_dm_uci->cmd);
                            }
                            free(pstruct_dm_uci);
                        }
                        break;
                    case DM_DEBUG:
                        pstruct_dm_debug = (char *)data_handler[i]->handler;
                        if(pstruct_dm_debug != NULL)
                        {
                            free(pstruct_dm_debug);
                        }
                        break;
                    case DM_STRING:
                        pstruct_dm_string = (char *)data_handler[i]->handler;
                        if(pstruct_dm_string != NULL)
                        {
                            free(pstruct_dm_string);
                        }
                        break;
                    case DM_DEFAULT_VALUE:
                        pstruct_dm_default_value = (char *)data_handler[i]->handler;
                        if(pstruct_dm_default_value != NULL)
                        {
                            free(pstruct_dm_default_value);
                        }
                        break;
                    case DM_REGEXP:
                        pstruct_dm_regexp = (char *)data_handler[i]->handler;
                        if(pstruct_dm_regexp != NULL)
                        {
                            free(pstruct_dm_regexp);
                        }
                        break;
                }
            }
            free(data_handler[i]);
        }
        free(node);
    }
    return DM_OK;
}

int dm_get_handler(struct dm_node *node,struct list_head *list)
{
    bool                        contain_get = FALSE;
    struct dm_system            *pstruct_dm_system;
    struct data_handler         **data_handler;
    struct list_data_handler    *idata_handler;
    char                        name[512];
    struct dm_node              *tmp_dm_head;

    if(!list_empty(list))
    {
        data_handler = calloc(node->size_data_handler,sizeof(struct data_handler*));
        node->data_handler = data_handler;
    }
    while(!list_empty(list))
    {
        idata_handler = list_entry (list->next, struct list_data_handler, list);

        if(idata_handler != NULL)
        {
            *data_handler = idata_handler->data_handler;
            switch(idata_handler->data_handler->type)
            {
                case DM_SYSTEM:
                    pstruct_dm_system = (struct dm_system *)idata_handler->data_handler->handler;
                    switch(pstruct_dm_system->type)
                    {
                        case DM_GET:
                            contain_get = TRUE;
                            break;
                        case DM_SET:
                            break;
                        case DM_APPLY:
                            break;
                    }
                break;
                case DM_UCI:
                    contain_get = TRUE;
                    break;
                case DM_STRING:
                    contain_get = TRUE;
                    break;
                case DM_DEFAULT_VALUE:
                    contain_get = TRUE;
                    break;
            }
            data_handler++;
            list_del(list->next);
            if(idata_handler != NULL)
            {
                free(idata_handler);
                idata_handler = NULL;
            }
        }
    }
    if(node->type == DM_PARAMETER)
    {
        if((node->size_data_handler == 0) || (contain_get == FALSE))
        {
            DM_LOG(DEBUG,"PARAMETER:loading:NO :%s%s",last_object_name,node->name);
            if(forced_inform_parameter != NULL)
            {
                if(forced_inform_parameter->name != NULL)
                {
                    free(forced_inform_parameter->name);
                }
                free(forced_inform_parameter);
                forced_inform_parameter = NULL;
            }
            dm_free_node(node);
        }
        else
        {
            dm_add_node(node);
            if(forced_inform_parameter != NULL)
            {
                list_add_tail(&(forced_inform_parameter->list),&(forced_inform_parameter_list));
            }
            name[0] = 0;
            while(!list_empty(&head_dm_tree))
            {
                tmp_dm_head = (struct dm_node *) list_entry (head_dm_tree.next, struct dm_node, list);
                if(strstr(last_object_name,tmp_dm_head->name) != NULL)
                {
                    break;
                }
            }
            if(node->active_notify.force_default_enabled)
            {
                sprintf(name,"%s%s",last_object_name,node->name);
                if(cwmp_check_notification(name,UCI_NOTIFICATION_PASSIVE_PATH) == CWMP_OK &&
                   cwmp_check_notification(name,UCI_NOTIFICATION_DENIED_PATH) == CWMP_OK)
                {
                    cwmp_apply_active_notification(node,name, xsd__boolean__true_);
                }
                tmp_dm_head->active_notify.force_default_enabled = TRUE;
            }
            if(node->active_notify.force_enabled)
            {
                sprintf(name,"%s%s",last_object_name,node->name);
                cwmp_apply_active_notification(node,name, xsd__boolean__true_);
                tmp_dm_head->active_notify.force_enabled = TRUE;
            }
            if(node->active_notify.can_deny)
            {
                tmp_dm_head->active_notify.can_deny = TRUE;
            }
            DM_LOG(DEBUG,"PARAMETER:loading:YES:%s%s",last_object_name,node->name);
        }
    }
    return DM_OK;
}

void start_hndl(void *data, const char *el, const char **attr)
{
    int                         i,j,type_idx = 0;
    struct data_handler         **data_handler;
    struct list_data_handler    *idata_handler;
    struct list_head            *ilist;
    struct dm_uci               *pstruct_dm_uci;
    struct dm_system            *pstruct_dm_system;
    char                        forced_inform_name[256];

    if((last_object_name == NULL)           &&
       ((strcmp(el,"parameter") == 0)       ||
        (strcmp(el,"system") == 0)          ||
        (strcmp(el,"uci")   ==0)            ||
#ifdef WITH_DM_XML_DEBUG
        (strcmp(el,"debug") == 0)           ||
#endif
        (strcmp(el,"correspondence") == 0)  ||
        (strcmp(el,"string") == 0)))
    {
        DM_LOG(ERROR,"data model xml file not well formed");
        exit(EXIT_FAILURE);
    }
    if((strcmp(el,"object") == 0)||(strcmp(el,"parameter") == 0))
    {
        if(strcmp(el,"object") == 0)
        {
            node = calloc(1,sizeof(struct dm_node));
            INIT_LIST_HEAD(&(node->head_child));
            current_object = node;
        }
        if(strcmp(el,"parameter") == 0)
        {
            node = (struct dm_node *) calloc(1,sizeof(struct dm_node_leaf));
        }
        node->size_data_handler = 0;
        for (i = 0; attr[i]; i += 2)
        {
            if(strcmp(attr[i],"name") == 0)
            {
                if(strcmp(el,"object") == 0)
                {
                    last_object_name = strdup(attr[i + 1]);
                    node->name = NULL;
                    node->type = DM_OBJECT;
                }
                if(strcmp(el,"parameter") == 0)
                {
                    node->name = strdup(attr[i + 1]);
                    node->type = DM_PARAMETER;
                }
            }
            if(strcmp(attr[i],"access") == 0)
            {
                if(strcmp(el,"object") == 0)
                {
                    if(strcmp(attr[i + 1],"readOnly") == 0)
                    {
                        node->permission = DM_PRESENT;
                    }
                    if(strcmp(attr[i + 1],"readWrite") == 0)
                    {
                        node->permission = DM_CREATE;
                    }
                }
                if(strcmp(el,"parameter") == 0)
                {
                    if(strcmp(attr[i + 1],"readOnly") == 0)
                    {
                        node->permission = DM_READ;
                    }
                    if(strcmp(attr[i + 1],"readWrite") == 0)
                    {
                        node->permission = DM_READ_WRITE;
                    }
                }
            }
            if(strcmp(attr[i],"defaultValue") == 0)
            {
                tmp_handler = calloc(1,sizeof(struct list_data_handler));

                if(tmp_handler != NULL)
                {
                    tmp_handler->data_handler = calloc(1,sizeof(struct data_handler));
                    if(node == current_object)
                    {
                        list_add_tail(&(tmp_handler->list),&(object_handler_list));
                    }
                    else
                    {
                        list_add_tail(&(tmp_handler->list),&(parameter_handler_list));
                    }
                }
                tmp_handler->data_handler->type = DM_DEFAULT_VALUE;
                tmp_handler->data_handler->handler = (void *)strdup(attr[i + 1]);
                node->size_data_handler++;
            }
            if(strcmp(attr[i],"regExp") == 0)
            {
                tmp_handler = calloc(1,sizeof(struct list_data_handler));

                if(tmp_handler != NULL)
                {
                    tmp_handler->data_handler = calloc(1,sizeof(struct data_handler));
                    if(node == current_object)
                    {
                        list_add_tail(&(tmp_handler->list),&(object_handler_list));
                    }
                    else
                    {
                        list_add_tail(&(tmp_handler->list),&(parameter_handler_list));
                    }
                }
                tmp_handler->data_handler->type = DM_REGEXP;
                tmp_handler->data_handler->handler = (void *)strdup(attr[i + 1]);
                node->size_data_handler++;
            }
            if(strcmp(attr[i],"forcedInform") == 0)
            {
                if(strcmp(el,"parameter") == 0)
                {
                    if(strcmp(attr[i + 1],"true") == 0)
                    {
                        forced_inform_parameter = calloc(1,sizeof(struct forced_inform_parameter));
                        if(forced_inform_parameter != NULL)
                        {
                            sprintf(forced_inform_name,"%s%s",last_object_name,node->name);
                            forced_inform_parameter->name = strdup(forced_inform_name);
                            forced_inform_parameter->node = (struct dm_node_leaf *)node;
                        }
                    }
                }
            }
            if(strcmp(attr[i],"activeNotify") == 0)
            {
                if(strcmp(attr[i + 1],"forceDefaultEnabled") == 0)
                {
                    node->active_notify.force_default_enabled = TRUE;
                    if(node->type == DM_PARAMETER)
                    {
                        current_object->active_notify.force_default_enabled = TRUE;
                    }
                }
                else if(strcmp(attr[i + 1],"forceEnabled") == 0)
                {
                    node->active_notify.force_enabled = TRUE;
                    if(node->type == DM_PARAMETER)
                    {
                        current_object->active_notify.force_enabled = TRUE;
                    }
                }
                else if(strcmp(attr[i + 1],"canDeny") == 0)
                {
                    node->active_notify.can_deny = TRUE;
                    if(node->type == DM_PARAMETER)
                    {
                        current_object->active_notify.can_deny = TRUE;
                    }
                }
            }
            if(strcmp(attr[i],"type") == 0)
            {
                if(node->type == DM_PARAMETER)
                {
                    struct dm_node_leaf *leaf = (struct dm_node_leaf *)node;
                    for (j=0;j<COUNT_TYPE_VALUES;j++)
                    {
                        if(strcmp(attr[i + 1],TYPE_VALUES_ARRAY[j]) == 0)
                        {
                            type_idx = j;
                            break;
                        }
                    }
                    leaf->value_type = type_idx;
                }
            }
        }
    }
    if( (strcmp(el,"system") == 0)          ||
        (strcmp(el,"uci")   ==0)            ||
#ifdef WITH_DM_XML_DEBUG
        (strcmp(el,"debug") == 0)           ||
#endif
        (strcmp(el,"correspondence") == 0)  ||
        (strcmp(el,"string") == 0))
    {
        tmp_handler = calloc(1,sizeof(struct list_data_handler));

        if(tmp_handler != NULL)
        {
            tmp_handler->data_handler = calloc(1,sizeof(struct data_handler));
            if(node == current_object)
            {
                list_add_tail(&(tmp_handler->list),&(object_handler_list));
            }
            else
            {
                list_add_tail(&(tmp_handler->list),&(parameter_handler_list));
            }
        }
        if(last_tag_name != NULL)
        {
            DM_LOG(ERROR,"data model xml file not well formed");
            exit(EXIT_FAILURE);
        }
        else
        {
            last_tag_name = strdup(el);
        }
        node->size_data_handler++;

    }
    if(strcmp(el,"system") == 0)
    {
        if(tmp_handler->data_handler != NULL)
        {
            tmp_handler->data_handler->type = DM_SYSTEM;
            tmp_handler->data_handler->handler = (void *)calloc(1,sizeof(struct dm_system));
        }

        pstruct_dm_system = (struct dm_system *)tmp_handler->data_handler->handler;
        /*
         * Setting SYSTEM default values
         */
        pstruct_dm_system->type = DM_GET;
        pstruct_dm_system->reboot_required = FALSE;
        pstruct_dm_system->end_session = FALSE;
        for (i = 0; attr[i]; i += 2)
        {
            if(strcmp(attr[i],"type") == 0)
            {
                if(strcmp(attr[i+1],"get") == 0)
                {
                    pstruct_dm_system->type = DM_GET;
                }
                if(strcmp(attr[i+1],"set") == 0)
                {
                    pstruct_dm_system->type = DM_SET;
                }
                if(strcmp(attr[i+1],"apply") == 0)
                {
                    pstruct_dm_system->type = DM_APPLY;
                }
                if(strcmp(attr[i+1],"cancel") == 0)
                {
                    pstruct_dm_system->type = DM_CANCEL;
                }
                if(strcmp(attr[i+1],"addObject") == 0)
                {
                    pstruct_dm_system->type = DM_ADD;
                }
                if(strcmp(attr[i+1],"deleteObject") == 0)
                {
                    pstruct_dm_system->type = DM_DEL;
                }
            }
            if(strcmp(attr[i],"rebootRequired") == 0)
            {
                if(strcmp(attr[i+1],"false") == 0)
                {
                    pstruct_dm_system->reboot_required = FALSE;
                }
                if(strcmp(attr[i+1],"true") == 0)
                {
                    pstruct_dm_system->reboot_required = TRUE;
                }
            }
            if(strcmp(attr[i],"endSession") == 0)
            {
                if(strcmp(attr[i+1],"false") == 0)
                {
                    pstruct_dm_system->end_session = FALSE;
                }
                if(strcmp(attr[i+1],"true") == 0)
                {
                    pstruct_dm_system->end_session = TRUE;
                }
            }
        }
    }
    if(strcmp(el,"uci") == 0)
    {
        if(tmp_handler->data_handler != NULL)
        {
            tmp_handler->data_handler->type = DM_UCI;
            tmp_handler->data_handler->handler = (void *)calloc(1,sizeof(struct dm_uci));
        }

        pstruct_dm_uci = (struct dm_uci *)tmp_handler->data_handler->handler;
        /*
         * Setting UCI default values
         */
        pstruct_dm_uci->reboot_required = FALSE;
        pstruct_dm_uci->end_session = FALSE;
        for (i = 0; attr[i]; i += 2)
        {
            if(strcmp(attr[i],"rebootRequired") == 0)
            {
                if(strcmp(attr[i+1],"false") == 0)
                {
                    pstruct_dm_uci->reboot_required = FALSE;
                }
                if(strcmp(attr[i+1],"true") == 0)
                {
                    pstruct_dm_uci->reboot_required = TRUE;
                }
            }
            if(strcmp(attr[i],"endSession") == 0)
            {
                if(strcmp(attr[i+1],"false") == 0)
                {
                    pstruct_dm_uci->end_session = FALSE;
                }
                if(strcmp(attr[i+1],"true") == 0)
                {
                    pstruct_dm_uci->end_session = TRUE;
                }
            }
        }
    }
#ifdef WITH_DM_XML_DEBUG
    if(strcmp(el,"debug") == 0)
    {
        if(tmp_handler->data_handler != NULL)
        {
            tmp_handler->data_handler->type = DM_DEBUG;
        }
    }
#endif
    if(strcmp(el,"correspondence") == 0)
    {
        if(tmp_handler->data_handler != NULL)
        {
            tmp_handler->data_handler->type = DM_CORRESPONDENCE;
            tmp_handler->data_handler->handler = (void *)calloc(1,sizeof(struct dm_correspondence));
        }
    }
    if(strcmp(el,"string") == 0)
    {
        if(tmp_handler->data_handler != NULL)
        {
            tmp_handler->data_handler->type = DM_STRING;
        }
    }
    if(strcmp(el,"object") == 0)
    {
        dm_add_node(node);
        DM_LOG(DEBUG,"OBJECT   :loading:YES:%s",last_object_name);
    }
    Depth++;
}

void end_hndl(void *data, const char *el)
{
    struct dm_data              *dm_data=NULL;
    struct dm_map               *dm_map=NULL;
    char                        *pstruct_dm_string;
    char                        *pstruct_dm_debug;
    struct dm_uci               *pstruct_dm_uci;
    struct dm_system            *pstruct_dm_system;
    struct dm_correspondence    *pstruct_dm_correspondence;

    if(strcmp(el,"parameter") == 0)
    {
        dm_get_handler(node,&parameter_handler_list);
        node = current_object;
        forced_inform_parameter = NULL;
    }
    if(strcmp(el,"object") == 0)
    {
        if(last_object_name != NULL)
        {
            free(last_object_name);
            last_object_name = NULL;
        }
        dm_get_handler(node,&object_handler_list);
        node = NULL;
        current_object = NULL;
    }
    if(tmp_handler != NULL)
    {
#ifdef WITH_DM_XML_DEBUG
        if(strcmp(el,"debug") == 0)
        {
            if((last_tag_name != NULL)&&(strcmp(el,last_tag_name) == 0))
            {
            tmp_handler->data_handler->handler = (void *)strdup(last_content);
            DM_XML_LOG(INFO,"%s",last_content);
        }
            else
            {
                DM_LOG(ERROR,"data model xml file not well formed");
                exit(EXIT_FAILURE);
            }
        }
#endif
        if(strcmp(el,"string") == 0)
        {
            if((last_tag_name != NULL)&&(strcmp(el,last_tag_name) == 0))
            {
            tmp_handler->data_handler->handler = (void *)strdup(last_content);
        }
            else
            {
                DM_LOG(ERROR,"data model xml file not well formed");
                exit(EXIT_FAILURE);
            }
        }
        if(strcmp(el,"cmd") == 0)
        {
            if((last_tag_name != NULL)&&((strcmp(last_tag_name,"uci") == 0)||(strcmp(last_tag_name,"system") == 0)))
            {
            if(tmp_handler->data_handler->type == DM_UCI)
            {
                pstruct_dm_uci      = (struct dm_uci *)tmp_handler->data_handler->handler;
                pstruct_dm_uci->cmd = strdup(last_content);
            }
            if(tmp_handler->data_handler->type == DM_SYSTEM)
            {
                pstruct_dm_system       = (struct dm_system *)tmp_handler->data_handler->handler;
                pstruct_dm_system->cmd  = strdup(last_content);
            }
        }
            else
            {
                DM_LOG(ERROR,"data model xml file not well formed");
                exit(EXIT_FAILURE);
            }

        }

        if(strcmp(el,"data") == 0)
        {
            if((last_tag_name != NULL)&&(strcmp(last_tag_name,"correspondence") == 0))
            {
            if(tmp_handler->data_handler->type == DM_CORRESPONDENCE)
            {
                pstruct_dm_correspondence   = (struct dm_correspondence *)tmp_handler->data_handler->handler;
                dm_data                     = calloc(1,sizeof(struct dm_data));
                if(dm_data != NULL)
                {
                    dm_get_xml_data(last_content,dm_data);
                    pstruct_dm_correspondence->dm_data  = dm_data;
                }
            }
        }
            else
            {
                DM_LOG(ERROR,"data model xml file not well formed");
                exit(EXIT_FAILURE);
            }
        }

        if(strcmp(el,"map") == 0)
        {
            if((last_tag_name != NULL)&&((strcmp(last_tag_name,"correspondence") == 0)||(strcmp(last_tag_name,"system") == 0)))
            {
            if(tmp_handler->data_handler->type == DM_SYSTEM)
            {
                pstruct_dm_system   = (struct dm_system *)tmp_handler->data_handler->handler;
                dm_map              = calloc(1,sizeof(struct dm_map));
                if(dm_map != NULL)
                {
                    dm_get_xml_map(last_content,dm_map);
                    pstruct_dm_system->dm_map   = dm_map;
                }
            }
            if(tmp_handler->data_handler->type == DM_CORRESPONDENCE)
            {
                pstruct_dm_correspondence   = (struct dm_correspondence *)tmp_handler->data_handler->handler;
                dm_map                      = calloc(1,sizeof(struct dm_map));
                if(dm_map != NULL)
                {
                    dm_get_xml_map(last_content,dm_map);
                    pstruct_dm_correspondence->dm_map   = dm_map;
                }
            }
        }
            else
            {
                DM_LOG(ERROR,"data model xml file not well formed");
                exit(EXIT_FAILURE);
            }
        }
    }
    if( (strcmp(el,"system") == 0)          ||
        (strcmp(el,"uci")   ==0)            ||
#ifdef WITH_DM_XML_DEBUG
        (strcmp(el,"debug") == 0)           ||
#endif
        (strcmp(el,"correspondence") == 0)  ||
        (strcmp(el,"string") == 0))
    {
        switch(tmp_handler->data_handler->type)
        {
            case DM_SYSTEM:
                pstruct_dm_system = (struct dm_system *)tmp_handler->data_handler->handler;
                if(pstruct_dm_system->cmd == NULL)
                {
                    DM_LOG(ERROR,"data model xml file not well formed");
                    exit(EXIT_FAILURE);
                }
                break;
            case DM_UCI:
                pstruct_dm_uci = (struct dm_uci *)tmp_handler->data_handler->handler;
                if(pstruct_dm_uci->cmd == NULL)
                {
                    DM_LOG(ERROR,"data model xml file not well formed");
                    exit(EXIT_FAILURE);
                }
                break;
            case DM_CORRESPONDENCE:
                pstruct_dm_correspondence = (struct dm_correspondence *)tmp_handler->data_handler->handler;
                if((pstruct_dm_correspondence->dm_data == NULL)||(pstruct_dm_correspondence->dm_map == NULL))
                {
                    DM_LOG(ERROR,"data model xml file not well formed");
                    exit(EXIT_FAILURE);
                }
                break;
            case DM_STRING:
                pstruct_dm_string = (char *)tmp_handler->data_handler->handler;
                if(pstruct_dm_string == NULL)
                {
                    DM_LOG(ERROR,"data model xml file not well formed");
                    exit(EXIT_FAILURE);
                }
                break;
            case DM_DEBUG:
                pstruct_dm_debug = (char *)tmp_handler->data_handler->handler;
                if(pstruct_dm_debug == NULL)
                {
                    DM_LOG(ERROR,"data model xml file not well formed");
                    exit(EXIT_FAILURE);
                }
                break;
        }
        tmp_handler = NULL;
        if(last_tag_name != NULL)
        {
            free(last_tag_name);
            last_tag_name = NULL;
        }
    }
    Depth--;
}

void char_hndl(void *data, const char *content,int length)
{
    char            tmp[512];
    int             i = 0,n = 0;

    if(length!=0)
    {
        strncpy(tmp, content, length);
        tmp[length] = '\0';
        while(i<length)
        {
            if(tmp[i]==' ')
            {
                n++;
            }
            i++;
        }
        if(n != length)
        {
            strcpy(last_content,tmp);
        }
        else
        {
            last_content[0] = 0;
        }
    }
}

int dm_parse_xml_file(FILE *pFile,char *filename)
{
    int     max,done = 0;
    char    *buff;

    XML_Parser parser = XML_ParserCreate(NULL);
    if (! parser)
    {
        DM_LOG(ERROR,"Couldn't allocate memory for parser");
        return DM_MEM_ERR;
    }

    XML_UseParserAsHandlerArg(parser);
    XML_SetElementHandler(parser, start_hndl, end_hndl);
    XML_SetCharacterDataHandler(parser, char_hndl);

    max = get_xml_file_size(pFile);

    buff = calloc(1,max);
    if(buff == NULL)
    {
        return DM_MEM_ERR;
    }
    for (fread(buff, sizeof(char), max, pFile); !feof(pFile); fread(buff, sizeof(char), max, pFile))
    {
        if (! XML_Parse(parser, buff, max, done))
        {
            DM_LOG(ERROR,"%s Parse error at line %d: %s",filename,(int)XML_GetCurrentLineNumber(parser),XML_ErrorString(XML_GetErrorCode(parser)));
            return DM_ORF_ERR;
        }
    }
    if(last_tag_name != NULL)
    {
        DM_LOG(ERROR,"data model xml file not well formed");
        exit(EXIT_FAILURE);
    }
    if(buff != NULL)
    {
        free(buff);
    }
    XML_ParserFree(parser);
    return DM_OK;
}

int dm_xml_init_tree(char *filename)
{

    FILE           *pFile;

    pFile = fopen(filename, "r");
    if(pFile != NULL)
    {
        if(dm_parse_xml_file(pFile,filename) == DM_ORF_ERR)
        {
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        return DM_ORF_ERR;
    }
    fclose(pFile);
    return DM_OK;
}

int  dm_match_patterns(char *pch,char *pattern)
{
    regex_t             *regex;
    regmatch_t          *result;
    int                 err_no = 0;
    int                 start = 0;

    regex = (regex_t *) calloc(1,sizeof(regex_t));
    if((err_no = regcomp(regex, pattern, REG_EXTENDED)) != 0)
    {
        size_t          length;
        char            *buffer;
        length = regerror (err_no, regex, NULL, 0);
        buffer = malloc(length);
        regerror (err_no, regex, buffer, length);
        DM_LOG("%s", buffer);
        free(buffer);
        regfree(regex);
        return DM_GEN_ERR;
    }
    result = (regmatch_t *) calloc(1,sizeof(regmatch_t));
    if(result == NULL)
    {
        return DM_GEN_ERR;
    }
    while(regexec(regex, pch+start, 1, result, 0) == 0)
    {
        start +=result->rm_eo;
    }
    regfree(regex);
    free(regex);
    if((result->rm_so == 0)&&(result->rm_eo == strlen(pch)))
    {
        return DM_OK;
    }
    return DM_GEN_ERR;
}

/*********************************************************************************************/
/**                                     browse tree                                         **/
/*********************************************************************************************/

int is_numeric(const char *p)
{
    if (*p)
    {
        char c;
        while ((c=*p++))
        {
            if (!isdigit(c)) return 0;
        }
        return 1;
    }
    return 0;
}

static int get_data_array_by_column (struct dm_index_path *index_path,
                                    int pos_xp,
                                    struct dm_data **data,
                                    unsigned short *ic,
                                    char *map_column)
{
    struct dm_data  *d;
    struct dm_map   *m;
    unsigned short  i;
    m = index_path[pos_xp].map;
    d = index_path[pos_xp].data;
    if (m==NULL||d==NULL)
    {
        *data = NULL;
        *ic = 0;
        return 1;
    }
    for (i=0;i<m->size;i++)
    {
        if (strcmp(map_column,m->map[i])==0)
        {
            *ic=i;
            if (i<d->column_size)
            {
                *data = d;
                return 0;
            }
            else
            {
                *data = NULL;
                return 1;
            }
        }
    }
    *data = NULL;
    return 1;
}


static int get_data_array_correspondence (  struct dm_index_path *index_path,
                                            int pos_xp,
                                            char *map_corr,
                                            char *corr)
{
    struct dm_data          *d;
    unsigned short          i,ic,mc,error;
    struct dm_index_path    *exp;
    exp = &index_path[pos_xp];
    if (error = get_data_array_by_column (index_path,pos_xp,&d,&ic,"index"))
    {
        return error;
    }
    if (error = get_data_array_by_column (index_path,pos_xp,&d,&mc,map_corr))
    {
        return error;
    }
    for (i=0;i<d->line_size;i++)
    {
        if (strcmp(exp->index,d->data[i][ic])==0)
        {
            strcpy(corr,d->data[i][mc]);
            return 0;
        }
    }
    return 1;
}

static int check_index_exist(struct dm_index_path *index_path,
                            int pos_xp,
                            char *index)
{
    struct dm_data          *d;
    unsigned short          i,ic,error;
    struct dm_index_path    *exp;
    exp = &index_path[pos_xp];
    if (error = get_data_array_by_column (index_path,pos_xp,&d,&ic,"index"))
    {
        return error;
    }
    for (i=0;i<d->line_size;i++)
    {
        if (strcmp(index,d->data[i][ic])==0)
        {
            exp->indice = i;
            return 0;
        }
    }
    return 1;
}

int get_string_correspondence (char *ret, char *src, struct dm_index_path *index_path, int pos_xp)
{
    char    *t,*pch,*s,*is;
    char    corr[128];
    int     i,len,error = 0;

    ret[0]  = 0;
    s       = src;
    is      = src;
    if (index_path==NULL)
    {
        strcpy(ret,src);
        return 0;
    }
    while (s=strchr(s,'$'))
    {
        len     = strlen(ret);
        if ((s-is)<0)
        {
            return 1;
        }
        memcpy(ret+len,is,s-is);
        ret[len+s-is] = 0;
        s++;
        is = s;
        pch     = strdup(s);
        t       = pch;
        pch     = strtok(pch,"{");
        if (pch==NULL)
        {
            free(t);
            return 1;
        }
        is  = is + strlen(pch) +1;
        i   = atoi(pch);
        if (i<1)
        {
            free(t);
            return 1;
        }
        pch = strtok(NULL,"}");
        if (pch==NULL)
        {
            free(t);
            return 1;
        }
        is  = is + strlen(pch) +1;
        if (error = get_data_array_correspondence (index_path,i-1,pch,corr))
        {
            free(t);
            return error;
        }
        strcat(ret,corr);
        free(t);
    }
    if ((is-src)<strlen(src))
    {
        strcat(ret,is);
    }

    return 0;
}

int get_data_array_indexes (struct dm_index_path *index_path,
                            int pos_xp,
                            struct dm_data **data,
                            unsigned short *ic)
{
    int error;
    error = get_data_array_by_column (index_path,pos_xp,data,ic,"index");
    return error;
}

int get_data_array_correspondence_column (struct dm_index_path *index_path,
                                          int pos_xp,
                                          struct dm_data **data,
                                          unsigned short *cc)
{
    int error;
    error = get_data_array_by_column (index_path,pos_xp,data,cc,"correspondence");
    return error;
}

int add_element_node_path ( struct dm_node *node,
                            struct dm_node_path *node_path,
                            int *pos_np)
{
    struct dm_node_path         *enp;

    enp = &node_path[*pos_np];

    enp->node       = node;
    enp->index_path = NULL;
    *pos_np         +=1;
    return DM_OK;
}

int free_element_node_path (struct dm_node_path *node_path, int *pos_np, int *pos_xp)
{
    struct dm_node_path         *enp;
    struct dm_index_path        *exp;

    *pos_np -=1;
    enp     = &node_path[*pos_np];
    exp     = enp->index_path;

    if (exp!=NULL)
    {
        free_element_index_path(exp,pos_xp);
    }
    memset(enp,0,sizeof(struct dm_node_path));
    return DM_OK;
}

int free_element_index_path (struct dm_index_path *exp, int *pos_xp)
{
    *pos_xp -=1;
    if (exp->data!=NULL && exp->data_type == DM_SYSTEM_DATA)
    {
        dm_free_data_array(exp->data);
    }
    if (exp->index!=NULL)
    {
        free(exp->index);
    }

    memset(exp,0,sizeof(struct dm_index_path));
    return DM_OK;
}

int free_all_index_path (struct dm_index_path *index_path)
{
    int                     i;
    struct dm_index_path    *exp;
    for (i=0;i<INDEX_PATH_SIZE;i++)
    {
        exp = &index_path[i];
        if (exp->data!=NULL && exp->data_type == DM_SYSTEM_DATA)
        {
            dm_free_data_array(exp->data);
        }
        if (exp->index!=NULL)
        {
            free(exp->index);
        }
    }
    return DM_OK;
}


int add_element_index_path (struct dm_node *node,
                            struct dm_node_path *node_path,
                            int *pos_np,
                            struct dm_index_path *index_path,
                            int *pos_xp,
                            char *index,
                            int indice)
{
    struct dm_node_path         *enp;
    struct dm_index_path        *exp;
    struct data_handler         **data_handler;
    __u8                        i,size_dh;
    char                        cmd[256];

    enp = &node_path[(*pos_np)-1];

    if (node == NULL || node->type != DM_INSTANCE)
    {
        return DM_ERR;
    }

    if (enp->index_path!=NULL)
    {
        exp = enp->index_path;
        if (exp->index!=NULL)
        {
            free(exp->index);
        }
        exp->index = strdup(index);
        if (indice>=0)
        {
            exp->indice = indice;
        }
        return DM_OK;
    }
    else
    {
        exp = &index_path[*pos_xp];
    }

    enp->index_path = exp;
    data_handler    = node->data_handler;
    size_dh         = node->size_data_handler;
    *pos_xp         +=1;

    for (i=0;i<size_dh;i++)
    {
        if (data_handler[i]->type == DM_CORRESPONDENCE)
        {
            struct dm_correspondence    *corresp;
            corresp         = (struct dm_correspondence *)data_handler[i]->handler;
            exp->data       = corresp->dm_data;
            exp->map        = corresp->dm_map;
            exp->data_type  = DM_XML_DATA;
            exp->node_path  = enp;
            exp->index      = (index!=NULL) ? strdup(index) : NULL;
            if (indice>=0)
            {
                exp->indice = indice;
            }
            break;
        }
        else if (data_handler[i]->type == DM_SYSTEM)
        {
            struct dm_system            *system;
            struct dm_data              *dm_data;
            system  = (struct dm_system *)data_handler[i]->handler;
            if (system->type != DM_GET)
            {
                continue;
            }
            dm_data = calloc(1,sizeof(struct dm_data));
            assert(dm_data);
            get_string_correspondence (cmd,system->cmd,index_path,(*pos_xp)-1);
            dm_get_system_data_array(cmd,dm_data);
            exp->data       = dm_data;
            exp->map        = system->dm_map;
            exp->data_type  = DM_SYSTEM_DATA;
            exp->node_path  = enp;
            exp->index      = (index!=NULL) ? strdup(index) : NULL;
            if (indice>=0)
            {
                exp->indice = indice;
            }
            break;
        }
    }
    return DM_OK;
}

int dm_browse_subtree (struct list_head *list,
                       struct dm_rpc *dm_rpc,
                       struct dm_node_path *node_path,
                       int *pos_np,
                       struct dm_index_path *index_path,
                       int *pos_xp)
{
    struct list_head                            *ilist,*jlist;
    struct dm_node                              *node=NULL;
    struct dm_data                              *data=NULL;
    int                                         error,subtree;
    unsigned short                              i,ic;

    error = FAULT_CPE_NO_FAULT_IDX;
    __list_for_each(jlist,list)
    {
        node = list_entry(jlist,struct dm_node,list);
        add_element_node_path(node, node_path, pos_np);
        if ((node->type == DM_INSTANCE) && (error = add_element_index_path(node, node_path, pos_np, index_path, pos_xp, NULL, -1)))
        {
            DM_XML_GET_HANDLER_DEBUG(node,index_path,*pos_xp);
            error = FAULT_CPE_INVALID_PARAMETER_NAME_IDX;
            goto end_subtree;
        }
        DM_XML_GET_HANDLER_DEBUG(node,index_path,*pos_xp);
        if (error = dm_rpc->method(node, dm_rpc, node_path, *pos_np, index_path, *pos_xp))
        {
            goto end_subtree;
        }
        if (dm_rpc->subtree==FALSE)
        {
            goto end_subtree;
        }
        if (node->type == DM_INSTANCE)
        {
            if (get_data_array_indexes (index_path,(*pos_xp)-1,&data,&ic))
            {
                goto end_subtree;
            }
            ilist = &(node->head_child);
            for (i=0;i<data->line_size;i++)
            {
                if (error = add_element_index_path(node, node_path, pos_np, index_path, pos_xp, data->data[i][ic], i))
                {
                    error = FAULT_CPE_INVALID_PARAMETER_NAME_IDX;
                    goto end_subtree;
                }
                if (error = dm_rpc->method(node, dm_rpc, node_path, *pos_np, index_path, *pos_xp))
                {
                    goto end_subtree;
                }
                if (dm_rpc->subtree==FALSE)
                {
                    continue;
                }
                if (error = dm_browse_subtree (ilist,dm_rpc,node_path,pos_np,index_path,pos_xp))
                {
                    goto end_subtree;
                }
            }
        }
        else if (node->type == DM_OBJECT)
        {
            ilist = &(node->head_child);
            if (error = dm_browse_subtree (ilist,dm_rpc,node_path,pos_np,index_path,pos_xp))
            {
                goto end_subtree;
            }
        }
end_subtree:
        free_element_node_path(node_path, pos_np, pos_xp);
        if (error)
        {
            return error;
        }
    }
    return FAULT_CPE_NO_FAULT_IDX;
}

int dm_browse_tree (char *name, struct dm_rpc *dm_rpc)
{
    struct dm_node_path                         node_path[NODE_PATH_SIZE];
    struct dm_index_path                        index_path[INDEX_PATH_SIZE];
    int                                         pos_np=0,pos_xp=0;
    char                                        *pch,*t;
    struct list_head                            *ilist,*jlist;
    struct dm_node                              *node=NULL;
    bool                                        found,subtree,pch_is_numeric;
    int                                         namelen,error;

    error = FAULT_CPE_NO_FAULT_IDX;
    if (name[0]=='.')
    {
        DM_LOG(INFO,"Invalid path name: %s",name);
        return FAULT_CPE_INVALID_PARAMETER_NAME_IDX;
    }

    namelen         = strlen(name);
    pch             = strdup(name);
    t               = pch;
    pch             = strtok (pch,".");
    ilist           = &head_dm_tree;
    error           = FAULT_CPE_NO_FAULT_IDX;
    pch_is_numeric  = TRUE;
    memset(node_path,0,NODE_PATH_SIZE*sizeof(struct dm_node_path));
    memset(index_path,0,INDEX_PATH_SIZE*sizeof(struct dm_index_path));
    if (namelen!=0)
    {
        while (pch != NULL && (pch-t)<namelen)
        {
            if (is_numeric(pch))
            {
                if (pch_is_numeric || (error = check_index_exist (index_path, pos_xp-1, pch)))
                {
                    error = FAULT_CPE_INVALID_PARAMETER_NAME_IDX;
                    goto endbrowse;
                }
                if (error = add_element_index_path (node,node_path, &pos_np, index_path, &pos_xp, pch, -1))
                {
                    goto endbrowse;
                }
                pch_is_numeric = TRUE;
                pch = strtok (pch+strlen(pch)+1, ".");
                continue;
            }
            else
            {
                pch_is_numeric = FALSE;
            }
            found = FALSE;
            __list_for_each(jlist,ilist)
            {
                node = list_entry(jlist,struct dm_node,list);
                if(strcmp(pch,node->name)==0)
                {
                    found = TRUE;
                    if (node->type == DM_OBJECT ||
                        node->type == DM_INSTANCE)
                    {
                        ilist = &(node->head_child);
                    }
                    add_element_node_path(node,node_path, &pos_np);
                    if ((node->type == DM_INSTANCE) && (error = add_element_index_path(node, node_path, &pos_np, index_path, &pos_xp, NULL, -1)))
                    {
                        DM_XML_GET_HANDLER_DEBUG(node,index_path,pos_xp);
                        error = FAULT_CPE_INVALID_PARAMETER_NAME_IDX;
                        goto endbrowse;
                    }
                    DM_XML_GET_HANDLER_DEBUG(node,index_path,pos_xp);
                    break;
                }
            }
            if (!found)
            {
            	break;
            }
            pch = strtok (pch+strlen(pch)+1, ".");
        }

        if (!found ||
            ((node->type == DM_OBJECT || node->type == DM_INSTANCE) && name[namelen-1]!='.') ||
            (node->type == DM_PARAMETER && name[namelen-1]=='.'))
        {
            DM_LOG(INFO,"Invalid path name: %s",name);
            error = FAULT_CPE_INVALID_PARAMETER_NAME_IDX;
            goto endbrowse;
        }

    }

    if (error = dm_rpc->method(node, dm_rpc, node_path, pos_np, index_path, pos_xp))
    {
        goto endbrowse;
    }

    if (dm_rpc->subtree == TRUE)
    {
        if (node!=NULL && node->type==DM_INSTANCE && !pch_is_numeric)
        {
            struct dm_data                              *data=NULL;
            unsigned short                              i,ic;
            if (get_data_array_indexes (index_path,pos_xp-1,&data,&ic))
            {
                goto endbrowse;
            }
            for (i=0;i<data->line_size;i++)
            {
                if (error = add_element_index_path(node, node_path, &pos_np, index_path, &pos_xp, data->data[i][ic], i))
                {
                    error = FAULT_CPE_INVALID_PARAMETER_NAME_IDX;
                    goto endbrowse;
                }
                if (error = dm_rpc->method(node, dm_rpc, node_path, pos_np, index_path, pos_xp))
                {
                    goto endbrowse;
                }
                if (dm_rpc->subtree==FALSE)
                {
                    continue;
                }
                if (error = dm_browse_subtree (ilist,dm_rpc,node_path,&pos_np,index_path,&pos_xp))
                {
                    goto endbrowse;
                }
            }
        }
        else {
            error = dm_browse_subtree (ilist,dm_rpc,node_path,&pos_np,index_path,&pos_xp);
        }
        goto endbrowse;
    }

endbrowse:
    free_all_index_path (index_path);
    free(t);
    return error;
}

#ifdef WITH_DM_XML_DEBUG
int dm_get_handler_debug (struct dm_node *node, struct dm_index_path *index_path, int pos_xp)
{
    struct data_handler         **data_handler;
    __u8                        i,size_dh;
    char                        corr[512];

    data_handler    = node->data_handler;
    size_dh         = node->size_data_handler;

    for (i=0;i<size_dh;i++)
    {
        if (data_handler[i]->type == DM_DEBUG)
        {
            char        *debug;
            debug       = (char *)data_handler[i]->handler;
            get_string_correspondence (corr,debug,index_path,pos_xp-1);
            DM_XML_LOG(INFO,corr);
        }
    }
    return DM_OK;
}
#endif
