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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uci.h>
#include <unistd.h>
#include <sys/file.h>
#include "cwmp.h"
#include "backupSession.h"
#include "xml.h"
#include "log.h"
#include "dmentry.h"
#include "deviceinfo.h"

typedef enum uci_config_action {
    CMD_SET,
    CMD_SET_STATE,
    CMD_ADD_LIST,
    CMD_DEL,
} uci_config_action;

void show_help()
{
    fprintf(stdout, "\nUsage: cwmpd [option]\n");
    fprintf(stdout, "-b:    this option should be added only in the load phase\n");
    fprintf(stdout, "-m:    execute data model commands\n");
    fprintf(stdout, "-w:    generate wep keys\n");
    fprintf(stdout, "-g:    send GetRPCMethods to ACS\n");
    fprintf(stdout, "-v:    show the application version\n");
    fprintf(stdout, "-h:    show this help\n\n");
}
void show_version()
{
#ifndef CWMP_REVISION
    fprintf(stdout, "\nVersion: %s\n\n",CWMP_VERSION);
#else
    fprintf(stdout, "\nVersion: %s revision %s\n\n",CWMP_VERSION,CWMP_REVISION);
#endif
}

int uci_get_list_value(char *cmd, struct list_head *list)
{
    struct  uci_ptr             ptr;
    struct  uci_context         *c = uci_alloc_context();
    struct uci_element          *e;
    struct config_uci_list      *uci_list_elem;
    char                        *s,*t;
    int                         size = 0;

    if (!c)
    {
        CWMP_LOG(ERROR, "Out of memory");
        return size;
    }

    s = strdup(cmd);
    t = s;
    if (uci_lookup_ptr(c, &ptr, s, true) != UCI_OK)
    {
        CWMP_LOG(ERROR, "Invalid uci command path: %s",cmd);
        free(t);
        uci_free_context(c);
        return size;
    }

    if(ptr.o == NULL)
    {
        free(t);
        uci_free_context(c);
        return size;
    }

    if(ptr.o->type == UCI_TYPE_LIST)
    {
        uci_foreach_element(&ptr.o->v.list, e)
        {
            if((e != NULL)&&(e->name))
            {
                uci_list_elem = calloc(1,sizeof(struct config_uci_list));
                if(uci_list_elem == NULL)
                {
                    free(t);
                    uci_free_context(c);
                    return CWMP_GEN_ERR;
                }
                uci_list_elem->value = strdup(e->name);
                list_add_tail (&(uci_list_elem->list), list);
                size++;
            }
            else
            {
                free(t);
                uci_free_context(c);
                return size;
            }
        }
    }
    free(t);
    uci_free_context(c);
    return size;
}

int uci_get_value_common(char *cmd,char **value,bool state)
{
    struct  uci_ptr             ptr;
    struct  uci_context         *c = uci_alloc_context();
    char                        *s,*t;
    char                        state_path[32];

    *value = NULL;
    if (!c)
    {
        CWMP_LOG(ERROR, "Out of memory");
        return CWMP_GEN_ERR;
    }
    if (state)
    {
        strcpy(state_path,"/var/state");
        uci_add_delta_path(c, c->savedir);
        uci_set_savedir(c, state_path);
    }
    s = strdup(cmd);
    t = s;
    if (uci_lookup_ptr(c, &ptr, s, true) != UCI_OK)
    {
        CWMP_LOG(ERROR, "Error occurred in uci %s get %s",state?"state":"config",cmd);
        free(t);
        uci_free_context(c);
        return CWMP_GEN_ERR;
    }
    free(t);
    if(ptr.flags & UCI_LOOKUP_COMPLETE)
    {
        if (ptr.o==NULL || ptr.o->v.string==NULL)
        {
            CWMP_LOG(INFO, "%s not found or empty value",cmd);
            uci_free_context(c);
            return CWMP_OK;
        }
        *value = strdup(ptr.o->v.string);
    }
    uci_free_context(c);
    return CWMP_OK;
}

