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
#include <stdbool.h>
#include <microxml.h>
#include <time.h>
#include "cwmp.h"
#include "backupSession.h"

# define CWMP_BACKUP_SESSION "<cwmp><acs><url/></acs></cwmp>"

pthread_mutex_t                      		mutex_backup_session;
extern struct list_head                     list_schedule_inform;
extern struct list_head                     list_download;
extern const struct EVENT_CONST_STRUCT      EVENT_CONST [COUNT_EVENT];
extern struct FAULT_CPE                     FAULT_CPE_ARRAY [FAULT_CPE_ARRAY_SIZE];
extern int									count_download_queue;

int cwmp_load_saved_session(struct cwmp *cwmp, char **acsurl, enum backup_loading load);
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
void backup_session_insert_download(char *commandKey, char *url, time_t schedule_time);
void backup_session_delete_download(char *commandKey, char *url, time_t schedule_time);
void backup_session_insert_parameter_download(char *name, char *value, char *url, time_t schedule_time, char *commandKey);



void backup_session_insert_rpc(char *name, char *commandKey, int status)
{
	FILE *fp;
	mxml_node_t *tree, *b;
	char *bkp_content;

	pthread_mutex_lock (&mutex_backup_session);
	fp = fopen(CWMP_BKP_FILE, "r");
	tree = mxmlLoadFile(NULL, fp, MXML_NO_CALLBACK);
	fclose(fp);

	if(strcmp(name,"Inform") == 0)
	{
		if(status == RPC_QUEUE)
		{
			b = mxmlFindElement(tree, tree, "rpc", "status", "queue", MXML_DESCEND);
		}
		if(status == RPC_SEND)
		{
			b = mxmlFindElement(tree, tree, "rpc", "status", "send", MXML_DESCEND);
		}
		if(b)
		{
			goto end;
		}
	}
	b = mxmlFindElement(tree, tree, "cwmp", NULL, NULL, MXML_DESCEND);

	b = mxmlNewElement(tree, "rpc");

	mxmlElementSetAttr(b, "name", name);
	if(status == RPC_QUEUE)
	{
		mxmlElementSetAttr(b, "status", "queue");
	}
	else if(status == RPC_SEND)
	{
		mxmlElementSetAttr(b, "status", "send");
	}

	if(commandKey != NULL)
	{
		mxmlElementSetAttr(b, "commandKey", commandKey);
	}
	bkp_content = mxmlSaveAllocString(tree, MXML_NO_CALLBACK);
	fp = fopen(CWMP_BKP_FILE, "w");
	fprintf(fp,"%s",bkp_content);
	fclose(fp);
end:
	mxmlDelete(tree);
	pthread_mutex_unlock (&mutex_backup_session);
}

void backup_session_delete_rpc(char *name, char *commandKey, int status)
{
	FILE *fp;
	mxml_node_t *tree, *b;
	char *bkp_content;

	pthread_mutex_lock (&mutex_backup_session);
	fp = fopen(CWMP_BKP_FILE, "r");
	tree = mxmlLoadFile(NULL, fp, MXML_NO_CALLBACK);
	fclose(fp);

	if(status == RPC_QUEUE)
	{
		b = mxmlFindElement(tree, tree, "rpc", "status", "queue", MXML_DESCEND);
	}
	else if(status == RPC_SEND)
	{
		b = mxmlFindElement(tree, tree, "rpc", "status", "send", MXML_DESCEND);
	}
	else
	{
		if(commandKey != NULL)
		{
			b = mxmlFindElement(tree, tree, "rpc", "commandKey", commandKey, MXML_DESCEND);
		}
	}
	if(b)
	{
		mxmlRemove(b);
	}
	bkp_content = mxmlSaveAllocString(tree, MXML_NO_CALLBACK);
	fp = fopen(CWMP_BKP_FILE, "w");
	fprintf(fp,"%s",bkp_content);
	fclose(fp);
	mxmlDelete(tree);
	pthread_mutex_unlock (&mutex_backup_session);
}

