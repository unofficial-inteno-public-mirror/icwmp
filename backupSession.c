/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2013 Inteno Broadband Technology AB
 *	  Author Mohamed Kallel <mohamed.kallel@pivasoftware.com>
 *	  Author Ahmed Zribi <ahmed.zribi@pivasoftware.com>
 *
 */

#include "cwmp.h"
#include "backupSession.h"
#include "xml.h"
#include "log.h"

static mxml_node_t		*bkp_tree = NULL;
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
					if((keys[i].value == NULL)||(d && d->type == MXML_TEXT && keys[i].value != NULL && strcmp(keys[i].value, d->value.text.string) == 0))
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

void bkp_session_simple_insert(char *parent, char *child, char *value)
{
	mxml_node_t *b = bkp_tree;

	pthread_mutex_lock (&mutex_backup_session);
	b = mxmlFindElement(b, b, parent, NULL, NULL, MXML_DESCEND);
	if(b)
		mxmlDelete(b);
	b = bkp_session_insert(bkp_tree, parent,NULL);
	bkp_session_insert(b,child,value);
	pthread_mutex_unlock (&mutex_backup_session);
}

void bkp_session_simple_insert_in_parent(char *parent, char *child, char *value)
{
	mxml_node_t *n, *b = bkp_tree;

	pthread_mutex_lock (&mutex_backup_session);
	n = mxmlFindElement(b, b, parent, NULL, NULL, MXML_DESCEND);
	if(!n)
		n = bkp_session_insert(bkp_tree, parent,NULL);
	b = mxmlFindElement(n, n, child, NULL, NULL, MXML_DESCEND);
	if(b)
		mxmlDelete(b);
	bkp_session_insert(n,child,value);
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

void bkp_session_insert_schedule_download(struct schedule_download *pschedule_download)
{
	struct search_keywords	keys[16];
	char 					delay[4][128];
	int 					i;
	char 					file_size[128];
	char 					maxretrie[2][128];
	mxml_node_t 			*b;

	pthread_mutex_lock (&mutex_backup_session);
	sprintf(file_size,"%d",pschedule_download->file_size);
	for (i = 0; i<2; i++)
	{
		sprintf(delay[2*i],"%ld",pschedule_download->timewindowstruct[i].windowstart);
		sprintf(delay[2*i+1],"%ld",pschedule_download->timewindowstruct[i].windowend);
		sprintf(maxretrie[i],"%d",pschedule_download->timewindowstruct[i].maxretries);
	}
	keys[0].name = "url";
	keys[0].value = pschedule_download->url;
	keys[1].name = "command_key";
	keys[1].value = pschedule_download->command_key;
	keys[2].name = "file_type";
	keys[2].value = pschedule_download->file_type;
	keys[3].name = "username";
	keys[3].value = pschedule_download->username;
	keys[4].name = "password";
	keys[4].value = pschedule_download->password;
	keys[5].name = "file_size";
	keys[5].value = file_size;
	keys[6].name = "windowstart1";
	keys[6].value = delay[0];
	keys[7].name = "windowend1";
	keys[7].value = delay[1];
	keys[8].name = "windowmode1";
	keys[8].value = pschedule_download->timewindowstruct[0].windowmode;
	keys[9].name = "usermessage1";
	keys[9].value = pschedule_download->timewindowstruct[0].usermessage;
	keys[10].name = "maxretrie1";
	keys[10].value = maxretrie[0];
	keys[11].name = "windowstart2";
	keys[11].value = delay[2];
	keys[12].name = "windowend2";
	keys[12].value = delay[3];
	keys[13].name = "windowmode2";
	keys[13].value = pschedule_download->timewindowstruct[1].windowmode;
	keys[14].name = "usermessage2";
	keys[14].value = pschedule_download->timewindowstruct[1].usermessage;
	keys[15].name = "maxretrie2";
	keys[15].value = maxretrie[1];
	b = bkp_session_node_found(bkp_tree, "schedule_download", keys, 16);
	if(!b)
	{
		b = bkp_session_insert(bkp_tree,"schedule_download",NULL);
		bkp_session_insert(b,"url",pschedule_download->url);
		bkp_session_insert(b,"command_key",pschedule_download->command_key);
		bkp_session_insert(b,"file_type",pschedule_download->file_type);
		bkp_session_insert(b,"username",pschedule_download->username);
		bkp_session_insert(b,"password",pschedule_download->password);
		bkp_session_insert(b,"file_size",file_size);
		bkp_session_insert(b,"windowstart1",delay[0]);
		bkp_session_insert(b,"windowend1",delay[1]);
		bkp_session_insert(b,"windowmode1",pschedule_download->timewindowstruct[0].windowmode);
		bkp_session_insert(b,"usermessage1",pschedule_download->timewindowstruct[0].usermessage);
		bkp_session_insert(b,"maxretrie1",maxretrie[0]);
		bkp_session_insert(b,"windowstart2",delay[2]);
		bkp_session_insert(b,"windowend2",delay[3]);
		bkp_session_insert(b,"windowmode2",pschedule_download->timewindowstruct[1].windowmode);
		bkp_session_insert(b,"usermessage2",pschedule_download->timewindowstruct[1].usermessage);
		bkp_session_insert(b,"maxretrie2",maxretrie[1]);
	}
	pthread_mutex_unlock (&mutex_backup_session);
}

void bkp_session_insert_apply_schedule_download(struct apply_schedule_download *papply_schedule_download)
{
	struct search_keywords	keys[9];
	char 					delay[4][128];
	int 					i;
	char 					file_size[128];
	char 					maxretrie[2][128];
	mxml_node_t 			*b;

	pthread_mutex_lock (&mutex_backup_session);
	
	for (i = 0; i<2; i++)
	{
		sprintf(delay[2*i],"%ld",papply_schedule_download->timeintervals[i].windowstart);
		sprintf(delay[2*i+1],"%ld",papply_schedule_download->timeintervals[i].windowend);
		sprintf(maxretrie[i],"%d",papply_schedule_download->timeintervals[i].maxretries);
	}
	keys[0].name = "command_key";
	keys[0].value = papply_schedule_download->command_key;
	keys[1].name = "file_type";
	keys[1].value = papply_schedule_download->file_type;
	keys[2].name = "start_time";
	keys[2].value = papply_schedule_download->start_time;
	keys[3].name = "windowstart1";
	keys[3].value = delay[0];
	keys[4].name = "windowend1";
	keys[4].value = delay[1];
	keys[5].name = "maxretrie1";
	keys[5].value = maxretrie[0];
	keys[6].name = "windowstart2";
	keys[6].value = delay[2];
	keys[7].name = "windowend2";
	keys[7].value = delay[3];
	keys[8].name = "maxretrie2";
	keys[8].value = maxretrie[1];
	b = bkp_session_node_found(bkp_tree, "apply_schedule_download", keys, 9);
	if(!b)
	{
		CWMP_LOG(INFO,"New schedule download key %s file",papply_schedule_download->command_key);
		b = bkp_session_insert(bkp_tree,"apply_schedule_download",NULL);
		bkp_session_insert(b,"start_time",papply_schedule_download->start_time);
		bkp_session_insert(b,"command_key",papply_schedule_download->command_key);
		bkp_session_insert(b,"file_type",papply_schedule_download->file_type);
		bkp_session_insert(b,"windowstart1",delay[0]);
		bkp_session_insert(b,"windowend1",delay[1]);
		bkp_session_insert(b,"maxretrie1",maxretrie[0]);
		
		bkp_session_insert(b,"windowstart2",delay[2]);
		bkp_session_insert(b,"windowend2",delay[3]);
		bkp_session_insert(b,"maxretrie2",maxretrie[1]);
	}
	pthread_mutex_unlock (&mutex_backup_session);
}

void bkp_session_delete_apply_schedule_download(struct apply_schedule_download *papply_schedule_download) //TODO
{
	struct search_keywords	keys[9];
	char 					delay[4][128];
	char schedule_time[128];
	char file_size[128];
	char 					maxretrie[2][128];
	int i;
	mxml_node_t *b;
	
	pthread_mutex_lock (&mutex_backup_session);
	for (i = 0; i<2; i++)
	{
		sprintf(delay[2*i],"%ld",papply_schedule_download->timeintervals[i].windowstart);
		sprintf(delay[2*i+1],"%ld",papply_schedule_download->timeintervals[i].windowend);
		sprintf(maxretrie[i],"%ld",papply_schedule_download->timeintervals[i].maxretries);
	}
	
	keys[0].name = "start_time";
	keys[0].value = papply_schedule_download->start_time;
	keys[1].name = "command_key";
	keys[1].value = papply_schedule_download->command_key;
	keys[2].name = "file_type";
	keys[2].value = papply_schedule_download->file_type;
	keys[3].name = "windowstart1";
	keys[3].value = delay[0];
	keys[4].name = "windowend1";
	keys[4].value = delay[1];
	keys[5].name = "maxretrie1";
	keys[5].value = maxretrie[0];
	keys[6].name = "windowstart2";
	keys[6].value = delay[2];
	keys[7].name = "windowend2";
	keys[7].value = delay[3];
	keys[8].name = "maxretrie2";
	keys[8].value = maxretrie[1];
	b = bkp_session_node_found(bkp_tree, "apply_schedule_download", keys, 9);
	
	if(b)
		mxmlDelete(b);
	pthread_mutex_unlock (&mutex_backup_session);
}

void bkp_session_insert_change_du_state(struct change_du_state *pchange_du_state)
{
	struct search_keywords	keys[7];
	char 					schedule_time[128];
	char 					file_size[128];
	mxml_node_t 			*b, *n, *t;
	int i;
	pthread_mutex_lock (&mutex_backup_session);	
	
	sprintf(schedule_time,"%ld",pchange_du_state->timeout);
	b = bkp_session_insert(bkp_tree,"change_du_state",NULL);
	bkp_session_insert(b,"command_key",pchange_du_state->command_key);		
	bkp_session_insert(b,"time",schedule_time);
	struct operations *p;
  	list_for_each_entry(p, &(pchange_du_state->list_operation), list) {
		if (p->type == DU_INSTALL ) {
			n =	bkp_session_insert(b,"install",NULL);
			bkp_session_insert(n,"url",p->url);
			bkp_session_insert(n,"uuid",p->uuid);
			bkp_session_insert(n,"username",p->username);
			bkp_session_insert(n,"password",p->password);				
			bkp_session_insert(n,"executionenvref",p->executionenvref);				
		}
		else if (p->type == DU_UNINSTALL) {
			n =	bkp_session_insert(b,"uninstall",NULL);				
			bkp_session_insert(n,"uuid",p->uuid);
			bkp_session_insert(n,"version",p->version);
			bkp_session_insert(n,"executionenvref",p->executionenvref);
		}
		else if (p->type == DU_UPDATE) {
			n =	bkp_session_insert(b,"upgrade",NULL);
			bkp_session_insert(n,"url",p->url);
			bkp_session_insert(n,"uuid",p->uuid);
			bkp_session_insert(n,"username",p->username);
			bkp_session_insert(n,"password",p->password);				
			bkp_session_insert(n,"version",p->version);
		}
	}
	pthread_mutex_unlock (&mutex_backup_session);
}

void bkp_session_delete_change_du_state(struct change_du_state *pchange_du_state) 
{
	struct search_keywords keys[2];
	char 					schedule_time[128];
	mxml_node_t *b;

	pthread_mutex_lock (&mutex_backup_session);	
	keys[0].name = "command_key";
	keys[0].value = pchange_du_state->command_key;
	keys[1].name = "time";
	sprintf(schedule_time,"%ld",pchange_du_state->timeout);
	keys[1].value = schedule_time;
	b = bkp_session_node_found(bkp_tree, "change_du_state", keys, 2);
	if(b)
		mxmlDelete(b);
	pthread_mutex_unlock (&mutex_backup_session);
}

void bkp_session_insert_upload(struct upload *pupload)
{
	struct search_keywords	keys[6];
	char 					schedule_time[128];
	mxml_node_t 			*b;

	pthread_mutex_lock (&mutex_backup_session);
	sprintf(schedule_time,"%ld",pupload->scheduled_time);
	keys[0].name = "url";
	keys[0].value = pupload->url;
	keys[1].name = "command_key";
	keys[1].value = pupload->command_key;
	keys[2].name = "file_type";
	keys[2].value = pupload->file_type;
	keys[3].name = "username";
	keys[3].value = pupload->username;
	keys[4].name = "password";
	keys[4].value = pupload->password;
	keys[5].name = "time";
	keys[5].value = schedule_time;
	b = bkp_session_node_found(bkp_tree, "upload", keys, 6);
	if(!b)
	{
		b = bkp_session_insert(bkp_tree,"upload",NULL);
		bkp_session_insert(b,"url",pupload->url);
		bkp_session_insert(b,"command_key",pupload->command_key);
		bkp_session_insert(b,"file_type",pupload->file_type);
		bkp_session_insert(b,"username",pupload->username);
		bkp_session_insert(b,"password",pupload->password);
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

void bkp_session_delete_schedule_download(struct schedule_download *pschedule_download) //TODO
{
	struct search_keywords	keys[16];
	char 					delay[4][128];
	char schedule_time[128];
	char file_size[128];
	char 					maxretrie[2][128];
	int i;
	mxml_node_t *b;

	pthread_mutex_lock (&mutex_backup_session);
	sprintf(file_size,"%d",pschedule_download->file_size);
	for (i = 0; i<2; i++)
	{
		sprintf(delay[2*i],"%ld",pschedule_download->timewindowstruct[i].windowstart);
		sprintf(delay[2*i+1],"%ld",pschedule_download->timewindowstruct[i].windowend);
		sprintf(maxretrie[i],"%ld",pschedule_download->timewindowstruct[i].maxretries);
	}
	keys[0].name = "url";
	keys[0].value = pschedule_download->url;
	keys[1].name = "command_key";
	keys[1].value = pschedule_download->command_key;
	keys[2].name = "file_type";
	keys[2].value = pschedule_download->file_type;
	keys[3].name = "username";
	keys[3].value = pschedule_download->username;
	keys[4].name = "password";
	keys[4].value = pschedule_download->password;
	keys[5].name = "file_size";
	keys[5].value = file_size;
	keys[6].name = "windowstart1";
	keys[6].value = delay[0];
	keys[7].name = "windowend1";
	keys[7].value = delay[1];
	keys[8].name = "windowmode1";
	keys[8].value = pschedule_download->timewindowstruct[0].windowmode;
	keys[9].name = "usermessage1";
	keys[9].value = pschedule_download->timewindowstruct[0].usermessage;
	keys[10].name = "maxretrie1";
	keys[10].value = maxretrie[0];
	keys[11].name = "windowstart2";
	keys[11].value = delay[2];
	keys[12].name = "windowend2";
	keys[12].value = delay[3];
	keys[13].name = "windowmode2";
	keys[13].value = pschedule_download->timewindowstruct[1].windowmode;
	keys[14].name = "usermessage2";
	keys[14].value = pschedule_download->timewindowstruct[1].usermessage;
	keys[15].name = "maxretrie2";
	keys[15].value = maxretrie[1];
	b = bkp_session_node_found(bkp_tree, "schedule_download", keys, 16);
	
	if(b) 
		mxmlDelete(b);
	pthread_mutex_unlock (&mutex_backup_session);
}
void bkp_session_delete_upload(struct upload *pupload)
{
	struct search_keywords keys[6];
	char schedule_time[128];
	mxml_node_t *b;

	pthread_mutex_lock (&mutex_backup_session);
	sprintf(schedule_time,"%ld",pupload->scheduled_time);
	keys[0].name = "url";
	keys[0].value = pupload->url;
	keys[1].name = "command_key";
	keys[1].value = pupload->command_key;
	keys[2].name = "file_type";
	keys[2].value = pupload->file_type;
	keys[3].name = "username";
	keys[3].value = pupload->username;
	keys[4].name = "password";
	keys[4].value = pupload->password;
	keys[5].name = "time";
	keys[5].value = schedule_time;
	b = bkp_session_node_found(bkp_tree, "upload", keys, 6);
	if(b)
		mxmlDelete(b);
	pthread_mutex_unlock (&mutex_backup_session);
}

void bkp_session_insert_du_state_change_complete(struct du_state_change_complete *pdu_state_change_complete) {
	char 					schedule_time[128];
	char 					resolved[8];
	char 					fault_code[8];
	mxml_node_t 			*b, *n, *t;
	int i;
	pthread_mutex_lock (&mutex_backup_session);	
	sprintf(schedule_time,"%ld",pdu_state_change_complete->timeout);
	
	b = bkp_session_insert(bkp_tree,"du_state_change_complete",NULL);
	bkp_session_insert(b,"command_key",pdu_state_change_complete->command_key);		
	bkp_session_insert(b,"time",schedule_time);
	struct opresult *p;
	list_for_each_entry(p, &(pdu_state_change_complete->list_opresult), list) {			
		n =	bkp_session_insert(b,"opresult",NULL);
		sprintf(resolved,"%b",p->resolved);
		sprintf(fault_code,"%d",p->fault);
		bkp_session_insert(n,"uuid",p->uuid);
		bkp_session_insert(n,"du_ref",p->du_ref);
		bkp_session_insert(n,"version",p->version);				
		bkp_session_insert(n,"current_state",p->current_state);
		bkp_session_insert(n,"resolved",resolved);
		bkp_session_insert(n,"execution_unit_ref",p->execution_unit_ref);
		bkp_session_insert(n,"start_time",p->start_time);
		bkp_session_insert(n,"complete_time",p->complete_time);	
		bkp_session_insert(n,"fault",fault_code);		
	}		
	pthread_mutex_unlock (&mutex_backup_session);
}

void bkp_session_delete_du_state_change_complete(struct du_state_change_complete *pdu_state_change_complete) {
	struct search_keywords	keys[2];
	mxml_node_t 			*b;
	char 					schedule_time[128];
	
	pthread_mutex_lock (&mutex_backup_session);	
	keys[0].name = "command_key";
	keys[0].value = pdu_state_change_complete->command_key;
	sprintf(schedule_time,"%ld",pdu_state_change_complete->timeout);

	keys[1].name = "time";
	keys[1].value = schedule_time;
	b = bkp_session_node_found(bkp_tree, "du_state_change_complete", keys, 2);
	if(b)
		mxmlDelete(b);
	pthread_mutex_unlock (&mutex_backup_session);
}
void bkp_session_insert_transfer_complete(struct transfer_complete *ptransfer_complete)
{
	struct search_keywords	keys[5];
	char					fault_code[16];
	char					type[16];
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
	keys[4].name = "type";
	sprintf(type,"%d",ptransfer_complete->type);
	keys[4].value = type;
	b = bkp_session_node_found(bkp_tree, "transfer_complete", keys, 5);
	if(!b)
	{
		b = bkp_session_insert(bkp_tree,"transfer_complete",NULL);
		bkp_session_insert(b,"command_key",ptransfer_complete->command_key);
		bkp_session_insert(b,"start_time",ptransfer_complete->start_time);
		bkp_session_insert(b,"complete_time",ptransfer_complete->complete_time);
		bkp_session_insert(b,"old_software_version",ptransfer_complete->old_software_version);
		bkp_session_insert(b,"fault_code",fault_code);
		bkp_session_insert(b,"type",type);
	}
	pthread_mutex_unlock (&mutex_backup_session);
}

void bkp_session_delete_transfer_complete(struct transfer_complete *ptransfer_complete)
{
	struct search_keywords	keys[5];
	char					fault_code[16];
	char					type[16];
	mxml_node_t 			*b;

	pthread_mutex_lock (&mutex_backup_session);
	sprintf(fault_code,"%d",ptransfer_complete->fault_code);
	sprintf(type,"%d",ptransfer_complete->type);
	keys[0].name = "command_key";
	keys[0].value = ptransfer_complete->command_key;
	keys[1].name = "start_time";
	keys[1].value = ptransfer_complete->start_time;
	keys[2].name = "complete_time";
	keys[2].value = ptransfer_complete->complete_time;
	keys[3].name = "fault_code";
	keys[3].value = fault_code;
	keys[4].name = "type";
	keys[4].value = type;
	b = bkp_session_node_found(bkp_tree, "transfer_complete", keys, 5);
	if(b)
		mxmlDelete(b);
	pthread_mutex_unlock (&mutex_backup_session);
}
/*
 * Load backup session
 */
char *load_child_value(mxml_node_t *tree, char *sub_name)
{
	char		*value = NULL;
	mxml_node_t	*b = tree;

	if (b) {
		b = mxmlFindElement(b, b, sub_name, NULL, NULL, MXML_DESCEND);
		if (b) {
			b = mxmlWalkNext(b, tree, MXML_DESCEND);
			if (b && b->type == MXML_TEXT)
			{
				if(b->value.text.string != NULL)
				{
					value = strdup(b->value.text.string);
				}
			}
		}
	}
	return value;
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
					if (EVENT_CONST[idx].RETRY & EVENT_RETRY_AFTER_REBOOT)
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
							add_dm_parameter_tolist(&(event_container_save->head_dm_parameter),
									c->value.text.string, NULL, NULL);
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
				if (c && c->type == MXML_TEXT)
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
				if (c && c->type == MXML_TEXT)
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
				if (c && c->type == MXML_TEXT)
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
				if (c && c->type == MXML_TEXT)
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
				if (c && c->type == MXML_TEXT)
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
				if (c && c->type == MXML_TEXT)
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
				if (c && c->type == MXML_TEXT)
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
				if (c && c->type == MXML_TEXT)
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
				if (c && c->type == MXML_TEXT)
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

void load_schedule_download(mxml_node_t *tree,struct cwmp *cwmp)
{
	mxml_node_t			*b = tree, *c;
	struct schedule_download		*download_request = NULL;
    struct list_head	*ilist = NULL;
    struct schedule_download		*idownload_request = NULL;

	download_request = calloc(1,sizeof(struct schedule_download));

	b = mxmlWalkNext(b, tree, MXML_DESCEND);

	while (b) {
		if (b && b->type == MXML_ELEMENT) {
			if (strcmp(b->value.element.name, "url") == 0)
			{
				c = mxmlWalkNext(b, b, MXML_DESCEND);
				if (c && c->type == MXML_TEXT)
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
				if (c && c->type == MXML_TEXT)
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
				if (c && c->type == MXML_TEXT)
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
				if (c && c->type == MXML_TEXT)
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
				if (c && c->type == MXML_TEXT)
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
				if (c && c->type == MXML_TEXT)
				{
					if(c->value.text.string != NULL)
					{
						download_request->file_size = atoi(c->value.text.string);
					}
				}
			}
			else if (strcmp(b->value.element.name, "windowstart1") == 0)
			{
				c = mxmlWalkNext(b, b, MXML_DESCEND);
				if (c && c->type == MXML_TEXT)
				{
					if(c->value.text.string != NULL)
					{
						download_request->timewindowstruct[0].windowstart = atol(c->value.text.string);
					}
				}
			}
			else if (strcmp(b->value.element.name, "windowend1") == 0)
			{
				c = mxmlWalkNext(b, b, MXML_DESCEND);
				if (c && c->type == MXML_TEXT)
				{
					if(c->value.text.string != NULL)
					{
						download_request->timewindowstruct[0].windowend = atol(c->value.text.string);
					}
				}
			}
			else if (strcmp(b->value.element.name, "windowmode1") == 0)
			{
				c = mxmlWalkNext(b, b, MXML_DESCEND);
				if (c && c->type == MXML_TEXT)
				{
					if(c->value.text.string != NULL)
					{
						download_request->timewindowstruct[0].windowmode = strdup(c->value.text.string);
					}
				}
			}
			else if (strcmp(b->value.element.name, "usermessage1") == 0)
			{
				c = mxmlWalkNext(b, b, MXML_DESCEND);
				if (c && c->type == MXML_TEXT)
				{
					if(c->value.text.string != NULL)
					{
						download_request->timewindowstruct[0].usermessage = strdup(c->value.text.string);
					}
				}
			}
			else if (strcmp(b->value.element.name, "maxretrie1") == 0)
			{
				c = mxmlWalkNext(b, b, MXML_DESCEND);
				if (c && c->type == MXML_TEXT)
				{
					if(c->value.text.string != NULL)
					{
						download_request->timewindowstruct[0].maxretries = atoi(c->value.text.string);
					}
				}
			}
			else if (strcmp(b->value.element.name, "windowstart2") == 0)
			{
				c = mxmlWalkNext(b, b, MXML_DESCEND);
				if (c && c->type == MXML_TEXT)
				{
					if(c->value.text.string != NULL)
					{
						download_request->timewindowstruct[1].windowstart = atol(c->value.text.string);
					}
				}
			}
			else if (strcmp(b->value.element.name, "windowend2") == 0)
			{
				c = mxmlWalkNext(b, b, MXML_DESCEND);
				if (c && c->type == MXML_TEXT)
				{
					if(c->value.text.string != NULL)
					{
						download_request->timewindowstruct[1].windowend = atol(c->value.text.string);
					}
				}
			}
			else if (strcmp(b->value.element.name, "windowmode2") == 0)
			{
				c = mxmlWalkNext(b, b, MXML_DESCEND);
				if (c && c->type == MXML_TEXT)
				{
					if(c->value.text.string != NULL)
					{
						download_request->timewindowstruct[1].windowmode = strdup(c->value.text.string);
					}
				}
			}
			else if (strcmp(b->value.element.name, "usermessage2") == 0)
			{
				c = mxmlWalkNext(b, b, MXML_DESCEND);
				if (c && c->type == MXML_TEXT)
				{
					if(c->value.text.string != NULL)
					{
						download_request->timewindowstruct[1].usermessage = strdup(c->value.text.string);
					}
				}
			}
			else if (strcmp(b->value.element.name, "maxretrie2") == 0)
			{
				c = mxmlWalkNext(b, b, MXML_DESCEND);
				if (c && c->type == MXML_TEXT)
				{
					if(c->value.text.string != NULL)
					{
						download_request->timewindowstruct[1].maxretries = atoi(c->value.text.string);
					}
				}
			}
		}
		b = mxmlWalkNext(b, tree, MXML_NO_DESCEND);
	}
	list_for_each(ilist,&(list_schedule_download))
	{
		idownload_request = list_entry(ilist,struct schedule_download,list);
		if (idownload_request->timewindowstruct[0].windowstart > download_request->timewindowstruct[0].windowstart)
		{
			break;
		}
	}
	list_add (&(download_request->list), ilist->prev);
	if(download_request->timewindowstruct[0].windowstart != 0)
		count_download_queue++;
}


void load_apply_schedule_download(mxml_node_t *tree,struct cwmp *cwmp)
{
	mxml_node_t			*b = tree, *c;
	struct apply_schedule_download		*download_request = NULL;
    struct list_head	*ilist = NULL;
    struct apply_schedule_download		*idownload_request = NULL;

	download_request = calloc(1,sizeof(struct apply_schedule_download));

	b = mxmlWalkNext(b, tree, MXML_DESCEND);

	while (b) {
		if (b && b->type == MXML_ELEMENT) {
			if (strcmp(b->value.element.name, "command_key") == 0)
			{
				c = mxmlWalkNext(b, b, MXML_DESCEND);
				if (c && c->type == MXML_TEXT)
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
				if (c && c->type == MXML_TEXT)
				{
					if(c->value.text.string != NULL)
					{
						download_request->file_type = strdup(c->value.text.string);
					}
				}
			}
			else if (strcmp(b->value.element.name, "start_time") == 0)
			{
				c = mxmlWalkNext(b, b, MXML_DESCEND);
				if (c && c->type == MXML_TEXT)
				{
					if(c->value.text.string != NULL)
					{
						download_request->start_time = strdup(c->value.text.string);
					}
				}
			}			
			else if (strcmp(b->value.element.name, "windowstart1") == 0)
			{
				c = mxmlWalkNext(b, b, MXML_DESCEND);
				if (c && c->type == MXML_TEXT)
				{
					if(c->value.text.string != NULL)
					{
						download_request->timeintervals[0].windowstart = atol(c->value.text.string);
					}
				}
			}
			else if (strcmp(b->value.element.name, "windowend1") == 0)
			{
				c = mxmlWalkNext(b, b, MXML_DESCEND);
				if (c && c->type == MXML_TEXT)
				{
					if(c->value.text.string != NULL)
					{
						download_request->timeintervals[0].windowend = atol(c->value.text.string);
					}
				}
			}			
			else if (strcmp(b->value.element.name, "maxretrie1") == 0)
			{
				c = mxmlWalkNext(b, b, MXML_DESCEND);
				if (c && c->type == MXML_TEXT)
				{
					if(c->value.text.string != NULL)
					{
						download_request->timeintervals[0].maxretries = atoi(c->value.text.string);
					}
				}
			}
			else if (strcmp(b->value.element.name, "windowstart2") == 0)
			{
				c = mxmlWalkNext(b, b, MXML_DESCEND);
				if (c && c->type == MXML_TEXT)
				{
					if(c->value.text.string != NULL)
					{
						download_request->timeintervals[1].windowstart = atol(c->value.text.string);
					}
				}
			}
			else if (strcmp(b->value.element.name, "windowend2") == 0)
			{
				c = mxmlWalkNext(b, b, MXML_DESCEND);
				if (c && c->type == MXML_TEXT)
				{
					if(c->value.text.string != NULL)
					{
						download_request->timeintervals[1].windowend = atol(c->value.text.string);
					}
				}
			}
			else if (strcmp(b->value.element.name, "maxretrie2") == 0)
			{
				c = mxmlWalkNext(b, b, MXML_DESCEND);
				if (c && c->type == MXML_TEXT)
				{
					if(c->value.text.string != NULL)
					{
						download_request->timeintervals[1].maxretries = atoi(c->value.text.string);
					}
				}
			}
			/*
			windowstart1",delay[0]);
			bkp_session_insert(b,"windowend1",delay[1]);
			bkp_session_insert(b,"windowmode1",pschedule_download->timewindowstruct[0].windowmode);
			bkp_session_insert(b,"usermessage1",pschedule_download->timewindowstruct[0].usermessage);
			bkp_session_insert(b,"maxretrie1*/

		}
		b = mxmlWalkNext(b, tree, MXML_NO_DESCEND);
	}
	list_add_tail (&(download_request->list), &(list_apply_schedule_download));
	if(download_request->timeintervals[0].windowstart != 0)
		count_download_queue++;
}
void load_upload(mxml_node_t *tree,struct cwmp *cwmp)
{
	mxml_node_t			*b = tree, *c;
	struct upload		*upload_request = NULL;
    struct list_head	*ilist = NULL;
    struct upload		*iupload_request = NULL;

	upload_request = calloc(1,sizeof(struct upload));

	b = mxmlWalkNext(b, tree, MXML_DESCEND);

	while (b) {
		if (b && b->type == MXML_ELEMENT) {
			if (strcmp(b->value.element.name, "url") == 0)
			{
				c = mxmlWalkNext(b, b, MXML_DESCEND);
				if (c && c->type == MXML_TEXT)
				{
					if(c->value.text.string != NULL)
					{
						upload_request->url = strdup(c->value.text.string);
					}
				}
			}
			else if (strcmp(b->value.element.name, "command_key") == 0)
			{
				c = mxmlWalkNext(b, b, MXML_DESCEND);
				if (c && c->type == MXML_TEXT)
				{
					if(c->value.text.string != NULL)
					{
						upload_request->command_key = strdup(c->value.text.string);
					}
				}
			}
			else if (strcmp(b->value.element.name, "file_type") == 0)
			{
				c = mxmlWalkNext(b, b, MXML_DESCEND);
				if (c && c->type == MXML_TEXT)
				{
					if(c->value.text.string != NULL)
					{
						upload_request->file_type = strdup(c->value.text.string);
					}
				}
			}
			else if (strcmp(b->value.element.name, "username") == 0)
			{
				c = mxmlWalkNext(b, b, MXML_DESCEND);
				if (c && c->type == MXML_TEXT)
				{
					if(c->value.text.string != NULL)
					{
						upload_request->username = strdup(c->value.text.string);
					}
				}
			}
			else if (strcmp(b->value.element.name, "password") == 0)
			{
				c = mxmlWalkNext(b, b, MXML_DESCEND);
				if (c && c->type == MXML_TEXT)
				{
					if(c->value.text.string != NULL)
					{
						upload_request->password = strdup(c->value.text.string);
					}
				}
			}			
			else if (strcmp(b->value.element.name, "time") == 0)
			{
				c = mxmlWalkNext(b, b, MXML_DESCEND);
				if (c && c->type == MXML_TEXT)
				{
					if(c->value.text.string != NULL)
					{
						upload_request->scheduled_time = atol(c->value.text.string);
					}
				}
			}
		}
		b = mxmlWalkNext(b, tree, MXML_NO_DESCEND);
	}
	list_for_each(ilist,&(list_upload))
	{
		iupload_request = list_entry(ilist,struct upload,list);
		if (iupload_request->scheduled_time > upload_request->scheduled_time)
		{
			break;
		}
	}
	list_add (&(upload_request->list), ilist->prev);
	if(upload_request->scheduled_time != 0)
		count_download_queue++;
}

void load_change_du_state(mxml_node_t *tree,struct cwmp *cwmp)
{
	mxml_node_t			*b = tree, *c, *d;
	struct change_du_state		*change_du_state_request = NULL;
    struct list_head	*ilist = NULL;
    struct change_du_state		*ichange_du_state_request = NULL;
	struct operations		*elem;

	change_du_state_request = calloc(1,sizeof(struct change_du_state));
	INIT_LIST_HEAD(&(change_du_state_request->list_operation));
	b = mxmlWalkNext(b, tree, MXML_DESCEND);

	while (b) {
		if (b && b->type == MXML_ELEMENT) {
			if (strcmp(b->value.element.name, "command_key") == 0)
			{
				c = mxmlWalkNext(b, b, MXML_DESCEND);
				if (c && c->type == MXML_TEXT)
				{
					if(c->value.text.string != NULL)
					{
						change_du_state_request->command_key = strdup(c->value.text.string);
					}
				}
			}
			else if (strcmp(b->value.element.name, "time") == 0)
			{
				c = mxmlWalkNext(b, b, MXML_DESCEND);
				if (c && c->type == MXML_TEXT)
				{
					if(c->value.text.string != NULL)
					{
						change_du_state_request->timeout = atol(c->value.text.string);
					}
				}
			}
			else if (strcmp(b->value.element.name, "upgrade") == 0)
			{
				elem = (operations*)calloc(1, sizeof(operations));
				elem->type = DU_UPDATE;
				list_add_tail(&(elem->list), &(change_du_state_request->list_operation));
				c = mxmlWalkNext(b, b, MXML_DESCEND);
				while(c) {
					//
					if (c && c->type == MXML_ELEMENT) {
						if (strcmp(c->value.element.name, "uuid") == 0)
						{
							d = mxmlWalkNext(c, c, MXML_DESCEND);
							if (d && d->type == MXML_TEXT)
							{
								if(d->value.text.string != NULL)
								{
									elem->uuid = strdup(d->value.text.string);
								}
							}
						}
					}					
					if (c && c->type == MXML_ELEMENT) {
						if (strcmp(c->value.element.name, "version") == 0)
						{
							d = mxmlWalkNext(c, c, MXML_DESCEND);
							if (d && d->type == MXML_TEXT)
							{
								if(d->value.text.string != NULL)
								{
									elem->version = strdup(d->value.text.string);
								}
							}
						}
					}
					if (c && c->type == MXML_ELEMENT) {
						if (strcmp(c->value.element.name, "url") == 0)
						{
							d = mxmlWalkNext(c, c, MXML_DESCEND);
							if (d && d->type == MXML_TEXT)
							{
								if(d->value.text.string != NULL)
								{
									elem->url = strdup(d->value.text.string);
								}
							}
						}
					}
					if (c && c->type == MXML_ELEMENT) {
						if (strcmp(c->value.element.name, "username") == 0)
						{
							d = mxmlWalkNext(c, c, MXML_DESCEND);
							if (d && d->type == MXML_TEXT)
							{
								if(d->value.text.string != NULL)
								{
									elem->username = strdup(d->value.text.string);
								}
							}
						}
					}
					if (c && c->type == MXML_ELEMENT) {
						if (strcmp(c->value.element.name, "password") == 0)
						{
							d = mxmlWalkNext(c, c, MXML_DESCEND);
							if (d && d->type == MXML_TEXT)
							{
								if(d->value.text.string != NULL)
								{
									elem->password = strdup(d->value.text.string);
								}
							}
						}
					}
					c = mxmlWalkNext(c, b, MXML_NO_DESCEND);
				}				
			}
			else if (strcmp(b->value.element.name, "install") == 0)
			{
				elem = (operations*)calloc(1, sizeof(operations));
				elem->type = DU_INSTALL;
				list_add_tail(&(elem->list), &(change_du_state_request->list_operation));
				c = mxmlWalkNext(b, b, MXML_DESCEND);
				while(c) {
					if (c && c->type == MXML_ELEMENT) {
						if (strcmp(c->value.element.name, "uuid") == 0)
						{
							d = mxmlWalkNext(c, c, MXML_DESCEND);
							if (d && d->type == MXML_TEXT)
							{
								if(d->value.text.string != NULL)
								{
									elem->uuid = strdup(d->value.text.string);
								}
							}
						}
					}					
					if (c && c->type == MXML_ELEMENT) {
						if (strcmp(c->value.element.name, "executionenvref") == 0)
						{
							d = mxmlWalkNext(c, c, MXML_DESCEND);
							if (d && d->type == MXML_TEXT)
							{
								if(d->value.text.string != NULL)
								{
									elem->executionenvref = strdup(d->value.text.string);
								}
							}
						}
					}
					if (c && c->type == MXML_ELEMENT) {
						if (strcmp(c->value.element.name, "url") == 0)
						{
							d = mxmlWalkNext(c, c, MXML_DESCEND);
							if (d && d->type == MXML_TEXT)
							{
								if(d->value.text.string != NULL)
								{
									elem->url = strdup(d->value.text.string);
								}
							}
						}
					}
					if (c && c->type == MXML_ELEMENT) {
						if (strcmp(c->value.element.name, "username") == 0)
						{
							d = mxmlWalkNext(c, c, MXML_DESCEND);
							if (d && d->type == MXML_TEXT)
							{
								if(d->value.text.string != NULL)
								{
									elem->username = strdup(d->value.text.string);
								}
							}
						}
					}
					if (c && c->type == MXML_ELEMENT) {
						if (strcmp(c->value.element.name, "password") == 0)
						{
							d = mxmlWalkNext(c, c, MXML_DESCEND);
							if (d && d->type == MXML_TEXT)
							{
								if(d->value.text.string != NULL)
								{
									elem->password = strdup(d->value.text.string);
								}
							}
						}
					}
					c = mxmlWalkNext(c, b, MXML_NO_DESCEND);
				}
			}
			else if (strcmp(b->value.element.name, "uninstall") == 0)
			{
				elem = (operations*)calloc(1, sizeof(operations));
				elem->type = DU_UNINSTALL;
				list_add_tail(&(elem->list), &(change_du_state_request->list_operation));
				c = mxmlWalkNext(b, b, MXML_DESCEND);
				while(c) {
					if (c && c->type == MXML_ELEMENT) {
						if (strcmp(c->value.element.name, "uuid") == 0)
						{
							d = mxmlWalkNext(c, c, MXML_DESCEND);
							if (d && d->type == MXML_TEXT)
							{
								if(d->value.text.string != NULL)
								{
									elem->uuid = strdup(d->value.text.string);
								}
							}
						}
					}					
					if (c && c->type == MXML_ELEMENT) {
						if (strcmp(c->value.element.name, "executionenvref") == 0)
						{
							d = mxmlWalkNext(c, c, MXML_DESCEND);
							if (d && d->type == MXML_TEXT)
							{
								if(d->value.text.string != NULL)
								{
									elem->executionenvref = strdup(d->value.text.string);
								}
							}
						}
					}
					if (c && c->type == MXML_ELEMENT) {
						if (strcmp(c->value.element.name, "version") == 0)
						{
							d = mxmlWalkNext(c, c, MXML_DESCEND);
							if (d && d->type == MXML_TEXT)
							{
								if(d->value.text.string != NULL)
								{
									elem->version = strdup(d->value.text.string);
								}
							}
						}
					}
					c = mxmlWalkNext(c, b, MXML_NO_DESCEND);
				}
			}			
		}
		b = mxmlWalkNext(b, tree, MXML_NO_DESCEND);
	}
	list_add_tail (&(change_du_state_request->list_operation), &(list_change_du_state));
}

void load_du_state_change_complete (mxml_node_t	*tree,struct cwmp *cwmp)
{
	mxml_node_t			*b = tree, *c, *d;
	struct du_state_change_complete		*du_state_change_complete_request = NULL;
    struct list_head	*ilist = NULL;
    struct change_du_state		*idu_state_change_complete_request = NULL;
	struct opresult		*elem;

	du_state_change_complete_request = calloc(1,sizeof(struct du_state_change_complete));
	INIT_LIST_HEAD(&(du_state_change_complete_request->list_opresult));
	b = mxmlWalkNext(b, tree, MXML_DESCEND);

	while (b) {
		if (b && b->type == MXML_ELEMENT) {
			if (strcmp(b->value.element.name, "command_key") == 0)
			{
				c = mxmlWalkNext(b, b, MXML_DESCEND);
				if (c && c->type == MXML_TEXT)
				{
					if(c->value.text.string != NULL)
					{
						du_state_change_complete_request->command_key = strdup(c->value.text.string);
					}
				}
			}
			else if (strcmp(b->value.element.name, "time") == 0)
			{
				c = mxmlWalkNext(b, b, MXML_DESCEND);
				if (c && c->type == MXML_TEXT)
				{
					if(c->value.text.string != NULL)
					{
						du_state_change_complete_request->timeout = atol(c->value.text.string);
					}
				}
			}
			else if (strcmp(b->value.element.name, "opresult") == 0)
			{
				elem = (opresult*)calloc(1, sizeof(opresult));
				list_add_tail(&(elem->list), &(du_state_change_complete_request->list_opresult));
				c = mxmlWalkNext(b, b, MXML_DESCEND);
				while(c) {
					if (c && c->type == MXML_ELEMENT) {
						if (strcmp(c->value.element.name, "uuid") == 0)
						{
							d = mxmlWalkNext(c, c, MXML_DESCEND);
							if (d && d->type == MXML_TEXT)
							{
								if(d->value.text.string != NULL)
								{
									elem->uuid = strdup(d->value.text.string);
								}
							}
						}
					}					
					if (c && c->type == MXML_ELEMENT) {
						if (strcmp(c->value.element.name, "version") == 0)
						{
							d = mxmlWalkNext(c, c, MXML_DESCEND);
							if (d && d->type == MXML_TEXT)
							{
								if(d->value.text.string != NULL)
								{
									elem->version = strdup(d->value.text.string);
								}
							}
						}
					}
					if (c && c->type == MXML_ELEMENT) {
						if (strcmp(c->value.element.name, "du_ref") == 0)
						{
							d = mxmlWalkNext(c, c, MXML_DESCEND);
							if (d && d->type == MXML_TEXT)
							{
								if(d->value.text.string != NULL)
								{
									elem->du_ref = strdup(d->value.text.string);
								}
							}
						}
					}
					if (c && c->type == MXML_ELEMENT) {
						if (strcmp(c->value.element.name, "current_state") == 0)
						{
							d = mxmlWalkNext(c, c, MXML_DESCEND);
							if (d && d->type == MXML_TEXT)
							{
								if(d->value.text.string != NULL)
								{
									elem->current_state = strdup(d->value.text.string);
								}
							}
						}
					}
					if (c && c->type == MXML_ELEMENT) {
						if (strcmp(c->value.element.name, "resolved") == 0)
						{
							d = mxmlWalkNext(c, c, MXML_DESCEND);
							if (d && d->type == MXML_TEXT)
							{
								if(d->value.text.string != NULL)
								{
									elem->resolved = d->value.text.string;
								}
							}
						}
					}
					if (c && c->type == MXML_ELEMENT) {
						if (strcmp(c->value.element.name, "start_time") == 0)
						{
							d = mxmlWalkNext(c, c, MXML_DESCEND);
							if (d && d->type == MXML_TEXT)
							{
								if(d->value.text.string != NULL)
								{
									elem->start_time = atol(d->value.text.string);
								}
							}
						}
					}
					if (c && c->type == MXML_ELEMENT) {
						if (strcmp(c->value.element.name, "complete_time") == 0)
						{
							d = mxmlWalkNext(c, c, MXML_DESCEND);
							if (d && d->type == MXML_TEXT)
							{
								if(d->value.text.string != NULL)
								{
									elem->complete_time = atol(d->value.text.string);
								}
							}
						}
					}
					if (c && c->type == MXML_ELEMENT) {
						if (strcmp(c->value.element.name, "fault") == 0)
						{
							d = mxmlWalkNext(c, c, MXML_DESCEND);
							if (d && d->type == MXML_TEXT)
							{
								if(d->value.text.string != NULL)
								{
									elem->fault = atoi(d->value.text.string);
								}
							}
						}
					}
					if (c && c->type == MXML_ELEMENT) {
						if (strcmp(c->value.element.name, "execution_unit_ref") == 0)
						{
							d = mxmlWalkNext(c, c, MXML_DESCEND);
							if (d && d->type == MXML_TEXT)
							{
								if(d->value.text.string != NULL)
								{
									elem->execution_unit_ref = strdup(d->value.text.string);
								}
							}
						}
					}
					c = mxmlWalkNext(c, b, MXML_NO_DESCEND);
				}				
			}			
		}
		b = mxmlWalkNext(b, tree, MXML_NO_DESCEND);
	}
	cwmp_root_cause_dustatechangeComplete (cwmp, du_state_change_complete_request);	
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
				if (c && c->type == MXML_TEXT)
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
				if (c && c->type == MXML_TEXT)
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
				if (c && c->type == MXML_TEXT)
				{
					if(c->value.text.string != NULL)
					{
						ptransfer_complete->complete_time = strdup(c->value.text.string);
					}
				}
			}
			else if (strcmp(b->value.element.name,"old_software_version") == 0)
			{
				c = mxmlWalkNext(b, b, MXML_DESCEND);
				if (c && c->type == MXML_TEXT)
				{
					if(c->value.text.string != NULL)
					{
						ptransfer_complete->old_software_version = strdup(c->value.text.string);
					}
				}
			}
			else if (strcmp(b->value.element.name,"fault_code") == 0)
			{
				c = mxmlWalkNext(b, b, MXML_DESCEND);
				if (c && c->type == MXML_TEXT)
				{
					if(c->value.text.string != NULL)
					{
						ptransfer_complete->fault_code = atoi(c->value.text.string);
					}
				}
			}
			else if (strcmp(b->value.element.name,"type") == 0)
			{
				c = mxmlWalkNext(b, b, MXML_DESCEND);
				if (c && c->type == MXML_TEXT)
				{
					if(c->value.text.string != NULL)
					{
						ptransfer_complete->type = atoi(c->value.text.string);
					}
				}
			}
		}
		b = mxmlWalkNext(b, tree, MXML_NO_DESCEND);
	}
	cwmp_root_cause_TransferComplete (cwmp, ptransfer_complete);
	sotfware_version_value_change(cwmp, ptransfer_complete);
}

void bkp_session_create_file()
{
	FILE		*pFile;

	pthread_mutex_lock (&mutex_backup_session);
	pFile = fopen(CWMP_BKP_FILE,"w");
	if(pFile == NULL)
	{
		CWMP_LOG(ERROR,"Unable to create %s file",CWMP_BKP_FILE);
		pthread_mutex_unlock (&mutex_backup_session);
		return;
	}
	fprintf(pFile,"%s",CWMP_BACKUP_SESSION);
	bkp_tree = mxmlLoadString(NULL, CWMP_BACKUP_SESSION, MXML_NO_CALLBACK);
	fclose(pFile);
	pthread_mutex_unlock (&mutex_backup_session);
}

int bkp_session_check_file()
{
	FILE		*pFile;

	if(access(CWMP_BKP_FILE, F_OK) == -1)
	{
		bkp_session_create_file();
		return -1;
	}

	if(bkp_tree == NULL)
	{
		pFile = fopen(CWMP_BKP_FILE, "r");
		bkp_tree = mxmlLoadFile(NULL, pFile, MXML_NO_CALLBACK);
		fclose(pFile);
	}

	if(bkp_tree == NULL)
	{
		bkp_session_create_file();
		return -1;
	}
	bkp_session_move_inform_to_inform_queue ();
	bkp_session_save();
	return 0;
}

int cwmp_init_backup_session(struct cwmp *cwmp, char **ret, enum backup_loading load)
{
	int error;

	if(bkp_session_check_file())
		return 0;

	error = cwmp_load_saved_session(cwmp, ret, load);
	return error;
}

int cwmp_load_saved_session(struct cwmp *cwmp, char **ret, enum backup_loading load)
{
	mxml_node_t *b;

	b = bkp_tree;
	b = mxmlWalkNext(b, bkp_tree, MXML_DESCEND);
	while(b)
	{
		if(load == ACS)
		{
			if(b->type == MXML_ELEMENT && strcmp(b->value.element.name, "acs") == 0)
			{
				*ret = load_child_value(b, "url");
				break;
			}
		}
		if(load == CR_IP)
		{
			if(b->type == MXML_ELEMENT && strcmp(b->value.element.name, "connection_request") == 0)
			{
				*ret = load_child_value(b, "ip");
				break;
			}
		}
		if(load == CR_PORT)
		{
			if(b->type == MXML_ELEMENT && strcmp(b->value.element.name, "connection_request") == 0)
			{
				*ret = load_child_value(b, "port");
				break;
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
			else if(b->type == MXML_ELEMENT && strcmp(b->value.element.name, "upload") == 0)
			{
				load_upload(b,cwmp);
			}
			else if(b->type == MXML_ELEMENT && strcmp(b->value.element.name, "transfer_complete") == 0)
			{
				load_transfer_complete(b,cwmp);
			}
			else if(b->type == MXML_ELEMENT && strcmp(b->value.element.name, "schedule_inform") == 0)
			{
				load_schedule_inform(b,cwmp);
			}
			else if(b->type == MXML_ELEMENT && strcmp(b->value.element.name, "change_du_state") == 0)
			{
				load_change_du_state(b,cwmp);
			}
			else if(b->type == MXML_ELEMENT && strcmp(b->value.element.name, "du_state_change_complete") == 0)
			{
				load_du_state_change_complete(b,cwmp);
			}
			else if(b->type == MXML_ELEMENT && strcmp(b->value.element.name, "schedule_download") == 0)
			{
				load_schedule_download(b,cwmp);
			}
			else if(b->type == MXML_ELEMENT && strcmp(b->value.element.name, "apply_schedule_download") == 0)
			{
				load_apply_schedule_download(b,cwmp);
			}
		}
		b = mxmlWalkNext(b, bkp_tree, MXML_NO_DESCEND);
	}

	return CWMP_OK;
}