int uci_get_state_value(char *cmd,char **value)
{
    int error;
    error = uci_get_value_common (cmd,value,true);
    return error;
}

int uci_get_value(char *cmd,char **value)
{
    int error;
    error = uci_get_value_common (cmd,value,false);
    return error;
}

static int uci_action_value_common(char *cmd, uci_config_action action)
{
    int                         ret = UCI_OK;
    char                        *s,*t;
    struct uci_context          *c = uci_alloc_context();
    struct uci_ptr              ptr;
    char                        state_path[32];

    s = strdup(cmd);
    t = s;

    if (!c)
    {
        CWMP_LOG(ERROR, "Out of memory");
        return CWMP_GEN_ERR;
    }

    if (action == CMD_SET_STATE)
    {
        strcpy(state_path,"/var/state");
        uci_add_delta_path(c, c->savedir);
        uci_set_savedir(c, state_path);
    }

    if (uci_lookup_ptr(c, &ptr, s, true) != UCI_OK)
    {
        free(t);
        uci_free_context(c);
        return CWMP_GEN_ERR;
    }
    switch (action)
    {
        case CMD_SET:
        case CMD_SET_STATE:
            ret = uci_set(c, &ptr);
            break;
        case CMD_DEL:
            ret = uci_delete(c, &ptr);
            break;
        case CMD_ADD_LIST:
            ret = uci_add_list(c, &ptr);
            break;
    }
    if (ret == UCI_OK)
    {
        ret = uci_save(c, ptr.p);
    }
    else
    {
        CWMP_LOG(ERROR, "UCI %s %s not succeed %s",action==CMD_SET_STATE?"state":"config",action==CMD_DEL?"delete":"set",cmd);
    }
    free(t);
    uci_free_context(c);
    return CWMP_OK;
}

int uci_delete_value(char *cmd)
{
    int error;
    error = uci_action_value_common (cmd,CMD_DEL);
    return error;
}

int uci_set_value(char *cmd)
{
    int error;
    error = uci_action_value_common (cmd,CMD_SET);
    return error;
}

int uci_set_state_value(char *cmd)
{
    int error;
    error = uci_action_value_common (cmd,CMD_SET_STATE);
    return error;
}

int uci_add_list_value(char *cmd)
{
    int error;
    error = uci_action_value_common (cmd,CMD_ADD_LIST);
    return error;
}

static int cwmp_package_commit(struct uci_context *c,char *tuple)
{
    struct uci_element      *e = NULL;
    struct uci_ptr          ptr;

    if (uci_lookup_ptr(c, &ptr, tuple, true) != UCI_OK) {
        return CWMP_GEN_ERR;
    }

    e = ptr.last;

    if (uci_commit(c, &ptr.p, false) != UCI_OK)
    {
        return CWMP_GEN_ERR;
    }

    uci_unload(c, ptr.p);
    return CWMP_OK;
}

static int cwmp_do_package_cmd(struct uci_context *c)
{
    char **configs = NULL;
    char **p;

    if ((uci_list_configs(c, &configs) != UCI_OK) || !configs)
    {
        return CWMP_GEN_ERR;
    }

    for (p = configs; *p; p++)
    {
        cwmp_package_commit(c,*p);
    }
    FREE(configs);
    return CWMP_OK;
}

int uci_commit_value()
{
    int                 ret;
    struct uci_context  *c = uci_alloc_context();

    if (!c)
    {
        CWMP_LOG(ERROR, "Out of memory");
        return CWMP_GEN_ERR;
    }

    ret = cwmp_do_package_cmd(c);
    if(ret == CWMP_OK)
    {
        uci_free_context(c);
        return ret;
    }

    uci_free_context(c);
    return CWMP_GEN_ERR;
}

