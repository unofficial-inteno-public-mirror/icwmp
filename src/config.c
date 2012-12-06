/*
    config.c

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
#include <regex.h>
#include <sys/stat.h>
#include "soapH.h"
#include "cwmp.h"
#include <uci.h>
#include "backupSession.h"
#include <unistd.h>

void backup_session_insert_acs(char *value);

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
    if (uci_lookup_ptr(c, &ptr, s, TRUE) != UCI_OK)
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
        uci_set_savedir(c, state_path); /* KMD TODO to check for DHCP*/
    }
    s = strdup(cmd);
    t = s;
    if (uci_lookup_ptr(c, &ptr, s, TRUE) != UCI_OK)
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
    error = uci_get_value_common (cmd,value,TRUE);
    return error;
}

int uci_get_value(char *cmd,char **value)
{
    int error;
    error = uci_get_value_common (cmd,value,FALSE);
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
        /*strcpy(state_path,"/var/state");
        uci_add_history_path(c, c->savedir);
        uci_set_savedir(c, state_path);*/ /* KMD TODO to check for DHCP*/
    }

    if (uci_lookup_ptr(c, &ptr, s, TRUE) != UCI_OK)
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

    if (uci_lookup_ptr(c, &ptr, tuple, TRUE) != UCI_OK) {
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
        if (uci_lookup_ptr(ctx, &ptr, *p, TRUE) != UCI_OK)
        {
            return CWMP_GEN_ERR;
        }
        uci_revert(ctx, &ptr);
    }
    uci_free_context(ctx);

    return CWMP_OK;
}

int uci_apply_web_packages()
{
    FILE            *fp;
    char            cmd[256];
    int             error;

    sprintf(cmd,"/bin/opkg install %s",DOWNLOADED_WEBCONTENT_FILE);
    fp = popen(cmd,"r");
    error = pclose(fp);

    remove(DOWNLOADED_WEBCONTENT_FILE);
    if(error == 0)
    {
        return FAULT_CPE_NO_FAULT_IDX;
    }
    else
    {
        return FAULT_CPE_DOWNLOAD_FAIL_FILE_CORRUPTED_IDX;
    }
}

int uci_apply_configuration()
{
    struct  uci_context         *ctx = uci_alloc_context();
    struct uci_package          *package = NULL;
    char                        *name = NULL;
    int                         ret = UCI_OK;
    FILE                        *pFile;
    struct uci_package          *p;
    struct uci_element          *e;

    if (!ctx)
    {
        CWMP_LOG(ERROR, "Out of memory");
        return FAULT_CPE_INTERNAL_ERROR_IDX;
    }

    pFile = fopen(DOWNLOADED_CONFIG_FILE,"rb");

    if(pFile == NULL)
    {
        CWMP_LOG(ERROR,"Configuration is not readable");
        uci_free_context(ctx);
        return FAULT_CPE_DOWNLOAD_FAIL_FILE_CORRUPTED_IDX;
    }
    ret = uci_import(ctx, pFile, name, &package, (name != NULL));
    if (ret == UCI_OK)
    {
        CWMP_LOG(INFO,"Trying to apply configuration file");
        uci_foreach_element(&ctx->root, e)
        {
            p = uci_to_package(e);
            ret = uci_commit(ctx, &p, true);
            if(ret != CWMP_OK)
            {
                CWMP_LOG(ERROR,"Unable to save configuration");
                fclose(pFile);
                remove(DOWNLOADED_CONFIG_FILE);
                uci_free_context(ctx);
                return FAULT_CPE_INTERNAL_ERROR_IDX;
            }
        }
    }
    else
    {
        CWMP_LOG(ERROR,"Can not apply downloaded configuration file");
        fclose(pFile);
        remove(DOWNLOADED_CONFIG_FILE);
        uci_free_context(ctx);
        return FAULT_CPE_DOWNLOAD_FAIL_FILE_CORRUPTED_IDX;
    }
    fclose(pFile);
    uci_free_context(ctx);
    return FAULT_CPE_NO_FAULT_IDX;
}