void backup_session_insert_event(char *name, char *commandKey, int id, int rpc_status)
{
	FILE *fp;
	mxml_node_t *tree, *b, *c;
	char tmp[4];
	char *bkp_content;

	pthread_mutex_lock (&mutex_backup_session);
	fp = fopen(CWMP_BKP_FILE, "r");
	tree = mxmlLoadFile(NULL, fp, MXML_NO_CALLBACK);
	fclose(fp);

	if(rpc_status == RPC_QUEUE)
	{
		b = mxmlFindElement(tree, tree, "rpc", "status", "queue", MXML_DESCEND);
	}
	else if(rpc_status == RPC_SEND)
	{
		b = mxmlFindElement(tree, tree, "rpc", "status", "send", MXML_DESCEND);
	}

	sprintf(tmp,"%d",id);
	c = mxmlFindElement(b, b, "event", "id", tmp, MXML_DESCEND);
	if(c)
	{
		goto end;
	}

	b = mxmlNewElement(b, "event");

	mxmlElementSetAttr(b, "name", name);

	mxmlElementSetAttr(b, "id", tmp);

	if(commandKey != NULL)
	{
		mxmlElementSetAttr(b, "commandKey", commandKey);
	}
	bkp_content = mxmlSaveAllocString(tree, MXML_NO_CALLBACK);
	fp = fopen(CWMP_BKP_FILE, "w");
	fprintf(fp,"%s",bkp_content);
	fclose(fp);
end:
	mxmlDelete(tree);
	pthread_mutex_unlock (&mutex_backup_session);
}

void backup_session_delete_event(char *name,char *commandKey, int id, int rpc_status)
{
	FILE *fp;
	mxml_node_t *tree, *b;
	char tmp[4];
	char *bkp_content;

	pthread_mutex_lock (&mutex_backup_session);
	fp = fopen(CWMP_BKP_FILE, "r");
	tree = mxmlLoadFile(NULL, fp, MXML_NO_CALLBACK);
	fclose(fp);

	if(rpc_status == RPC_QUEUE)
	{
		b = mxmlFindElement(tree, tree, "rpc", "status", "queue", MXML_DESCEND);
	}
	else if(rpc_status == RPC_SEND)
	{
		b = mxmlFindElement(tree, tree, "rpc", "status", "send", MXML_DESCEND);
	}
	if(b)
	{
		sprintf(tmp,"%d",id);
		b = mxmlFindElement(b, b, "event", "id", tmp, MXML_DESCEND);
		if(b)
		{
			mxmlRemove(b);
		}
	}
	bkp_content = mxmlSaveAllocString(tree, MXML_NO_CALLBACK);
	fp = fopen(CWMP_BKP_FILE, "w");
	fprintf(fp,"%s",bkp_content);
	fclose(fp);
	mxmlDelete(tree);
	pthread_mutex_unlock (&mutex_backup_session);
}

void backup_session_insert_parameter(char *name, char *value, char *parent, char *parent_name, int id, char *commandKey)
{
	FILE *fp;
	mxml_node_t *tree, *b, *c;
	char tmp[4];
	char *bkp_content;

	pthread_mutex_lock (&mutex_backup_session);
	fp = fopen(CWMP_BKP_FILE, "r");
	tree = mxmlLoadFile(NULL, fp, MXML_NO_CALLBACK);
	fclose(fp);

	if(id==-1)
	{
		b = mxmlFindElement(tree, tree, parent, "name", parent_name, MXML_DESCEND);
	}
	else
	{
		sprintf(tmp,"%d",id);
		b = mxmlFindElement(tree, tree, parent, "id", tmp, MXML_DESCEND);
	}
	if(b)
	{
		c = mxmlFindElement(b, b, "parameter", "name", name, MXML_DESCEND);
		if(c)
		{
			goto end;
		}
		b = mxmlNewElement(b, "parameter");

		mxmlElementSetAttr(b, "name", name);

		if(value != NULL)
		{
			b = mxmlNewText(b, 0, value);
		}
		bkp_content = mxmlSaveAllocString(tree, MXML_NO_CALLBACK);
		fp = fopen(CWMP_BKP_FILE, "w");
		fprintf(fp,"%s",bkp_content);
		fclose(fp);
	}
end:
	mxmlDelete(tree);
	pthread_mutex_unlock (&mutex_backup_session);
}

