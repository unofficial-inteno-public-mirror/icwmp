/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *	Powered by Inteno Broadband Technology AB
 *
 *	Copyright (C) 2013 Mohamed Kallel <mohamed.kallel@pivasoftware.com>
 *	Copyright (C) 2013 Ahmed Zribi <ahmed.zribi@pivasoftware.com>
 *
 */

#include "cwmp.h"
#include "backupSession.h"
#include "xml.h"
#include "log.h"

static mxml_node_t		*bkp_tree;
pthread_mutex_t         mutex_backup_session = PTHREAD_MUTEX_INITIALIZER;

void bkp_session_save()
{
	FILE *fp;

	pthread_mutex_lock (&mutex_backup_session);
	fp = fopen(CWMP_BKP_FILE, "w");
	mxmlSaveFile(bkp_tree, fp, MXML_NO_CALLBACK);
	fclose(fp);
	sync();
	pthread_mutex_unlock (&mutex_backup_session);
}

mxml_node_t *bkp_session_insert(mxml_node_t *tree, char *name, char *value)
{
	mxml_node_t *b;

	b = mxmlNewElement(tree, name);

	if(value != NULL)
	{
		mxmlNewText(b, 0, value);
	}
	return b;
}
/*
 * The order of key array filling should be the same of insertion function
 */
mxml_node_t *bkp_session_node_found(mxml_node_t *tree, char *name, struct search_keywords *keys, int size)
{
	mxml_node_t *b = tree, *c, *d;
	struct search_keywords;
	int i;

	b = mxmlFindElement(b, b, name, NULL, NULL, MXML_DESCEND_FIRST);
	while (b)
	{
		if(b && b->child)
		{
			c = b->child;
			i = 0;
			while(c && i < size)
			{
				if(c->type == MXML_ELEMENT && strcmp(keys[i].name, c->value.element.name) == 0)
				{
					d = c;
					d = mxmlWalkNext(d, c, MXML_DESCEND);
					if(d->type == MXML_TEXT && strcmp(keys[i].value, d->value.text.string) == 0)
						i++;
				}
				c = mxmlWalkNext(c, b, MXML_NO_DESCEND);
			}
		}
		if(i == size)
		{
			break;
		}
		b = mxmlWalkNext(b, tree, MXML_NO_DESCEND);
	}
	return b;
}

mxml_node_t *bkp_session_insert_event(int index, char *command_key, int id, char *status)
{
	struct search_keywords	keys[1];
	char 					parent_name[32];
	char 					event_id[32];
	char					event_idx[32];
	mxml_node_t				*b;

	pthread_mutex_lock (&mutex_backup_session);
	sprintf(parent_name,"%s_event",status);
	sprintf(event_id,"%d",id);
	sprintf(event_idx,"%d",index);
	keys[0].name = "id";
	keys[0].value = event_id;
	b = bkp_session_node_found(bkp_tree, parent_name, keys, 1);
	if(!b)
	{
		b = bkp_session_insert(bkp_tree,parent_name,NULL);
		bkp_session_insert(b,"index",event_idx);
		bkp_session_insert(b,"id",event_id);
		bkp_session_insert(b,"command_key",command_key);
	}
	pthread_mutex_unlock (&mutex_backup_session);
	return b;
}

void bkp_session_delete_event(int id, char *status)
{
	struct search_keywords	keys[1];
	char 					parent_name[32];
	char 					event_id[32];
	mxml_node_t				*b;

	pthread_mutex_lock (&mutex_backup_session);
	sprintf(parent_name,"%s_event",status);
	sprintf(event_id,"%d",id);
	keys[0].name = "id";
	keys[0].value = event_id;
	b = bkp_session_node_found(bkp_tree, parent_name, keys, 1);
	if(b)
		mxmlDelete(b);
	pthread_mutex_unlock (&mutex_backup_session);
}

