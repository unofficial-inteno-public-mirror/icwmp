/*
    backupSession.c

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
#include <stdlib.h>
#include <string.h>
#include <expat.h>
#include "soapH.h"
#include "cwmp.h"
#include "list.h"
#include "backupSession.h"

static int backup_session_insert (char *element, char *value, char *parent, char *parent_name, struct attribute *attrs, int level);
static int backup_session_delete (char *element, struct attribute *attrs,int level, char *parent_name);
void backup_session_insert_rpc(char *name, char *commandKey, int status);
void backup_session_delete_rpc(char *name, char *commandKey, int status);
void backup_session_insert_event(char *name, char *commandKey, int id, int rpc_status);
void backup_session_delete_event(char *name,char *commandKey, int id, int rpc_status);
void backup_session_insert_parameter(char *name, char *value, char *parent, char *parent_name, int id, char *commandKey);
void backup_session_insert_acs(char *value);
void backup_session_move_inform_to_inform_send ();
void backup_session_move_inform_to_inform_queue ();
void backup_session_insert_schedule_time(time_t schedule_time,char *commandKey);
void backup_session_delete_schedule_time(time_t schedule_time,char *commandKey);
void backup_session_delete_scheduled_inform();
void backup_session_insert_download(char *commandKey, char *url, time_t id);
void backup_session_delete_download(char *commandKey, char *url, time_t id);
void backup_session_insert_parameter_download(char *name, char *value, char *url, int id, char *commandKey);

struct event_container *cwmp_add_event_container (struct cwmp *cwmp, int event_idx, char *command_key);
struct _cwmp1__TransferComplete *cwmp_set_data_rpc_acs_transferComplete ();

static pthread_mutex_t                      mutex_backup_session;
extern struct list_head                     list_schedule_inform;
extern struct list_head                     list_download;
extern const struct EVENT_CONST_STRUCT      EVENT_CONST [COUNT_EVENT];
extern struct FAULT_CPE                     FAULT_CPE_ARRAY [FAULT_CPE_ARRAY_SIZE];
static struct cwmp                          *save_cwmp;
static struct event_container               *event_container_save = NULL;
static char                                 *acs_url = NULL;
static char                                 value[256];
static char                                 *parent = NULL;
static char                                 *rpc_name = NULL;
static char                                 *rpc_commandkey = NULL;
static char                                 *parameter_name = NULL;
static struct _cwmp1__TransferComplete      *p_soap_cwmp1__TransferComplete;
static enum backup_loading                  init_option;
static struct download          			*download_request = NULL;
extern int									count_download_queue;

void start_bkp_hndl(void *data, const char *el, const char **attr)
{
    char                            *event_name;
    char                            *event_commandKey;
    char                            *event_id;
    int                             event_idx;
    int                             idx;
    int                             i;
    struct schedule_inform          *schedule_inform;
    struct list_head                *ilist;
    char                            *scheduled_commandKey;
    time_t                          scheduled_time;

    if(init_option == ALL)
    {
        if(strcmp(el,"rpc") == 0)
        {
            for(i = 0; attr[i] ; i+=2)
            {
                if(strcmp(attr[i],"name") == 0)
                {
                    rpc_name = strdup(attr[i + 1]);
                }
                if(strcmp(attr[i],"commandKey") == 0)
                {
                    rpc_commandkey = strdup(attr[i + 1]);
                }
            }
            if(strcmp(rpc_name,"TransferComplete") == 0)
            {
                parent = strdup(el);
                p_soap_cwmp1__TransferComplete = cwmp_set_data_rpc_acs_transferComplete ();
            }
        }
        if(strcmp(el,"event") == 0)
        {
            parent = strdup(el);
            for(i = 0; attr[i] ; i+=2)
            {
                if(strcmp(attr[i],"name") == 0)
                {
                    event_name = strdup(attr[i+1]);
                }
                if(strcmp(attr[i],"commandKey") == 0)
                {
                    event_commandKey = strdup(attr[i+1]);
                }
                if(strcmp(attr[i],"id") == 0)
                {
                    event_id = strdup(attr[i+1]);
                }
            }
            for (i = 0; i< COUNT_EVENT; i++)
            {
                if(strcmp(event_name,EVENT_CONST [i].CODE) == 0)
                {
                    event_idx = i;
                    break;
                }
            }
            if ((save_cwmp->env.boot != CWMP_START_BOOT) || (EVENT_CONST[event_idx].RETRY & EVENT_RETRY_AFTER_REBOOT))
            {
                event_container_save = cwmp_add_event_container (save_cwmp, event_idx, event_commandKey);
                if(event_container_save != NULL)
                {
                    event_container_save->id = atoi(event_id);
                }
            }
            if(event_id != NULL)
            {
                free(event_id);
                event_id = NULL;
            }
            if(event_commandKey != NULL)
            {
                free(event_commandKey);
                event_commandKey = NULL;
            }
            if(event_name != NULL)
            {
                free(event_name);
                event_name = NULL;
            }
        }
        if(strcmp(el,"parameter") == 0)
        {
            for(i = 0; attr[i] ; i+=2)
            {
                if(strcmp(attr[i],"name") == 0)
                {
                    parameter_name = strdup(attr[i + 1]);
                }
            }
        }
        if(strcmp(el,"scheduledTime") == 0)
        {
            for(i = 0; attr[i] ; i+=2)
            {
                if(strcmp(attr[i],"commandKey") == 0)
                {
                    scheduled_commandKey = strdup(attr[i + 1]);
                }
                if(strcmp(attr[i],"time") == 0)
                {
                    scheduled_time = atol(attr[i + 1]);
                }
            }
            __list_for_each(ilist,&(list_schedule_inform))
            {
                schedule_inform = list_entry(ilist,struct schedule_inform, list);
                if (schedule_inform->scheduled_time > scheduled_time)
                {
                    break;
                }
            }
            schedule_inform = calloc (1,sizeof(struct schedule_inform));
            if (schedule_inform!=NULL)
            {
                schedule_inform->commandKey     = scheduled_commandKey;
                schedule_inform->scheduled_time = scheduled_time;
                list_add (&(schedule_inform->list), ilist->prev);
            }
        }
        if(strcmp(el,"download") == 0)
        {
            parent = strdup(el);
        	download_request = calloc(1,sizeof(struct download));
            for(i = 0; attr[i] ; i+=2)
            {
                if(strcmp(attr[i],"commandKey") == 0)
                {
                	download_request->CommandKey = strdup(attr[i + 1]);
                }
                if(strcmp(attr[i],"url") == 0)
                {
                	download_request->URL = strdup(attr[i + 1]);
                }
                if(strcmp(attr[i],"time") == 0)
                {
                	download_request->scheduled_time = atol(attr[i + 1]);
                }
            }
        }
    }
}

void end_bkp_hndl(void *data, const char *el)
{
    int                 			i,fault_idx;
    struct session      			*session;
    struct list_head                *ilist;
    struct download					*idownload_request;

    if(init_option == ALL || init_option == ACS)
    {
        if(strcmp(el,"url") == 0)
        {
            acs_url = strdup(value);
        }
    }
    if(init_option == ALL)
    {
        if(strcmp(el,"rpc") == 0)
        {
            if(strcmp(rpc_name,"TransferComplete") == 0)
            {
                if(rpc_commandkey != NULL)
                {
                    p_soap_cwmp1__TransferComplete->CommandKey = strdup(rpc_commandkey);
                }
                else
                {
                    p_soap_cwmp1__TransferComplete->CommandKey = strdup("");
                }
                cwmp_root_cause_TransferComplete (save_cwmp);
            }
            if(parent != NULL)
            {
                free(parent);
                parent = NULL;
            }
            if(rpc_name != NULL)
            {
                free(rpc_name);
                rpc_name = NULL;
            }
        }
        if(strcmp(el,"event") == 0)
        {
            if(parent != NULL)
            {
                free(parent);
                parent = NULL;
            }
        }
        if(strcmp(el,"download") == 0)
		{
			__list_for_each(ilist,&(list_download))
			{
				idownload_request = list_entry(ilist,struct download,list);
				if (idownload_request->scheduled_time > download_request->scheduled_time)
				{
					break;
				}
			}
			list_add (&(download_request->list), ilist->prev);
			count_download_queue++;
            if(parent != NULL)
            {
                free(parent);
                parent = NULL;
            }
		}
        if(strcmp(el,"parameter") == 0)
        {
            if(parent != NULL && strcmp(parent,"rpc") == 0)
            {
                if(strcmp(rpc_name,"TransferComplete") == 0)
                {
                    if(p_soap_cwmp1__TransferComplete != NULL)
                    {
                        if(strcmp(parameter_name,"StartTime") == 0)
                        {
                            p_soap_cwmp1__TransferComplete->StartTime = atol(value);
                        }
                        if(strcmp(parameter_name,"CompleteTime") == 0)
                        {
                            p_soap_cwmp1__TransferComplete->CompleteTime = atol(value);
                        }
                        if(strcmp(parameter_name,"FaultCode") == 0)
                        {
                            p_soap_cwmp1__TransferComplete->FaultStruct = calloc(1,sizeof(struct cwmp1__FaultStruct));
                            p_soap_cwmp1__TransferComplete->FaultStruct->FaultCode = strdup(value);
                            for(i = 0; i < FAULT_CPE_ARRAY_SIZE; i++)
                            {
                                if((FAULT_CPE_ARRAY[i].CODE != NULL)&&(strcmp(FAULT_CPE_ARRAY[i].CODE, value) == 0))
                                {
                                    fault_idx = i;
                                    break;
                                }
                            }
                            p_soap_cwmp1__TransferComplete->FaultStruct->FaultString = strdup(FAULT_CPE_ARRAY[i].DESCRIPTION);
                        }
                    }
                }
            }
            if(parent != NULL && strcmp(parent,"download") == 0)
			{
				if(download_request != NULL)
				{
					if(strcmp(parameter_name,"FileType") == 0)
					{
						download_request->FileType = strdup(value);
					}
					if(strcmp(parameter_name,"Username") == 0)
					{
						download_request->Username = strdup(value);
					}
					if(strcmp(parameter_name,"Password") == 0)
					{
						download_request->Password = strdup(value);
					}
				}
            }
            if(parent != NULL && strcmp(parent,"event") == 0)
            {
                if(event_container_save != NULL)
                {
                    cwmp_add_parameter_container (save_cwmp,event_container_save, parameter_name);
                }
            }
            if(parameter_name != NULL)
            {
                free(parameter_name);
                parameter_name = NULL;
            }
        }
    }
}

void char_bkp_hndl(void *data, const char *content,int length)
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
            strcpy(value,tmp);
        }
        else
        {
            value[0] = 0;
        }
    }
}

int cwmp_load_saved_session(struct cwmp *cwmp, char **acsurl, enum backup_loading load)
{
    int     max,done = 0;
    char    *buff;
    FILE    *pFile;

    save_cwmp = cwmp;
    init_option = load;

    pFile = fopen(CWMP_BKP_FILE,"r+");

    if(pFile == NULL)
    {
        pFile = fopen(CWMP_BKP_FILE,"w");
        if(pFile == NULL)
        {
            CWMP_LOG(ERROR,"Unable to create %s file",CWMP_BKP_FILE);
            return CWMP_MEM_ERR;
        }
        fclose(pFile);
        backup_session_insert("cwmp",NULL,NULL,NULL,NULL,1);
        backup_session_insert("acs",NULL,"cwmp",NULL,NULL,2);
        return CWMP_OK;
    }
    fclose(pFile);

    backup_session_move_inform_to_inform_queue ();

    pFile = fopen(CWMP_BKP_FILE,"r+");
    XML_Parser parser = XML_ParserCreate(NULL);
    if (! parser)
    {
        CWMP_LOG(ERROR,"Couldn't allocate memory for parser");
        return CWMP_MEM_ERR;
    }

    XML_UseParserAsHandlerArg(parser);
    XML_SetElementHandler(parser, start_bkp_hndl, end_bkp_hndl);
    XML_SetCharacterDataHandler(parser, char_bkp_hndl);

    max = get_xml_file_size(pFile);

    buff = calloc(1,max);
    if(buff == NULL)
    {
        return CWMP_MEM_ERR;
    }

    for (fread(buff, sizeof(char), max, pFile); !feof(pFile); fread(buff, sizeof(char), max, pFile))
    {
        if (! XML_Parse(parser, buff, max, done))
        {
            CWMP_LOG(ERROR,"%s Parse error at line %d: %s",CWMP_BKP_FILE,(int)XML_GetCurrentLineNumber(parser),XML_ErrorString(XML_GetErrorCode(parser)));
            return CWMP_ORF_ERR;
        }
    }
    if(buff != NULL)
    {
        free(buff);
    }
    XML_ParserFree(parser);
    fclose(pFile);

    if(acsurl != NULL)
    {
        if(acs_url != NULL)
        {
            *acsurl = acs_url;
        }
        else
        {
            *acsurl = NULL;
        }
    }

    return CWMP_OK;
}

static int backup_session_insert (char *element, char *value, char *parent, char *parent_name, struct attribute *attrs, int level)
{
    char        line[256];
    char        start[256];
    char        end[256];
    char        indent[256];
    char        attrib[256];
    char        parent_tag[256];
    int         i;
    FILE        *pFile;
    FILE        *tmpFile;

    pthread_mutex_lock (&mutex_backup_session);
    attrib[0] = 0;
    start[0] = 0;
    end[0] = 0;
    indent[0] = 0;
    parent_tag[0] = 0;

    pFile = fopen(CWMP_BKP_FILE,"r");
    tmpFile = fopen("tmp.xml","w");
    if(pFile == NULL || tmpFile == NULL)
    {
        CWMP_LOG(ERROR,"Unable to write in %s file",CWMP_BKP_FILE);
        pthread_mutex_unlock (&mutex_backup_session);
        exit(EXIT_FAILURE);
    }

    for(i=0 ; i<level-1 ; i++)
    {
        indent[i] = '\t';
    }
    indent[level-1] = 0;
    if(attrs != NULL)
    {
        if(strcmp(element,"rpc") == 0)
        {
        	if(attrs->status != NULL)
            {
                sprintf(attrib,"name=\"%s\" status=\"%s\"", attrs->name, attrs->status);
            }
            else
            {
                if(attrs->commandKey != NULL)
                {
                    sprintf(attrib,"name=\"%s\" commandKey=\"%s\"", attrs->name, attrs->commandKey);
                }
                else
                {
                    sprintf(attrib,"name=\"%s\"", attrs->name);
                }
            }
        }
        else if (strcmp(element,"event") == 0)
        {
            sprintf(attrib,"name=\"%s\" id=\"%li\" commandKey=\"%s\"", attrs->name, attrs->id, attrs->commandKey);
        }
        else if  (strcmp(element,"parameter") == 0)
        {
            sprintf(attrib,"name=\"%s\"", attrs->name);
        }
        else if  (strcmp(element,"scheduledTime") == 0)
        {
            sprintf(attrib,"commandKey=\"%s\" time=\"%li\"", attrs->commandKey, attrs->id);
        }
        else if  (strcmp(element,"download") == 0)
        {
            sprintf(attrib,"commandKey=\"%s\" url=\"%s\" time=\"%li\"", attrs->commandKey, attrs->url, attrs->id);
        }
    }

    if(strlen(indent) != 0)
    {
        if(value != NULL)
        {
            if(strlen(attrib) != 0)
            {
                sprintf(start,"%s<%s %s>%s",indent,element,attrib,value);
            }
            else
            {
                sprintf(start,"%s<%s>%s",indent,element,value);
            }
            sprintf(end,"</%s>\n",element);
        }
        else
        {
            if(strlen(attrib) != 0)
            {
                sprintf(start,"%s<%s %s>\n",indent,element,attrib);
            }
            else
            {
                sprintf(start,"%s<%s>\n",indent,element);
            }
            sprintf(end,"%s</%s>\n",indent,element);
        }
    }
    else
    {
        if(strlen(attrib) != 0)
        {
            sprintf(start,"<%s %s>\n",element,attrib);
        }
        else
        {
            sprintf(start,"<%s>\n",element);
        }
        sprintf(end,"</%s>",element);
    }

    if(parent != NULL)
    {
        if(parent_name != NULL)
        {
            if(strcmp(parent,"event") == 0)
            {
                sprintf(parent_tag,"<%s name=\"%s\" id=\"%li\" commandKey=\"%s\"", parent, parent_name, attrs->id, attrs->commandKey);
            }
            else if(strcmp(parent,"rpc") == 0)
            {
                if(attrs->commandKey != NULL)
            	{
                	sprintf(parent_tag,"<%s name=\"%s\" commandKey=\"%s\"", parent, parent_name, attrs->commandKey);
            	}
                else
                {
                	sprintf(parent_tag,"<%s name=\"%s\"", parent, parent_name);
                }
            }
            else
            {
                sprintf(parent_tag,"<%s name=\"%s\"", parent, parent_name);
            }
        }
        else
        {
            if(strcmp(parent,"download") == 0)
            {
                if(attrs->commandKey != NULL)
            	{
                	sprintf(parent_tag,"<%s commandKey=\"%s\" url=\"%s\" time=\"%li\"", parent, attrs->commandKey, attrs->url, attrs->id);
            	}
                else
                {
                	sprintf(parent_tag,"<%s commandKey=\"\" url=\"%s\" time=\"%li\"", parent, attrs->url, attrs->id);
                }
            }
            else
			{
            	sprintf(parent_tag,"<%s", parent);
			}
        }
    }
    /** search element if exist **/
    while(!feof(pFile))
    {
        fgets(line,sizeof(line),pFile);
        if ((strcmp(element,"parameter") != 0)   &&
            (strstr(line,start) != NULL))
        {
            fclose(pFile);
            fclose(tmpFile);
            remove("tmp.xml");
            pthread_mutex_unlock (&mutex_backup_session);
            return CWMP_OK;
        }
        if ((strcmp(element,"parameter") == 0)               &&
            (parent != NULL) && (strcmp(parent,"event") == 0)&&
            (strstr(line,parent_tag) != NULL))
        {
            while(strstr(line,"</event>") == NULL)
            {
                fgets(line,sizeof(line),pFile);
                if (strstr(line,start) != NULL)
                {
                    fclose(pFile);
                    fclose(tmpFile);
                    remove("tmp.xml");
                    pthread_mutex_unlock (&mutex_backup_session);
                    return CWMP_OK;
                }
            }
        }
        if ((strcmp(element,"parameter") == 0)               &&
            (parent != NULL) && (strcmp(parent,"rpc") == 0)  &&
            (strstr(line,parent_tag) != NULL))
        {
            while(strstr(line,"</rpc>") == NULL)
            {
                fgets(line,sizeof(line),pFile);
                if (strstr(line,start) != NULL)
                {
                    fclose(pFile);
                    fclose(tmpFile);
                    remove("tmp.xml");
                    pthread_mutex_unlock (&mutex_backup_session);
                    return CWMP_OK;
                }
            }
        }
        if ((strcmp(element,"parameter") == 0)               &&
            (parent != NULL) && (strcmp(parent,"download") == 0)  &&
            (strstr(line,parent_tag) != NULL))
        {
            while(strstr(line,"</download>") == NULL)
            {
                fgets(line,sizeof(line),pFile);
                if (strstr(line,start) != NULL)
                {
                    fclose(pFile);
                    fclose(tmpFile);
                    remove("tmp.xml");
                    pthread_mutex_unlock (&mutex_backup_session);
                    return CWMP_OK;
                }
            }
        }
    }

    if((feof(pFile))&&(strcmp(element,"cwmp") == 0))
    {
        fputs(start,tmpFile);
        fputs(end,tmpFile);
        goto end;
    }
    rewind(pFile);
    while(!feof(pFile))
    {
        fgets(line,sizeof(line),pFile);
        fputs(line,tmpFile);
        if(strstr(line,parent_tag) != NULL)
        {
            if((strcmp(element,"event") == 0)&&(strstr(line,attrs->status) == NULL))
            {
                continue;
            }
            break;
        }
    }

    fputs(start,tmpFile);
    fputs(end,tmpFile);
    while(!feof(pFile))
    {
        fgets(line,sizeof(line),pFile);
        fputs(line,tmpFile);
    }