int uci_revert_value ()
{
    char **configs = NULL;
    char **p;
    struct  uci_context         *ctx = uci_alloc_context();
    struct  uci_ptr             ptr;

    if (!ctx)
    {
        return CWMP_GEN_ERR;
    }

    if ((uci_list_configs(ctx, &configs) != UCI_OK) || !configs) {
        return CWMP_GEN_ERR;
    }

    for (p = configs; *p; p++)
    {
        if (uci_lookup_ptr(ctx, &ptr, *p, true) != UCI_OK)
        {
            return CWMP_GEN_ERR;
        }
        uci_revert(ctx, &ptr);
    }
    FREE(configs);
    uci_free_context(ctx);

    return CWMP_OK;
}

int check_global_config (struct config *conf)
{
    if (conf->acsurl==NULL)
    {
        conf->acsurl = strdup(DEFAULT_ACSURL);
    }
    return CWMP_OK;
}

static void uppercase ( char *sPtr )
{
	while ( *sPtr != '\0' )
	{
		*sPtr = toupper ( ( unsigned char ) *sPtr );
		++sPtr;
	}
}

int get_global_config(struct config *conf)
{
    int                     error, error2, error3;
    char                    *value = NULL, *value2 = NULL, *value3 = NULL;

    if((error = uci_get_value(UCI_CPE_LOG_FILE_NAME,&value)) == CWMP_OK)
    {
        if(value != NULL)
        {
            log_set_log_file_name (value);
            free(value);
            value = NULL;
        }
    }

    if((error = uci_get_value(UCI_CPE_LOG_MAX_SIZE,&value)) == CWMP_OK)
    {
        if(value != NULL)
        {
            log_set_file_max_size(value);
            free(value);
            value = NULL;
        }
    }

    if((error = uci_get_value(UCI_CPE_ENABLE_STDOUT_LOG,&value)) == CWMP_OK)
    {
        if(value != NULL)
        {
            log_set_on_console(value);
            free(value);
            value = NULL;
        }
    }

    if((error = uci_get_value(UCI_CPE_ENABLE_FILE_LOG,&value)) == CWMP_OK)
    {
        if(value != NULL)
        {
            log_set_on_file(value);
            free(value);
            value = NULL;
        }
    }

    if((error = uci_get_value(UCI_DHCP_ACS_URL_PATH,&value)) == CWMP_OK)
    {
        if(value != NULL)
        {
            if (conf->dhcp_url_path!=NULL)
            {
                free(conf->dhcp_url_path);
            }
            conf->dhcp_url_path = value;
            value = NULL;
        }
    }
    else
    {
        return error;
    }

    error 	= uci_get_value(UCI_DHCP_DISCOVERY_PATH,&value);
    error2 	= uci_get_state_value(UCI_ACS_URL_PATH,&value2);
    error3 	= uci_get_state_value(conf->dhcp_url_path,&value3);

    if ((((error == CWMP_OK) && (value != NULL) && (strcmp(value,"enable") == 0)) ||
	   ((error2 == CWMP_OK) && ((value2 == NULL) || (value2[0] == 0)))) &&
	   ((error3 == CWMP_OK) && (value3 != NULL) && (value3[0] != 0)))
    {
		if (conf->acsurl!=NULL)
		{
			free(conf->acsurl);
		}
		conf->acsurl = value3;
		value3 = NULL;
    }
    else if ((error2 == CWMP_OK) && (value2 != NULL) && (value2[0] != 0))
    {
		if (conf->acsurl!=NULL)
		{
			free(conf->acsurl);
		}
		conf->acsurl = value2;
		value2 = NULL;
    }
    if (value!=NULL)
    {
    	free(value);
    	value = NULL;
    }
    if (value2!=NULL)
	{
		free(value2);
		value2 = NULL;
	}
    if (value3!=NULL)
	{
		free(value3);
		value3 = NULL;
	}

    if((error = uci_get_value(UCI_ACS_USERID_PATH,&value)) == CWMP_OK)
    {
        if(value != NULL)
        {
            if (conf->acs_userid!=NULL)
            {
                free(conf->acs_userid);
            }
            conf->acs_userid = value;
            value = NULL;
        }
    }
    else
    {
        return error;
    }
    if((error = uci_get_value(UCI_ACS_PASSWD_PATH,&value)) == CWMP_OK)
    {
        if(value != NULL)
        {
            if (conf->acs_passwd!=NULL)
            {
                free(conf->acs_passwd);
            }
            conf->acs_passwd = value;
            value = NULL;
        }
    }
    else
    {
        return error;
    }
    if((error = uci_get_value(UCI_CPE_INTERFACE_PATH,&value)) == CWMP_OK)
    {
        if(value != NULL)
        {
            if (conf->interface!=NULL)
            {
                free(conf->interface);
            }
            conf->interface = value;
            value = NULL;
        }
    }
    else
    {
        return error;
    }
    if((error = uci_get_value(UCI_CPE_USERID_PATH,&value)) == CWMP_OK)
    {
        if(value != NULL)
        {
            if (conf->cpe_userid!=NULL)
            {
                free(conf->cpe_userid);
            }
            conf->cpe_userid = value;
            value = NULL;
        }
    }
    else
    {
        return error;
    }
    if((error = uci_get_value(UCI_CPE_PASSWD_PATH,&value)) == CWMP_OK)
    {
        if(value != NULL)
        {
            if (conf->cpe_passwd!=NULL)
            {
                free(conf->cpe_passwd);
            }
            conf->cpe_passwd = value;
            value = NULL;
        }
    }
    else
    {
        return error;
    }

    if((error = uci_get_value(UCI_CPE_UBUS_SOCKET_PATH,&value)) == CWMP_OK)
	{
		if(value != NULL)
		{
			if (conf->ubus_socket!=NULL)
			{
				free(conf->ubus_socket);
			}
			conf->ubus_socket = value;
			value = NULL;
		}
	}
	else
	{
		return error;
	}

    if((error = uci_get_value(UCI_LOG_SEVERITY_PATH,&value)) == CWMP_OK)
    {
        if(value != NULL)
        {
            log_set_severity_idx (value);
            free(value);
            value = NULL;
        }
    }
    else
    {
        return error;
    }
    if((error = uci_get_value(UCI_CPE_PORT_PATH,&value)) == CWMP_OK)
    {
        int a = 0;

        if(value != NULL)
        {
            a = atoi(value);
            free(value);
            value = NULL;
        }
        if(a==0)
        {
            CWMP_LOG(INFO,"Set the connection request port to the default value: %d",DEFAULT_CONNECTION_REQUEST_PORT);
            conf->connection_request_port = DEFAULT_CONNECTION_REQUEST_PORT;
        }
        else
        {
            conf->connection_request_port = a;
        }
    }
    else
    {
        return error;
    }
     if((error = uci_get_value(UCI_PERIODIC_INFORM_TIME_PATH,&value)) == CWMP_OK)
    {
        int a = 0;

        if(value != NULL)
        {
            a = atol(value);
            free(value);
            value = NULL;
        }
        conf->time = a;
    }
    else
    {
        return error;
    }
    if((error = uci_get_value(UCI_PERIODIC_INFORM_INTERVAL_PATH,&value)) == CWMP_OK)
    {
        int a = 0;

        if(value != NULL)
        {
            a = atoi(value);
            free(value);
            value = NULL;
        }
        if(a>=PERIOD_INFORM_MIN)
        {
            conf->period = a;
        }
        else
        {
            CWMP_LOG(ERROR,"Period interval of periodic inform should be > %ds. Set to default: %ds",PERIOD_INFORM_MIN,PERIOD_INFORM_DEFAULT);
            conf->period = PERIOD_INFORM_DEFAULT;
        }
    }
    else
    {
        return error;
    }
    if((error = uci_get_value(UCI_PERIODIC_INFORM_ENABLE_PATH,&value)) == CWMP_OK)
	{
		if(value != NULL)
		{
			uppercase(value);
			if ((strcmp(value,"TRUE")==0) || (strcmp(value,"1")==0))
			{
				conf->periodic_enable = true;
			}
			else
			{
				conf->periodic_enable = false;
			}
			free(value);
			value = NULL;
		}
		else
		{
			conf->periodic_enable = false;
		}
	}
	else
	{
		return error;
	}

    return CWMP_OK;
}