void bkp_session_insert_parameter(mxml_node_t *b, char *name)
{
	pthread_mutex_lock (&mutex_backup_session);
	bkp_session_insert(b,"parameter",name);
	pthread_mutex_unlock (&mutex_backup_session);
}

void bkp_session_insert_acs(char *value)
{
	mxml_node_t *b = bkp_tree;
	struct search_keywords *key;

	pthread_mutex_lock (&mutex_backup_session);
	b = mxmlFindElement(b, b, "acs", NULL, NULL, MXML_DESCEND);
	if(b)
		mxmlDelete(b);
	b = bkp_session_insert(bkp_tree, "acs",NULL);
	bkp_session_insert(b,"url",value);
	pthread_mutex_unlock (&mutex_backup_session);
}

void bkp_session_move_inform_to_inform_send ()
{
	mxml_node_t *b = bkp_tree;

	pthread_mutex_lock (&mutex_backup_session);
	while (b) {
		if (b && b->type == MXML_ELEMENT &&
			!strcmp(b->value.element.name, "queue_event") &&
			b->parent->type == MXML_ELEMENT &&
			!strcmp(b->parent->value.element.name, "cwmp")) {
			FREE(b->value.element.name);
			b->value.element.name = strdup("send_event");
		}
		b = mxmlWalkNext(b, bkp_tree, MXML_DESCEND);
	}
	pthread_mutex_unlock (&mutex_backup_session);
}

void bkp_session_move_inform_to_inform_queue ()
{
	mxml_node_t *b = bkp_tree;

	pthread_mutex_lock (&mutex_backup_session);
	while (b) {
		if (b && b->type == MXML_ELEMENT &&
			!strcmp(b->value.element.name, "send_event") &&
			b->parent->type == MXML_ELEMENT &&
			!strcmp(b->parent->value.element.name, "cwmp")) {
			FREE(b->value.element.name);
			b->value.element.name = strdup("queue_event");
		}
		b = mxmlWalkNext(b, bkp_tree, MXML_DESCEND);
	}
	pthread_mutex_unlock (&mutex_backup_session);
}

void bkp_session_insert_schedule_inform(time_t time,char *command_key)
{
	struct search_keywords keys[2];
	char schedule_time[128];
	mxml_node_t *b;

	pthread_mutex_lock (&mutex_backup_session);
	sprintf(schedule_time,"%ld",time);
	keys[0].name = "command_key";
	keys[0].value = command_key;
	keys[1].name = "time";
	keys[1].value = schedule_time;
	b = bkp_session_node_found(bkp_tree, "schedule_inform", keys, 2);
	if(!b)
	{
		b = bkp_session_insert(bkp_tree,"schedule_inform",NULL);
		bkp_session_insert(b,"command_key",command_key);
		bkp_session_insert(b,"time",schedule_time);
	}
	pthread_mutex_unlock (&mutex_backup_session);
}

void bkp_session_delete_schedule_inform(time_t time,char *command_key)
{
	struct search_keywords keys[2];
	char schedule_time[128];
	mxml_node_t *b;

	pthread_mutex_lock (&mutex_backup_session);
	sprintf(schedule_time,"%ld",time);
	keys[0].name = "command_key";
	keys[0].value = command_key;
	keys[1].name = "time";
	keys[1].value = schedule_time;
	b = bkp_session_node_found(bkp_tree, "schedule_inform", keys, 2);
	if(b)
		mxmlDelete(b);
	pthread_mutex_unlock (&mutex_backup_session);
}