end:
    fclose(pFile);
    fclose(tmpFile);
    rename("tmp.xml", CWMP_BKP_FILE);
    pthread_mutex_unlock (&mutex_backup_session);
    return CWMP_OK;
}

static int backup_session_delete (char *element, struct attribute *attrs,int level, char *parent_name)
{
    char        line[256];
    char        start[256];
    char        end[256];
    char        attrib[256];
    char        id[16];
    int         i;
    FILE        *pFile;
    FILE        *tmpFile;
    bool        found = FALSE;

    pthread_mutex_lock (&mutex_backup_session);
    attrib[0] = 0;
    start[0] = 0;
    end[0] = 0;

    pFile = fopen(CWMP_BKP_FILE,"r");
    tmpFile = fopen("tmp.xml","w");
    if(pFile == NULL || tmpFile == NULL)
    {
        CWMP_LOG(ERROR,"Unable to write in %s file",CWMP_BKP_FILE);
        pthread_mutex_unlock (&mutex_backup_session);
        exit(EXIT_FAILURE);
    }

    if(attrs != NULL)
    {
        if(strcmp(element,"rpc") == 0)
        {
        	if(attrs->status != NULL)
            {
                sprintf(attrib,"name=\"%s\" status=\"%s\"", attrs->name, attrs->status);
            }
            else
            {
                if(attrs->commandKey != NULL)
                {
                    sprintf(attrib,"name=\"%s\" commandKey=\"%s\"", attrs->name, attrs->commandKey);
                }
                else
                {
                    sprintf(attrib,"name=\"%s\"", attrs->name);
                }
            }
        }
        else if (strcmp(element,"event") == 0)
        {
            sprintf(attrib,"name=\"%s\" id=\"%li\" commandKey=\"%s\"", attrs->name, attrs->id, attrs->commandKey);
        }
        else if  (strcmp(element,"parameter") == 0)
        {
            sprintf(attrib,"name=\"%s\"", attrs->name);
            sprintf(id,"%li",attrs->id);
        }
        else if  (strcmp(element,"scheduledTime") == 0)
        {
            sprintf(attrib,"commandKey=\"%s\" time=\"%li\"", attrs->commandKey, attrs->id);
        }
        else if  (strcmp(element,"download") == 0)
        {
            sprintf(attrib,"commandKey=\"%s\" url=\"%s\" time=\"%li\"", attrs->commandKey, attrs->url, attrs->id);
        }
    }

    if(strlen(attrib) != 0)
    {
        sprintf(start,"<%s %s>",element,attrib);
    }
    else
    {
        sprintf(start,"<%s>",element);
    }
    sprintf(end,"</%s>\n",element);

    /** search element if exist **/

    while(!feof(pFile))
    {
        fgets(line,sizeof(line),pFile);
        if (strstr(line,start) != NULL)
        {
            found = TRUE;
            break;
        }
    }
    if((found == FALSE) && (feof(pFile)))
    {
        fclose(pFile);
        fclose(tmpFile);
        remove("tmp.xml");
        pthread_mutex_unlock (&mutex_backup_session);
        return CWMP_OK;
    }
    else
    {
        found = FALSE;
        rewind(pFile);
    }

    while(!feof(pFile))
    {
        fgets(line,sizeof(line),pFile);
        if (
            (strcmp(element,"parameter") == 0)                                                  &&
            (strstr(line,parent_name) != NULL)                                                  &&
            ((attrs != NULL)&&((attrs->id == -1)||((attrs->id != -1) && (strstr(line,id) != NULL))))
           )
        {
                found = TRUE;
        }
        if((strcmp(element,"parameter") != 0)||((strcmp(element,"parameter") == 0)&&(found == TRUE)))
        {
            if((strstr(line,start) != NULL)&&(strstr(line,end) == NULL))
            {
                break;
            }
            else if((strstr(line,start) != NULL)&&(strstr(line,end) != NULL))
            {
                goto end;
            }
        }
        fputs(line,tmpFile);
    }

    if(feof(pFile))
    {
        pthread_mutex_unlock (&mutex_backup_session);
        return CWMP_OK;
    }

    while(!feof(pFile)&&(strstr(line,end) == NULL))
    {
        fgets(line,sizeof(line),pFile);
    }
end:
    while(!feof(pFile))
    {
        fgets(line,sizeof(line),pFile);
        fputs(line,tmpFile);
    }
    fclose(pFile);
    fclose(tmpFile);
    rename("tmp.xml", CWMP_BKP_FILE);
    pthread_mutex_unlock (&mutex_backup_session);
    return CWMP_OK;
}