void backup_session_insert_acs(char *value)
{
	FILE *fp;
	mxml_node_t *tree, *b;
	char *bkp_content;

	pthread_mutex_lock (&mutex_backup_session);
	fp = fopen(CWMP_BKP_FILE, "r");
	tree = mxmlLoadFile(NULL, fp, MXML_NO_CALLBACK);
	fclose(fp);

	b = mxmlFindElement(tree, tree, "acs", NULL, NULL, MXML_DESCEND);
	if(!b)
	{
		b = mxmlNewElement(tree, "acs");
	}

	b = mxmlFindElement(tree, tree, "url", NULL, NULL, MXML_DESCEND);
	if(b)
	{
		mxmlRemove(b);
	}

	b = mxmlFindElement(tree, tree, "acs", NULL, NULL, MXML_DESCEND);

	b = mxmlNewElement(b, "url");

	b = mxmlNewText(b, 0, value);

	bkp_content = mxmlSaveAllocString(tree, MXML_NO_CALLBACK);
	fp = fopen(CWMP_BKP_FILE, "w");
	fprintf(fp,"%s",bkp_content);
	fclose(fp);
	mxmlDelete(tree);
	pthread_mutex_unlock (&mutex_backup_session);
}

void backup_session_move_inform_to_inform_send ()
{
	FILE *fp;
	mxml_node_t *tree, *b;
	char *bkp_content;

	pthread_mutex_lock (&mutex_backup_session);
	fp = fopen(CWMP_BKP_FILE, "r");
	tree = mxmlLoadFile(NULL, fp, MXML_NO_CALLBACK);
	fclose(fp);

	b = mxmlFindElement(tree, tree, "rpc", "status", "queue", MXML_DESCEND);
	if(b)
	{
		mxmlElementSetAttr(b, "status", "send");
		bkp_content = mxmlSaveAllocString(tree, MXML_NO_CALLBACK);
		fp = fopen(CWMP_BKP_FILE, "w");
		fprintf(fp,"%s",bkp_content);
		fclose(fp);
	}

	mxmlDelete(tree);
	pthread_mutex_unlock (&mutex_backup_session);
}

void backup_session_move_inform_to_inform_queue ()
{
	FILE *fp;
	mxml_node_t *tree, *b;
	char *bkp_content;

	pthread_mutex_lock (&mutex_backup_session);
	fp = fopen(CWMP_BKP_FILE, "r");
	tree = mxmlLoadFile(NULL, fp, MXML_NO_CALLBACK);
	fclose(fp);

	b = mxmlFindElement(tree, tree, "rpc", "status", "send", MXML_DESCEND);
	if(b)
	{
		mxmlElementSetAttr(b, "status", "queue");
		bkp_content = mxmlSaveAllocString(tree, MXML_NO_CALLBACK);
		fp = fopen(CWMP_BKP_FILE, "w");
		fprintf(fp,"%s",bkp_content);
		fclose(fp);
	}
	mxmlDelete(tree);
	pthread_mutex_unlock (&mutex_backup_session);
}

void backup_session_insert_schedule_time(time_t schedule_time,char *commandKey)
{
	FILE *fp;
	mxml_node_t *tree, *b;
	char time[64];
	char *bkp_content;

	pthread_mutex_lock (&mutex_backup_session);
	fp = fopen(CWMP_BKP_FILE, "r");
	tree = mxmlLoadFile(NULL, fp, MXML_NO_CALLBACK);
	fclose(fp);

	b = mxmlFindElement(tree, tree, "scheduledInform", NULL, NULL, MXML_DESCEND);
	if(!b)
	{
		b = mxmlNewElement(tree, "scheduledInform");
	}

	b = mxmlNewElement(b, "scheduledTime");

	if(commandKey != NULL)
	{
		mxmlElementSetAttr(b, "commandKey", commandKey);
	}
	else
	{
		mxmlElementSetAttr(b, "commandKey", "");
	}
    sprintf(time,"%li",schedule_time);
	mxmlElementSetAttr(b, "time", time);

	bkp_content = mxmlSaveAllocString(tree, MXML_NO_CALLBACK);
	fp = fopen(CWMP_BKP_FILE, "w");
	fprintf(fp,"%s",bkp_content);
	fclose(fp);
	mxmlDelete(tree);
	pthread_mutex_unlock (&mutex_backup_session);
}