static int cwmp_check_image()
{
    FILE            *fp;
    char            cmd[256];
    int             error;

    sprintf(cmd,". /etc/functions.sh; include /lib/upgrade; platform_check_image %s >/dev/null",DOWNLOADED_FIRMWARE_FILE);
    fp = popen(cmd,"r");
    error = pclose(fp);

    if(error == 0)
    {
        return CWMP_OK;
    }
    else
    {
        return CWMP_GEN_ERR;
    }

    return CWMP_OK;
}

static int checkline (const char *str_regex,const char *str_request)
{
	int                 error;
	regex_t             preg;
	int                 match;
	size_t              nmatch = 0;
	regmatch_t          *pmatch = NULL;

	if((str_request == NULL) || (strcmp(str_request,"") == 0))
	{
		return CWMP_GEN_ERR;
	}

	error = regcomp (&preg, str_regex, REG_EXTENDED);
	if (error == 0)
	{
		nmatch = preg.re_nsub;
		pmatch = malloc (sizeof (*pmatch) * nmatch);
		if (pmatch)
		{
			match = regexec (&preg, str_request, nmatch, pmatch, 0);
			regfree (&preg);
			if (match == 0)
			{
				return CWMP_OK;
			}
			else if (match == REG_NOMATCH)
			{
				return CWMP_GEN_ERR;
			}
			else
			{
				return CWMP_MEM_ERR;
			}
	  }
	  else
	  {
		  return CWMP_MEM_ERR;
	  }
	}
    return CWMP_GEN_ERR;
}

long int cwmp_check_flash_size()
{
    char        line[256];
    long int    size = 0;
    FILE        *fp;
    char        *n = NULL;
    char        *b = NULL;
    char        *s = NULL;
    char        *t = NULL;
    char        *endptr = NULL;
    int         i = 0,error;

    fp = fopen("/proc/mtd","r");
    if (fp != NULL)
    {
        while (fgets(line,sizeof(line),fp))
        {
            if(checkline("^([^[:space:]]+)[[:space:]]+([^[:space:]]+)[[:space:]]+([^[:space:]]+)[[:space:]]+\"([^[:space:]]+)\"",line) == CWMP_OK)
            {
                t = strdup(line);
                n = t;
                n = strtok(n," ");
                i = 0;
                while(n != NULL)
                {
                    if(i == 1)
                    {
                        s = strdup(n);
                    }
                    if(i == 3)
                    {
                        break;
                    }
                    n = strtok(NULL," ");
                    i++;
                }
                n[strlen(n)-1] = 0;
                if(strcmp(n,"\"linux\"") == 0 || strcmp(n,"\"firmware\"") == 0)
                {
                    size = strtol(s, &endptr, 16);
                    break;
                }
            }
        }
    }
    else if (fp = fopen("/proc/partitions","r"))
    {
        while (fgets(line,sizeof(line),fp))
        {
            if(checkline("[[:space:]]*([[:digit:]]+)[[:space:]]+([[:digit:]]+)[[:space:]]+([^[:space:]]+)[[:space:]]+([^[:space:]]+)",line) == CWMP_OK)
            {
                i = 0;
                t = strdup(line);
                n = t;
                n = strtok(n," ");
                while(n != NULL)
                {
                    if(i == 2)
                    {
                        b = strdup(n);
                    }
                    if(i == 3)
                    {
                        break;
                    }
                    n = strtok(NULL," ");
                    i++;
                }
                if ((b != NULL) && (n != NULL) && (checkline("([^[:space:]]+)",n) == CWMP_OK))
                {
                    size = atoi(b) * 1024;
                    break;
                }
            }
        }
    }
    if(t != NULL)
    {
        free(t);
    }
    if(b != NULL)
    {
        free(b);
    }
    if(s != NULL)
    {
        free(s);
    }
    fclose(fp);
    return size;
}

static long int cwmp_get_firmware_size()
{
    struct stat st;
    long int    size = 0;

    if (stat(DOWNLOADED_FIRMWARE_FILE, &st) == 0)
    {
        size = st.st_size;
    }

    return size;
}