void backup_session_insert_event(char *name, char *commandKey, int id, int rpc_status)
{
    struct attribute *attrs;

    attrs = calloc(1,sizeof(struct attribute));
    if(attrs != NULL)
    {
        attrs->id = id;
        attrs->name = strdup(name);
        if(rpc_status == RPC_QUEUE)
        {
            attrs->status = strdup("queue");
        }
        else if(rpc_status == RPC_SEND)
        {
            attrs->status = strdup("send");
        }
        if(commandKey != NULL)
        {
            attrs->commandKey = strdup(commandKey);
        }
        else
        {
            attrs->commandKey = NULL;
        }
    }
    backup_session_insert("event",NULL,"rpc",NULL,attrs,3);
    if(attrs->name != NULL)
    {
        free(attrs->name);
    }
    if(attrs->status != NULL)
    {
        free(attrs->status);
    }
    if(attrs->commandKey != NULL)
    {
        free(attrs->commandKey);
    }
    free(attrs);
}

void backup_session_insert_parameter(char *name, char *value, char *parent,char *parent_name, int id, char *commandKey)
{
    struct attribute *attrs;
    int level;
    attrs = calloc(1,sizeof(struct attribute));
    if(attrs != NULL)
    {
        attrs->id = id;
        attrs->name = strdup(name);
        if(commandKey != NULL)
        {
        	attrs->commandKey = strdup(commandKey);
        }
    }
    if(strcmp(parent,"rpc") == 0)
    {
        level = 3;
    }
    else if(strcmp(parent,"event") == 0)
    {
        level = 4;
    }
    backup_session_insert("parameter",value,parent,parent_name,attrs,level);
    if(attrs->name != NULL)
    {
        free(attrs->name);
    }
    if(attrs->commandKey != NULL)
    {
        free(attrs->commandKey);
    }
    free(attrs);
}