void bkp_session_insert_download(struct download *pdownload)
{
	struct search_keywords	keys[7];
	char 					schedule_time[128];
	char 					file_size[128];
	mxml_node_t 			*b;

	pthread_mutex_lock (&mutex_backup_session);
	sprintf(schedule_time,"%ld",pdownload->scheduled_time);
	sprintf(file_size,"%d",pdownload->file_size);
	keys[0].name = "url";
	keys[0].value = pdownload->url;
	keys[1].name = "command_key";
	keys[1].value = pdownload->command_key;
	keys[2].name = "file_type";
	keys[2].value = pdownload->file_type;
	keys[3].name = "username";
	keys[3].value = pdownload->username;
	keys[4].name = "password";
	keys[4].value = pdownload->password;
	keys[5].name = "file_size";
	keys[5].value = file_size;
	keys[6].name = "time";
	keys[6].value = schedule_time;
	b = bkp_session_node_found(bkp_tree, "download", keys, 7);
	if(!b)
	{
		b = bkp_session_insert(bkp_tree,"download",NULL);
		bkp_session_insert(b,"url",pdownload->url);
		bkp_session_insert(b,"command_key",pdownload->command_key);
		bkp_session_insert(b,"file_type",pdownload->file_type);
		bkp_session_insert(b,"username",pdownload->username);
		bkp_session_insert(b,"password",pdownload->password);
		bkp_session_insert(b,"file_size",file_size);
		bkp_session_insert(b,"time",schedule_time);
	}
	pthread_mutex_unlock (&mutex_backup_session);
}

void bkp_session_delete_download(struct download *pdownload)
{
	struct search_keywords keys[7];
	char schedule_time[128];
	char file_size[128];
	mxml_node_t *b;

	pthread_mutex_lock (&mutex_backup_session);
	sprintf(schedule_time,"%ld",pdownload->scheduled_time);
	sprintf(file_size,"%d",pdownload->file_size);
	keys[0].name = "url";
	keys[0].value = pdownload->url;
	keys[1].name = "command_key";
	keys[1].value = pdownload->command_key;
	keys[2].name = "file_type";
	keys[2].value = pdownload->file_type;
	keys[3].name = "username";
	keys[3].value = pdownload->username;
	keys[4].name = "password";
	keys[4].value = pdownload->password;
	keys[5].name = "file_size";
	keys[5].value = file_size;
	keys[6].name = "time";
	keys[6].value = schedule_time;
	b = bkp_session_node_found(bkp_tree, "download", keys, 7);
	if(b)
		mxmlDelete(b);
	pthread_mutex_unlock (&mutex_backup_session);
}

void bkp_session_insert_transfer_complete(struct transfer_complete *ptransfer_complete)
{
	struct search_keywords	keys[4];
	char					fault_code[16];
	mxml_node_t 			*b;

	pthread_mutex_lock (&mutex_backup_session);
	sprintf(fault_code,"%d",ptransfer_complete->fault_code);
	keys[0].name = "command_key";
	keys[0].value = ptransfer_complete->command_key;
	keys[1].name = "start_time";
	keys[1].value = ptransfer_complete->start_time;
	keys[2].name = "complete_time";
	keys[2].value = ptransfer_complete->complete_time;
	keys[3].name = "fault_code";
	keys[3].value = fault_code;
	b = bkp_session_node_found(bkp_tree, "transfer_complete", keys, 4);
	if(!b)
	{
		b = bkp_session_insert(bkp_tree,"transfer_complete",NULL);
		bkp_session_insert(b,"command_key",ptransfer_complete->command_key);
		bkp_session_insert(b,"start_time",ptransfer_complete->start_time);
		bkp_session_insert(b,"complete_time",ptransfer_complete->complete_time);
		bkp_session_insert(b,"fault_code",fault_code);
	}
	pthread_mutex_unlock (&mutex_backup_session);
}

void bkp_session_delete_transfer_complete(struct transfer_complete *ptransfer_complete)
{
	struct search_keywords	keys[4];
	char					fault_code[16];
	mxml_node_t 			*b;

	pthread_mutex_lock (&mutex_backup_session);
	sprintf(fault_code,"%d",ptransfer_complete->fault_code);
	keys[0].name = "command_key";
	keys[0].value = ptransfer_complete->command_key;
	keys[1].name = "start_time";
	keys[1].value = ptransfer_complete->start_time;
	keys[2].name = "complete_time";
	keys[2].value = ptransfer_complete->complete_time;
	keys[3].name = "fault_code";
	keys[3].value = fault_code;
	b = bkp_session_node_found(bkp_tree, "transfer_complete", keys, 4);
	if(b)
		mxmlDelete(b);
	pthread_mutex_unlock (&mutex_backup_session);
}
/*
 * Load backup session
 */