int cwmp_start_upgrade(struct cwmp *cwmp,void *v)
{
    char            cmd[256];
    char            line[256];
    FILE            *fp;
    int             error;

    CWMP_LOG(INFO,"RUN Firmware upgrade function");
    /** flush file system buffers **/
    sync();
    sprintf(cmd,"killall dropbear uhttpd; sleep 1; /sbin/sysupgrade %s",DOWNLOADED_LAST_VALID_FIRMWARE_FILE);
    fp = popen(cmd,"r");
    while (fgets(line,sizeof(line),fp))
    {
        continue;
    }
    error = pclose(fp);
    if(error == 0)
    {
        return CWMP_OK;
    }
    else
    {
        return CWMP_GEN_ERR;
    }
}

int cwmp_reset_factory(struct cwmp *cwmp,void *v)
{
    char            cmd[256];
    char            line[256];
    FILE            *fp;
    int             error;

    CWMP_LOG(INFO,"RUN Factory reset function"); /* TODO to be removed*/
    sprintf(cmd,"killall dropbear uhttpd; sleep 1; mtd -r erase rootfs_data");
    fp = popen(cmd,"r");
    while (fgets(line,sizeof(line),fp))
    {
        continue;
    }
    error = pclose(fp);
    if(error == 0)
    {
        return CWMP_OK;
    }
    else
    {
        return CWMP_GEN_ERR;
    }
}

int uci_upgrade_image(struct cwmp *cwmp, struct session *session)
{
    int 		error;
    long int 	flashsize = 0,filesize = 0;
    FILE		*fp;

    if(cwmp_check_image() == CWMP_OK)
    {
    	flashsize = cwmp->env.max_firmware_size;
        filesize = cwmp_get_firmware_size();

        if((flashsize > 0)&&(filesize > flashsize))
        {
            remove(DOWNLOADED_FIRMWARE_FILE);
            return FAULT_CPE_DOWNLOAD_FAIL_FILE_CORRUPTED_IDX;
        }
        else
        {
			remove(DOWNLOADED_LAST_VALID_FIRMWARE_FILE);
        	rename(DOWNLOADED_FIRMWARE_FILE,DOWNLOADED_LAST_VALID_FIRMWARE_FILE);
        	if(session != NULL)
        	{
            	add_session_end_func(session,cwmp_start_upgrade,NULL,FALSE);
        	}
            else
            {
            	add_download_end_func(cwmp_start_upgrade,NULL);
            }
            return FAULT_CPE_NO_FAULT_IDX;
        }
    }
    else
    {
        remove(DOWNLOADED_FIRMWARE_FILE);
        return FAULT_CPE_DOWNLOAD_FAIL_FILE_CORRUPTED_IDX;
    }
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

    error 	= uci_get_value(UCI_DHCP_DISCOVERY_PATH,&value);
    error2 	= uci_get_state_value(UCI_ACS_URL_PATH,&value2);
    error3 	= uci_get_state_value(UCI_DHCP_ACS_URL_PATH,&value3);

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
				conf->periodic_enable = TRUE;
			}
			else
			{
				conf->periodic_enable = FALSE;
			}
			free(value);
			value = NULL;
		}
		else
		{
			conf->periodic_enable = FALSE;
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
            case 'c':
				env->iccu = CWMP_START_ICCU;
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
    env->max_firmware_size = cwmp_check_flash_size();
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
    int             error;
    char            *acsurl = NULL;
    struct config   *conf;

    conf = &(cwmp->conf);
    error = cwmp_load_saved_session(cwmp, &acsurl, ACS);
    if((acsurl == NULL)||(acsurl != NULL && strcmp(acsurl,conf->acsurl) != 0))
    {
        backup_session_insert_acs(conf->acsurl);
        if(acsurl != NULL)
        {
            free(acsurl);
        }
    }
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
    memset(cwmp,0,sizeof(struct cwmp));
    memcpy(&(cwmp->env),&env,sizeof(struct env));
    INIT_LIST_HEAD(&(cwmp->head_session_queue));
    INIT_LIST_HEAD(&(cwmp->api_value_change.parameter_list));
    if(error = global_conf_init(&(cwmp->conf)))
    {
        return error;
    }
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
    return CWMP_OK;
}