int global_env_init (int argc, char** argv, struct env *env)
{
    int i,error=0;

    for (i=1;i<argc;i++)
    {
        if (argv[i][0]!='-')
            continue;
        switch (argv[i][1])
        {
            case 'b':
                env->boot = CWMP_START_BOOT;
                break;
            case 'g':
                env->periodic = CWMP_START_PERIODIC;
                break;
            case 'm':
            	dm_entry_cli(argc, argv);
            	exit(EXIT_SUCCESS);
            	break;
            case 'w':
            	wepkey_cli(argc, argv);
            	exit(EXIT_SUCCESS);
            	break;
            case 'v':
                show_version();
                exit(EXIT_SUCCESS);
                break;
            case 'h':
                show_help();
                exit(EXIT_SUCCESS);
                break;
        }
    }

    return CWMP_OK;
}

int global_conf_init (struct config *conf)
{
    int error;

    if (error = get_global_config(conf))
    {
        return error;
    }
    if (error = check_global_config(conf))
    {
        return error;
    }
    return CWMP_OK;
}

int save_acs_bkp_config(struct cwmp *cwmp)
{
    struct config   *conf;

    conf = &(cwmp->conf);
	bkp_session_simple_insert("acs", "url", conf->acsurl);
	bkp_session_save();
    return CWMP_OK;
}