void backup_session_insert_rpc(char *name, char *commandKey, int status)
{
    struct attribute *attrs;

    attrs = calloc(1,sizeof(struct attribute));
    if(attrs != NULL)
    {
        attrs->id = -1;
        attrs->name = strdup(name);
        if(status == RPC_QUEUE)
        {
            attrs->status = strdup("queue");
        }
        else if(status == RPC_SEND)
        {
            attrs->status = strdup("send");
        }
        else
        {
            attrs->status = NULL;
        }
        if(commandKey != NULL)
        {
            attrs->commandKey = strdup(commandKey);
        }
        else
        {
            attrs->commandKey = NULL;
        }
    }
    backup_session_insert("rpc",NULL,"cwmp",NULL,attrs,2);
    if(attrs->name != NULL)
    {
        free(attrs->name);
    }
    if(attrs->status != NULL)
    {
        free(attrs->status);
    }
    if(attrs->commandKey != NULL)
    {
        free(attrs->commandKey);
    }
    free(attrs);
}

void backup_session_insert_acs(char *value)
{
    backup_session_delete ("url", NULL, 3, NULL);
    backup_session_insert("url", value, "acs", NULL, NULL, 3);
}

void backup_session_delete_event(char *name,char *commandKey, int id, int rpc_status)
{
    struct attribute *attrs;

    attrs = calloc(1,sizeof(struct attribute));
    if(attrs != NULL)
    {
        attrs->id = id;
        attrs->name = strdup(name);
        if(rpc_status == RPC_QUEUE)
        {
            attrs->status = strdup("queue");
        }
        else if(rpc_status == RPC_SEND)
        {
            attrs->status = strdup("send");
        }
        if(commandKey != NULL)
        {
            attrs->commandKey = strdup(commandKey);
        }
        else
        {
            attrs->commandKey = NULL;
        }
    }
    backup_session_delete ("event", attrs, 3, NULL);
    if(attrs->name != NULL)
    {
        free(attrs->name);
    }
    if(attrs->status != NULL)
    {
        free(attrs->status);
    }
    if(attrs->commandKey != NULL)
    {
        free(attrs->commandKey);
    }
    free(attrs);
}