void backup_session_delete_schedule_time(time_t schedule_time,char *commandKey)
{
	FILE *fp;
	mxml_node_t *tree, *b;
	char time[64];
	char *bkp_content;

	pthread_mutex_lock (&mutex_backup_session);
	fp = fopen(CWMP_BKP_FILE, "r");
	tree = mxmlLoadFile(NULL, fp, MXML_NO_CALLBACK);
	fclose(fp);

	sprintf(time,"%li",schedule_time);
	b = mxmlFindElement(tree, tree, "scheduledTime", "time", time, MXML_DESCEND);

	if(b)
	{
		mxmlRemove(b);
	}

	bkp_content = mxmlSaveAllocString(tree, MXML_NO_CALLBACK);
	fp = fopen(CWMP_BKP_FILE, "w");
	fprintf(fp,"%s",bkp_content);
	fclose(fp);
	mxmlDelete(tree);
	pthread_mutex_unlock (&mutex_backup_session);
}

void backup_session_delete_scheduled_inform()
{
	FILE *fp;
	mxml_node_t *tree, *b;
	char *bkp_content;

	fp = fopen(CWMP_BKP_FILE, "r");
	tree = mxmlLoadFile(NULL, fp, MXML_NO_CALLBACK);
	fclose(fp);

	b = mxmlFindElement(tree, tree, "scheduledInform", NULL, NULL, MXML_DESCEND);

	if(b)
	{
		mxmlRemove(b);
	}

	bkp_content = mxmlSaveAllocString(tree, MXML_NO_CALLBACK);
	fp = fopen(CWMP_BKP_FILE, "w");
	fprintf(fp,"%s",bkp_content);
	fclose(fp);
	mxmlDelete(tree);
	pthread_mutex_unlock (&mutex_backup_session);
}

void backup_session_insert_download(char *commandKey, char *url, time_t schedule_time)
{
	FILE *fp;
	mxml_node_t *tree, *b;
	char time[64];
	char *bkp_content;

	pthread_mutex_lock (&mutex_backup_session);
	fp = fopen(CWMP_BKP_FILE, "r");
	tree = mxmlLoadFile(NULL, fp, MXML_NO_CALLBACK);
	fclose(fp);

	b = mxmlFindElement(tree, tree, "cwmp", NULL, NULL, MXML_DESCEND);

	b = mxmlNewElement(tree, "download");

	if(commandKey != NULL)
	{
		mxmlElementSetAttr(b, "commandKey", commandKey);
	}
	else
	{
		mxmlElementSetAttr(b, "commandKey", "");
	}

	mxmlElementSetAttr(b, "url", url);

	sprintf(time,"%li",schedule_time);
	mxmlElementSetAttr(b, "time", time);

	bkp_content = mxmlSaveAllocString(tree, MXML_NO_CALLBACK);
	fp = fopen(CWMP_BKP_FILE, "w");
	fprintf(fp,"%s",bkp_content);
	fclose(fp);
	mxmlDelete(tree);
	pthread_mutex_unlock (&mutex_backup_session);
}

void backup_session_delete_download(char *commandKey, char *url, time_t schedule_time)
{
	FILE *fp;
	mxml_node_t *tree, *b;
	char time[64];
	char *bkp_content;

	pthread_mutex_lock (&mutex_backup_session);
	fp = fopen(CWMP_BKP_FILE, "r");
	tree = mxmlLoadFile(NULL, fp, MXML_NO_CALLBACK);
	fclose(fp);

	sprintf(time,"%li",schedule_time);
	b = mxmlFindElement(tree, tree, "download", "time", time, MXML_DESCEND);

	if(b)
	{
		mxmlRemove(b);
	}

	bkp_content = mxmlSaveAllocString(tree, MXML_NO_CALLBACK);
	fp = fopen(CWMP_BKP_FILE, "w");
	fprintf(fp,"%s",bkp_content);
	fclose(fp);
	mxmlDelete(tree);
	pthread_mutex_unlock (&mutex_backup_session);
}