int cwmp_get_deviceid(struct cwmp *cwmp) {
	cwmp->deviceid.manufacturer = strdup(get_deviceid_manufacturer()); //TODO free
	cwmp->deviceid.serialnumber = strdup(get_deviceid_serialnumber());
	cwmp->deviceid.productclass = strdup(get_deviceid_productclass());
	cwmp->deviceid.oui = strdup(get_deviceid_manufactureroui());
	cwmp->deviceid.softwareversion = strdup(get_softwareversion());
	return CWMP_OK;
}

int cwmp_init(int argc, char** argv,struct cwmp *cwmp)
{
    int         error;
    struct env  env;
    memset(&env,0,sizeof(struct env));
    if(error = global_env_init (argc, argv, &env))
    {
        return error;
    }
    /* Only One instance should run*/
    cwmp->pid_file = open("/var/run/cwmpd.pid", O_CREAT | O_RDWR, 0666);
    fcntl(cwmp->pid_file, F_SETFD, fcntl(cwmp->pid_file, F_GETFD) | FD_CLOEXEC);
    int rc = flock(cwmp->pid_file, LOCK_EX | LOCK_NB);
    if(rc) {
        if(EWOULDBLOCK != errno)
        {
        	char *piderr = "PID file creation failed: Quit the daemon!";
        	fprintf(stderr, "%s\n", piderr);
        	CWMP_LOG(ERROR, piderr);
        	exit(EXIT_FAILURE);
        }
        else exit(EXIT_SUCCESS);
    }

    pthread_mutex_init(&cwmp->mutex_periodic, NULL);
    pthread_mutex_init(&cwmp->mutex_session_queue, NULL);
    pthread_mutex_init(&cwmp->mutex_session_send, NULL);
    memcpy(&(cwmp->env),&env,sizeof(struct env));
    INIT_LIST_HEAD(&(cwmp->head_session_queue));
    if(error = global_conf_init(&(cwmp->conf)))
    {
        return error;
    }
	dm_global_init();
    cwmp_get_deviceid(cwmp);
    dm_entry_load_enabled_notify();
	dm_global_clean();
    return CWMP_OK;
}

int cwmp_config_reload(struct cwmp *cwmp)
{
    int error;
    memset(&cwmp->env,0,sizeof(struct env));
    memset(&cwmp->conf,0,sizeof(struct config));
    if(error = global_conf_init(&(cwmp->conf)))
    {
        return error;
    }
    dm_entry_load_enabled_notify();
    return CWMP_OK;
}