void backup_session_delete_rpc(char *name, char *commandKey, int status)
{
    struct attribute *attrs;

    attrs = calloc(1,sizeof(struct attribute));
    if(attrs != NULL)
    {
        attrs->id = -1;
        attrs->name = strdup(name);
        if(status == RPC_QUEUE)
        {
            attrs->status = strdup("queue");
        }
        else if(status == RPC_SEND)
        {
            attrs->status = strdup("send");
        }
        else
        {
            attrs->status = NULL;
        }
        if(commandKey != NULL)
        {
            attrs->commandKey = strdup(commandKey);
        }
        else
        {
            attrs->commandKey = NULL;
        }
    }
    backup_session_delete("rpc",attrs,2, NULL);
    if(attrs->name != NULL)
    {
        free(attrs->name);
    }
    if(attrs->status != NULL)
    {
        free(attrs->status);
    }
    if(attrs->commandKey != NULL)
    {
        free(attrs->commandKey);
    }
    free(attrs);
}

void backup_session_move_inform_to_inform_send ()
{
    char        line[256];
    char        start[256];
    FILE        *pFile;
    FILE        *tmpFile;

    pthread_mutex_lock (&mutex_backup_session);
    start[0] = 0;

    pFile = fopen(CWMP_BKP_FILE,"r");
    tmpFile = fopen("tmp.xml","w");
    if(pFile == NULL || tmpFile == NULL)
    {
        CWMP_LOG(ERROR,"Unable to write in %s file",CWMP_BKP_FILE);
        pthread_mutex_unlock (&mutex_backup_session);
        exit(EXIT_FAILURE);
    }

    sprintf(start,"\t<rpc name=\"Inform\" status=\"send\">\n");

    while(!feof(pFile))
    {
        fgets(line,sizeof(line),pFile);
        if(strcmp(line,"\t<rpc name=\"Inform\" status=\"queue\">\n") == 0)
        {
            break;
        }
        fputs(line,tmpFile);
    }
    if(!feof(pFile))
    {
        fputs(start,tmpFile);
    }
    while(!feof(pFile))
    {
        fgets(line,sizeof(line),pFile);
        fputs(line,tmpFile);
    }
    fclose(pFile);
    fclose(tmpFile);
    rename("tmp.xml", CWMP_BKP_FILE);
    pthread_mutex_unlock (&mutex_backup_session);
}