char *load_acs_url(mxml_node_t *tree)
{
	char		*acs_url = NULL;
	mxml_node_t	*b = tree;

	if (b) {
		b = mxmlWalkNext(b, tree, MXML_DESCEND);
		if (b && b->type == MXML_ELEMENT && strcmp(b->value.element.name, "url") == 0) {
			b = mxmlWalkNext(b, tree, MXML_DESCEND);
			if (b && b->type == MXML_TEXT)
			{
				if(b->value.text.string != NULL)
				{
					acs_url = strdup(b->value.text.string);
				}
			}
		}
	}

	return acs_url;
}

void load_queue_event(mxml_node_t *tree,struct cwmp *cwmp)
{
	char						*command_key = NULL;
	mxml_node_t					*b = tree, *c;
	int							idx = -1, id;
	struct event_container		*event_container_save = NULL;

	b = mxmlWalkNext(b, tree, MXML_DESCEND);

	while (b) {
		if (b && b->type == MXML_ELEMENT) {
			if(strcmp(b->value.element.name, "index") == 0)
			{
				c = mxmlWalkNext(b, b, MXML_DESCEND);
				if (c && c->type == MXML_TEXT)
				{
					if(c->value.text.string != NULL)
					{
						idx = atoi(c->value.text.string);
					}
				}
			}
			else if(strcmp(b->value.element.name, "id") == 0)
			{
				c = mxmlWalkNext(b, b, MXML_DESCEND);
				if (c && c->type == MXML_TEXT)
				{
					if(c->value.text.string != NULL)
					{
						id = atoi(c->value.text.string);
					}
				}
			}
			else if(strcmp(b->value.element.name, "command_key") == 0)
			{
				c = mxmlWalkNext(b, b, MXML_DESCEND);
				if (c && c->type == MXML_TEXT)
				{
					if(c->value.text.string != NULL)
					{
						command_key = strdup(c->value.text.string);
					}
				}
				if(idx != -1)
				{
					if ((cwmp->env.boot != CWMP_START_BOOT) || (EVENT_CONST[idx].RETRY & EVENT_RETRY_AFTER_REBOOT))
					{
						event_container_save = cwmp_add_event_container (cwmp, idx, ((command_key!=NULL)?command_key:""));
						if(event_container_save != NULL)
						{
							event_container_save->id = id;
						}
					}
				}
				FREE(command_key);
			}
			else if(strcmp(b->value.element.name, "parameter") == 0)
			{
				c = mxmlWalkNext(b, b, MXML_DESCEND);
				if (c && c->type == MXML_TEXT)
				{
					if(c->value.text.string != NULL)
					{
						if(event_container_save != NULL)
						{
							parameter_container_add(&(event_container_save->head_parameter_container),
									c->value.text.string, NULL, NULL, NULL);
						}
					}
				}
			}
		}
		b = mxmlWalkNext(b, tree, MXML_NO_DESCEND);
	}
}

void load_schedule_inform(mxml_node_t *tree,struct cwmp *cwmp)
{
	char						*command_key;
	mxml_node_t					*b = tree, *c;
	time_t						scheduled_time;
	struct schedule_inform		*schedule_inform = NULL;
	struct list_head			*ilist = NULL;

	b = mxmlWalkNext(b, tree, MXML_DESCEND);

	while (b) {
		if (b && b->type == MXML_ELEMENT) {
			if (strcmp(b->value.element.name, "command_key") == 0)
			{
				c = mxmlWalkNext(b, b, MXML_DESCEND);
				if (c->type == MXML_TEXT)
				{
					if(c->value.text.string != NULL)
					{
						command_key = strdup(c->value.text.string);
					}
				}
			}
			else if (strcmp(b->value.element.name, "time") == 0)
			{
				c = mxmlWalkNext(b, b, MXML_DESCEND);
				if (c->type == MXML_TEXT)
				{
					if(c->value.text.string != NULL)
					{
						scheduled_time = atol(c->value.text.string);
					}
				}
			}
		}
		b = mxmlWalkNext(b, tree, MXML_NO_DESCEND);
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
		schedule_inform->commandKey     = command_key;
		schedule_inform->scheduled_time = scheduled_time;
		list_add (&(schedule_inform->list), ilist->prev);
	}
}