void backup_session_insert_parameter_download(char *name, char *value, char *url, time_t schedule_time, char *commandKey)
{
	FILE *fp;
	mxml_node_t *tree, *b;
	char time[64];
	char *bkp_content;

	pthread_mutex_lock (&mutex_backup_session);
	fp = fopen(CWMP_BKP_FILE, "r");
	tree = mxmlLoadFile(NULL, fp, MXML_NO_CALLBACK);
	fclose(fp);

	sprintf(time, "%li", schedule_time);
	b = mxmlFindElement(tree, tree, "download", "time", time, MXML_DESCEND);

	if(b)
	{
		b = mxmlNewElement(b, "parameter");

		mxmlElementSetAttr(b, "name", name);

		mxmlNewText(b, 0, value);

		bkp_content = mxmlSaveAllocString(tree, MXML_NO_CALLBACK);
		fp = fopen(CWMP_BKP_FILE, "w");
		fprintf(fp,"%s",bkp_content);
		fclose(fp);
	}

	mxmlDelete(tree);
	pthread_mutex_unlock (&mutex_backup_session);
}

int cwmp_load_saved_session(struct cwmp *cwmp, char **acsurl, enum backup_loading load)
{
	int                 				i,j,fault_idx;
	char    							*name = NULL, *commandKey  = NULL, *status  = NULL;
	FILE    							*pFile;
	mxml_node_t 						*tree, *n, *b, *c;
	mxml_attr_t							*attr;
	struct download						*download_request = NULL;
	struct event_container				*event_container_save = NULL;
	struct _cwmp1__TransferComplete		*p_soap_cwmp1__TransferComplete = NULL;
    struct list_head					*ilist = NULL;
    struct download						*idownload_request = NULL;
    char								*event_name = NULL;
	char								*event_commandKey = NULL;
	char								*event_id = NULL;
	int									event_idx;
	struct schedule_inform				*schedule_inform = NULL;
	char                            	*scheduled_commandKey = NULL;
	time_t								scheduled_time;

	pthread_mutex_lock (&mutex_backup_session);
	pFile = fopen(CWMP_BKP_FILE,"r+");

	if(pFile == NULL)
	{
		pFile = fopen(CWMP_BKP_FILE,"w");
		if(pFile == NULL)
		{
			CWMP_LOG(ERROR,"Unable to create %s file",CWMP_BKP_FILE);
			pthread_mutex_unlock (&mutex_backup_session);
			return CWMP_MEM_ERR;
		}
		fprintf(pFile,"%s",CWMP_BACKUP_SESSION);
		fclose(pFile);
		pthread_mutex_unlock (&mutex_backup_session);
		return CWMP_OK;
	}
	fclose(pFile);
	pthread_mutex_unlock (&mutex_backup_session);
	backup_session_move_inform_to_inform_queue ();
	pthread_mutex_lock (&mutex_backup_session);
	pFile = fopen(CWMP_BKP_FILE, "r");
	tree = mxmlLoadFile(NULL, pFile, MXML_NO_CALLBACK);
	fclose(pFile);

	n = tree;

	if (!n)
	{
		CWMP_LOG(ERROR,"Backup file %s was empty",CWMP_BKP_FILE);
		return CWMP_GEN_ERR;
	}

	b = n;

	while (b != NULL)
	{
		if(b->type == MXML_ELEMENT)
		{
			if(load == ALL || load == ACS)
			{
				if ((b != NULL) && (strcmp(b->value.element.name, "url") == 0))
				{
					if(b->child != NULL)
					{
						b = mxmlWalkNext(b, n, MXML_DESCEND);
						if (b->type == MXML_TEXT)
						{
							if(acsurl != NULL)
							{
								if(b->value.text.string != NULL)
								{
									*acsurl = strdup(b->value.text.string);
								}
								else
								{
									*acsurl =  NULL;
								}
							}
						}
					}
					else
					{
						if(acsurl != NULL)
						{
							*acsurl =  NULL;
						}
					}
					if(load == ACS)
					{
						break;
					}
				}
			}
			if(load == ALL)
			{
				if ((b != NULL) && (b->type == MXML_ELEMENT) && (strcmp(b->value.element.name, "download") == 0))
				{
					download_request = calloc(1,sizeof(struct download));
					for (i = b->value.element.num_attrs, attr = b->value.element.attrs;i > 0;i --, attr ++)
					{
						if(strcmp(attr->name,"commandKey") == 0)
						{
							download_request->CommandKey = strdup(attr->value);
						}
						if(strcmp(attr->name,"url") == 0)
						{
							download_request->URL = strdup(attr->value);
						}
						if(strcmp(attr->name,"time") == 0)
						{
							download_request->scheduled_time = atol(attr->value);
						}
					}
					while ((b != NULL)&&(b = mxmlWalkNext(b, n, MXML_DESCEND)) && (strcmp(b->parent->value.element.name,"download") == 0))
					{
						if (b->type == MXML_ELEMENT)
						{
							for (i = b->value.element.num_attrs, attr = b->value.element.attrs;i > 0;i --, attr ++)
							{
								if(strcmp(attr->name,"name") == 0)
								{
									if((attr->value!= NULL) && (strcmp(attr->value,"FileType") == 0))
									{
										b = mxmlWalkNext(b, n, MXML_DESCEND);
										if (b->type == MXML_TEXT)
										{
											download_request->FileType = strdup(b->value.text.string);
										}
										break;
									}
									else if ((attr->value!= NULL) && (strcmp(attr->value,"Username") == 0))
									{
										b = mxmlWalkNext(b, n, MXML_DESCEND);
										if (b->type == MXML_TEXT)
										{
											download_request->Username = strdup(b->value.text.string);
										}
										break;
									}
									else if ((attr->value!= NULL) && (strcmp(attr->value,"Password") == 0))
									{
										b = mxmlWalkNext(b, n, MXML_DESCEND);
										if (b->type == MXML_TEXT)
										{
											download_request->Password = strdup(b->value.text.string);
										}
										break;
									}
								}
							}
						}
					}
					list_for_each(ilist,&(list_download))
					{
						idownload_request = list_entry(ilist,struct download,list);
						if (idownload_request->scheduled_time > download_request->scheduled_time)
						{
							break;
						}
					}
					list_add (&(download_request->list), ilist->prev);
					count_download_queue++;
				}
				if ((b != NULL) && (b->type == MXML_ELEMENT) && (strcmp(b->value.element.name, "rpc") == 0))
				{
					for (i = b->value.element.num_attrs, attr = b->value.element.attrs;i > 0;i --, attr ++)
					{
						if(strcmp(attr->name,"name") == 0)
						{
							name = strdup(attr->value);
						}
						if(strcmp(attr->name,"commandKey") == 0)
						{
							commandKey = strdup(attr->value);
						}
						if(strcmp(attr->name,"status") == 0)
						{
							status = strdup(attr->value);
						}
					}
					if(strcmp(name,"TransferComplete") == 0)
					{
						p_soap_cwmp1__TransferComplete = cwmp_set_data_rpc_acs_transferComplete ();
						p_soap_cwmp1__TransferComplete->CommandKey = strdup(commandKey);
						while ((b != NULL)&&(b = mxmlWalkNext(b, n, MXML_DESCEND))&&(strcmp(b->parent->value.element.name,"rpc") == 0)&&(strcmp(name,"TransferComplete") == 0))
						{
							if ((b->type == MXML_ELEMENT) && (strcmp(b->value.element.name,"parameter") == 0))
							{
								for (i = b->value.element.num_attrs, attr = b->value.element.attrs;i > 0;i --, attr ++)
								{
									if(strcmp(attr->name,"name") == 0)
									{
										if(strcmp(attr->value,"FaultCode") == 0)
										{
											b = mxmlWalkNext(b, n, MXML_DESCEND);
											if (b->type == MXML_TEXT)
											{
												p_soap_cwmp1__TransferComplete->FaultStruct = calloc(1,sizeof(struct cwmp1__FaultStruct));
												p_soap_cwmp1__TransferComplete->FaultStruct->FaultCode = strdup(b->value.text.string);
												for(j = 0; j < FAULT_CPE_ARRAY_SIZE; j++)
												{
													if((FAULT_CPE_ARRAY[j].CODE != NULL)&&(strcmp(FAULT_CPE_ARRAY[j].CODE, b->value.text.string) == 0))
													{
														fault_idx = j;
														break;
													}
												}
												p_soap_cwmp1__TransferComplete->FaultStruct->FaultString = strdup(FAULT_CPE_ARRAY[fault_idx].DESCRIPTION);
											}
											break;
										}
										else if(strcmp(attr->value,"StartTime") == 0)
										{
											b = mxmlWalkNext(b, n, MXML_DESCEND);
											if (b->type == MXML_TEXT)
											{
												p_soap_cwmp1__TransferComplete->StartTime = atol(b->value.text.string);
											}
											break;
										}
										else if(strcmp(attr->value,"CompleteTime") == 0)
										{
											b = mxmlWalkNext(b, n, MXML_DESCEND);
											if (b->type == MXML_TEXT)
											{
												p_soap_cwmp1__TransferComplete->CompleteTime = atol(b->value.text.string);
											}
											break;
										}
									}
								}
							}
						}
						cwmp_root_cause_TransferComplete (cwmp);
					}
					if((strcmp(name,"Inform") == 0) && (strcmp(status,"queue") == 0))
					{
						while ((b != NULL) && (b = mxmlWalkNext(b, n, MXML_DESCEND)) && (b->type == MXML_ELEMENT) && (strcmp(b->value.element.name, "event") == 0) && (strcmp(b->parent->value.element.name, "rpc") == 0))
						{
							for (i = b->value.element.num_attrs, attr = b->value.element.attrs;i > 0;i --, attr ++)
							{
								if(strcmp(attr->name,"name") == 0)
								{
									event_name = strdup(attr->value);
								}
								if(strcmp(attr->name,"commandKey") == 0)
								{
									event_commandKey = strdup(attr->value);
								}
								if(strcmp(attr->name,"id") == 0)
								{
									event_id = strdup(attr->value);
								}
							}
							for (j = 0; j< COUNT_EVENT; j++)
							{
								if(strcmp(event_name,EVENT_CONST[j].CODE) == 0)
								{
									event_idx = j;
									break;
								}
							}
							if ((cwmp->env.boot != CWMP_START_BOOT) || (EVENT_CONST[event_idx].RETRY & EVENT_RETRY_AFTER_REBOOT))
							{
								event_container_save = cwmp_add_event_container (cwmp, event_idx, event_commandKey);
								if(event_container_save != NULL)
								{
									event_container_save->id = atoi(event_id);
								}
							}
							while ((b != NULL)&&(b = mxmlWalkNext(b, n, MXML_DESCEND))&&(strcmp(b->parent->value.element.name,"event") == 0))
							{
								if ((b->type == MXML_ELEMENT) && (strcmp(b->value.element.name,"parameter") == 0))
								{
									for (i = b->value.element.num_attrs, attr = b->value.element.attrs;i > 0;i --, attr ++)
									{
										if(strcmp(attr->name,"name") == 0)
										{
											if(event_container_save != NULL)
											{
												cwmp_add_parameter_container (cwmp,event_container_save, attr->value, NULL, NULL);
											}
										}
									}
								}
							}
							if ((b != NULL)&&(b->type == MXML_ELEMENT)&&(strcmp(b->value.element.name,"event") == 0))
							{
								b = mxmlWalkPrev(b, n, MXML_DESCEND);
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
					}
					if(name != NULL)
					{
						free(name);
						name = NULL;
					}
					if(status != NULL)
					{
						free(status);
						status = NULL;
					}
					if(commandKey != NULL)
					{
						free(commandKey);
						commandKey = NULL;
					}
				}
				if ((b != NULL) && (b->type == MXML_ELEMENT) && (strcmp(b->value.element.name, "scheduledTime") == 0))
				{
					for (i = b->value.element.num_attrs, attr = b->value.element.attrs;i > 0;i --, attr ++)
					{
						if(strcmp(attr->name,"commandKey") == 0)
						{
							scheduled_commandKey = strdup(attr->value);
						}
						if(strcmp(attr->name,"time") == 0)
						{
							scheduled_time = atol(attr->value);
						}
					}
					list_for_each(ilist,&(list_schedule_inform))
					{
						schedule_inform = list_entry(ilist,struct schedule_inform, list);
						if (schedule_inform->scheduled_time > scheduled_time)
						{
							break;
						}
					}
					schedule_inform = calloc (1,sizeof(struct schedule_inform));
					if (schedule_inform != NULL)
					{
						schedule_inform->commandKey     = scheduled_commandKey;
						schedule_inform->scheduled_time = scheduled_time;
						list_add (&(schedule_inform->list), ilist->prev);
					}
				}
			}
		}
		b = mxmlWalkNext(b, n, MXML_DESCEND);
	}

	mxmlDelete(tree);
	pthread_mutex_unlock (&mutex_backup_session);
	return CWMP_OK;
}