void backup_session_move_inform_to_inform_queue ()
{
    char        line[256];
    char        *buffer;
    FILE        *pFile;
    FILE        *tmpFile;
    long int    max;

    pthread_mutex_lock (&mutex_backup_session);


    pFile = fopen(CWMP_BKP_FILE,"r");

    max = get_xml_file_size(pFile);
    buffer = calloc(1,max);

    tmpFile = fopen("tmp.xml","w");
    if(pFile == NULL || tmpFile == NULL)
    {
        CWMP_LOG(ERROR,"Unable to write in %s file",CWMP_BKP_FILE);
        exit(EXIT_FAILURE);
    }

    while(!feof(pFile))
    {
        fgets(line,sizeof(line),pFile);
        if(strcmp(line,"\t<rpc name=\"Inform\" status=\"send\">\n") == 0)
        {
            break;
        }
        fputs(line,tmpFile);
    }
    while(!feof(pFile))
    {
        fgets(line,sizeof(line),pFile);
        if(strcmp(line,"\t</rpc>\n") != 0)
        {
            strcat (buffer,line);
        }
        else
        {
            break;
        }
    }
    while(!feof(pFile))
    {
        fgets(line,sizeof(line),pFile);
        fputs(line,tmpFile);
    }
    fclose(pFile);
    fclose(tmpFile);
    rename("tmp.xml", CWMP_BKP_FILE);
    pthread_mutex_unlock (&mutex_backup_session);

    if(strlen(buffer) != 0)
    {
        backup_session_insert_rpc("Inform",NULL,RPC_QUEUE);
    }
    else
    {
        return;
    }

    pthread_mutex_lock (&mutex_backup_session);
    pFile = fopen(CWMP_BKP_FILE,"r");
    tmpFile = fopen("tmp.xml","w");
    if(pFile == NULL || tmpFile == NULL)
    {
        CWMP_LOG(ERROR,"Unable to write in %s file",CWMP_BKP_FILE);
        pthread_mutex_unlock (&mutex_backup_session);
        exit(EXIT_FAILURE);
    }

    while(!feof(pFile))
    {
        fgets(line,sizeof(line),pFile);
        fputs(line,tmpFile);
        if(strcmp(line,"\t<rpc name=\"Inform\" status=\"queue\">\n") == 0)
        {
            break;
        }
    }
    if(!feof(pFile))
    {
        fputs(buffer,tmpFile);
    }
    while(!feof(pFile))
    {
        fgets(line,sizeof(line),pFile);
        fputs(line,tmpFile);
    }
    fclose(pFile);
    fclose(tmpFile);
    rename("tmp.xml", CWMP_BKP_FILE);
    if(buffer != NULL)
    {
        free(buffer);
    }
    pthread_mutex_unlock (&mutex_backup_session);
}