void load_download(mxml_node_t *tree,struct cwmp *cwmp)
{
	mxml_node_t			*b = tree, *c;
	struct download		*download_request = NULL;
    struct list_head	*ilist = NULL;
    struct download		*idownload_request = NULL;

	download_request = calloc(1,sizeof(struct download));

	b = mxmlWalkNext(b, tree, MXML_DESCEND);

	while (b) {
		if (b && b->type == MXML_ELEMENT) {
			if (strcmp(b->value.element.name, "url") == 0)
			{
				c = mxmlWalkNext(b, b, MXML_DESCEND);
				if (c->type == MXML_TEXT)
				{
					if(c->value.text.string != NULL)
					{
						download_request->url = strdup(c->value.text.string);
					}
				}
			}
			else if (strcmp(b->value.element.name, "command_key") == 0)
			{
				c = mxmlWalkNext(b, b, MXML_DESCEND);
				if (c->type == MXML_TEXT)
				{
					if(c->value.text.string != NULL)
					{
						download_request->command_key = strdup(c->value.text.string);
					}
				}
			}
			else if (strcmp(b->value.element.name, "file_type") == 0)
			{
				c = mxmlWalkNext(b, b, MXML_DESCEND);
				if (c->type == MXML_TEXT)
				{
					if(c->value.text.string != NULL)
					{
						download_request->file_type = strdup(c->value.text.string);
					}
				}
			}
			else if (strcmp(b->value.element.name, "username") == 0)
			{
				c = mxmlWalkNext(b, b, MXML_DESCEND);
				if (c->type == MXML_TEXT)
				{
					if(c->value.text.string != NULL)
					{
						download_request->username = strdup(c->value.text.string);
					}
				}
			}
			else if (strcmp(b->value.element.name, "password") == 0)
			{
				c = mxmlWalkNext(b, b, MXML_DESCEND);
				if (c->type == MXML_TEXT)
				{
					if(c->value.text.string != NULL)
					{
						download_request->password = strdup(c->value.text.string);
					}
				}
			}
			else if (strcmp(b->value.element.name, "file_size") == 0)
			{
				c = mxmlWalkNext(b, b, MXML_DESCEND);
				if (c->type == MXML_TEXT)
				{
					if(c->value.text.string != NULL)
					{
						download_request->file_size = atoi(c->value.text.string);
					}
				}
			}
			else if (strcmp(b->value.element.name, "time") == 0)
			{
				c = mxmlWalkNext(b, b, MXML_DESCEND);
				if (c->type == MXML_TEXT)
				{
					if(c->value.text.string != NULL)
					{
						download_request->scheduled_time = atol(c->value.text.string);
					}
				}
			}
		}
		b = mxmlWalkNext(b, tree, MXML_NO_DESCEND);
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
	if(download_request->scheduled_time != 0)
		count_download_queue++;
}

void load_transfer_complete(mxml_node_t	*tree,struct cwmp *cwmp)
{
	mxml_node_t						*b = tree, *c;
	struct transfer_complete 		*ptransfer_complete;

	b = mxmlWalkNext(b, tree, MXML_DESCEND);

	ptransfer_complete = calloc (1,sizeof(struct transfer_complete));

	while (b) {
		if (b && b->type == MXML_ELEMENT) {
			if (strcmp(b->value.element.name,"command_key") == 0)
			{
				c = mxmlWalkNext(b, b, MXML_DESCEND);
				if (c->type == MXML_TEXT)
				{
					if(c->value.text.string != NULL)
					{
						ptransfer_complete->command_key = strdup(c->value.text.string);
					}
				}
			}
			else if (strcmp(b->value.element.name,"start_time") == 0)
			{
				c = mxmlWalkNext(b, b, MXML_DESCEND);
				if (c->type == MXML_TEXT)
				{
					if(c->value.text.string != NULL)
					{
						ptransfer_complete->start_time = strdup(c->value.text.string);
					}
				}
			}
			else if (strcmp(b->value.element.name,"complete_time") == 0)
			{
				c = mxmlWalkNext(b, b, MXML_DESCEND);
				if (c->type == MXML_TEXT)
				{
					if(c->value.text.string != NULL)
					{
						ptransfer_complete->complete_time = strdup(c->value.text.string);
					}
				}
			}
			else if (strcmp(b->value.element.name,"fault_code") == 0)
			{
				c = mxmlWalkNext(b, b, MXML_DESCEND);
				if (c->type == MXML_TEXT)
				{
					if(c->value.text.string != NULL)
					{
						ptransfer_complete->fault_code = atoi(c->value.text.string);
					}
				}
			}
		}
		b = mxmlWalkNext(b, tree, MXML_NO_DESCEND);
	}
	cwmp_root_cause_TransferComplete (cwmp, ptransfer_complete);
}

int cwmp_load_saved_session(struct cwmp *cwmp, char **acsurl, enum backup_loading load)
{
	char		*acs_url = NULL;
	FILE		*pFile;
	mxml_node_t *b;

	if(access(CWMP_BKP_FILE, F_OK) == -1)
	{
		pthread_mutex_lock (&mutex_backup_session);
		pFile = fopen(CWMP_BKP_FILE,"w");
		if(pFile == NULL)
		{
			CWMP_LOG(ERROR,"Unable to create %s file",CWMP_BKP_FILE);
			pthread_mutex_unlock (&mutex_backup_session);
			return CWMP_MEM_ERR;
		}
		fprintf(pFile,"%s",CWMP_BACKUP_SESSION);
		bkp_tree = mxmlLoadString(NULL, CWMP_BACKUP_SESSION, MXML_NO_CALLBACK);
		fclose(pFile);
		pthread_mutex_unlock (&mutex_backup_session);
		return CWMP_OK;
	}

	pFile = fopen(CWMP_BKP_FILE, "r");
	bkp_tree = mxmlLoadFile(NULL, pFile, MXML_NO_CALLBACK);
	fclose(pFile);

	bkp_session_move_inform_to_inform_queue ();
	bkp_session_save();

	b = bkp_tree;
	b = mxmlWalkNext(b, bkp_tree, MXML_DESCEND);
	while(b)
	{
		if(load == ALL || load == ACS)
		{
			if(b->type == MXML_ELEMENT && strcmp(b->value.element.name, "acs") == 0)
			{
				acs_url = load_acs_url(b);
				if(acsurl != NULL)
				{
					*acsurl = acs_url;
				}
			}
		}
		if(load == ALL)
		{
			if(b->type == MXML_ELEMENT && strcmp(b->value.element.name, "queue_event") == 0)
			{
				load_queue_event(b,cwmp);
			}
			else if(b->type == MXML_ELEMENT && strcmp(b->value.element.name, "download") == 0)
			{
				load_download(b,cwmp);
			}
			else if(b->type == MXML_ELEMENT && strcmp(b->value.element.name, "transfer_complete") == 0)
			{
				load_transfer_complete(b,cwmp);
			}
			else if(b->type == MXML_ELEMENT && strcmp(b->value.element.name, "schedule_inform") == 0)
			{
				load_schedule_inform(b,cwmp);
			}
		}
		b = mxmlWalkNext(b, bkp_tree, MXML_NO_DESCEND);
	}

	return CWMP_OK;
}