void backup_session_insert_schedule_time(time_t schedule_time,char *commandKey)
{
    struct attribute    *attrs;

    attrs = calloc(1,sizeof(struct attribute));
    if(attrs != NULL)
    {
        attrs->id = schedule_time;
        if(commandKey != NULL)
        {
            attrs->commandKey = strdup(commandKey);
        }
        else
        {
            attrs->commandKey = NULL;
        }
    }
    backup_session_insert("scheduledInform",NULL,"cwmp",NULL,NULL,2);
    backup_session_insert("scheduledTime", NULL, "scheduledInform", NULL, attrs, 3);
    if(attrs->commandKey != NULL)
    {
        free(attrs->commandKey);
    }
    free(attrs);
}

void backup_session_delete_schedule_time(time_t schedule_time,char *commandKey)
{
    struct attribute    *attrs;

    attrs = calloc(1,sizeof(struct attribute));
    if(attrs != NULL)
    {
        attrs->id = schedule_time;
        if(commandKey != NULL)
        {
            attrs->commandKey = strdup(commandKey);
        }
        else
        {
            attrs->commandKey = NULL;
        }
    }
    backup_session_delete("scheduledTime", attrs, 3, NULL);
    if(attrs->commandKey != NULL)
    {
        free(attrs->commandKey);
    }
    free(attrs);
}

void backup_session_delete_scheduled_inform()
{
    backup_session_delete("scheduledInform",NULL,2,NULL);
}

void backup_session_insert_download(char *commandKey, char *url, time_t id)
{
    struct attribute *attrs;

    attrs = calloc(1,sizeof(struct attribute));
    if(attrs != NULL)
    {
        attrs->id = id;
        if(url != NULL)
        {
            attrs->url = strdup(url);
        }
        else
        {
            attrs->url = NULL;
        }
        if(commandKey != NULL)
        {
            attrs->commandKey = strdup(commandKey);
        }
        else
        {
            attrs->commandKey = NULL;
        }
    }
    backup_session_insert("download",NULL,"cwmp",NULL,attrs,2);
    if(attrs->url != NULL)
    {
        free(attrs->url);
    }
    if(attrs->commandKey != NULL)
    {
        free(attrs->commandKey);
    }
    free(attrs);
}

void backup_session_delete_download(char *commandKey, char *url, time_t id)
{
    struct attribute *attrs;

    attrs = calloc(1,sizeof(struct attribute));
    if(attrs != NULL)
    {
        attrs->id = id;
        if(url != NULL)
        {
            attrs->url = strdup(url);
        }
        else
        {
            attrs->url = NULL;
        }
        if(commandKey != NULL)
        {
            attrs->commandKey = strdup(commandKey);
        }
        else
        {
            attrs->commandKey = NULL;
        }
    }
    backup_session_delete("download",attrs,2, NULL);
    if(attrs->url != NULL)
    {
        free(attrs->url);
    }
    if(attrs->commandKey != NULL)
    {
        free(attrs->commandKey);
    }
    free(attrs);
}

void backup_session_insert_parameter_download(char *name, char *value, char *url, int id, char *commandKey)
{
    struct attribute *attrs;

    attrs = calloc(1,sizeof(struct attribute));
    if(attrs != NULL)
    {
        attrs->id = id;
        attrs->name = strdup(name);
        if(commandKey != NULL)
        {
        	attrs->commandKey = strdup(commandKey);
        }
        if(url != NULL)
        {
        	attrs->url = strdup(url);
        }
    }
    backup_session_insert("parameter",value,"download",NULL,attrs,3);
    if(attrs->name != NULL)
    {
        free(attrs->name);
    }
    if(attrs->commandKey != NULL)
    {
        free(attrs->commandKey);
    }
    if(attrs->url != NULL)
    {
        free(attrs->url);
    }
    free(attrs);
}
