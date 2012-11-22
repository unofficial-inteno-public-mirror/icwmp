/*
    Download.c

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
#include <time.h>
#include <curl/curl.h>
#include <curl/easy.h>
#include <string.h>
#include "soapH.h"
#include "cwmp.h"
#include "backupSession.h"

#if LIBCURL_VERSION_NUM < 0x070907
#error LIBCURL version should be >= 7.9.7
#endif

LIST_HEAD(list_download);
static struct download_end_func     *download_end_func = NULL;
static pthread_mutex_t      		mutex_download;
static pthread_cond_t       		threshold_download;
static bool                 		thread_download_is_working = FALSE;
int									count_download_queue = 0;
extern struct cwmp         			cwmp_main;

struct rpc_cpe *cwmp_add_session_rpc_cpe (struct session *session);
struct rpc_cpe *cwmp_add_session_rpc_cpe_download (struct session *session);
int cwmp_rpc_cpe_download (struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
int cwmp_rpc_cpe_download_response_data_init(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
int cwmp_rpc_cpe_download_response(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
int cwmp_rpc_cpe_download_end(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
int cwmp_session_rpc_cpe_destructor (struct cwmp *cwmp, struct session *session, struct rpc_cpe *rpc_cpe);
struct rpc_cpe *cwmp_add_session_rpc_cpe_Fault (struct session *session, int idx);
struct _cwmp1__TransferComplete *cwmp_set_data_rpc_acs_transferComplete();
int cwmp_reboot(struct cwmp *cwmp,void *v);
size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream);
int cwmp_launch_download(struct cwmp *cwmp, struct session *session, struct download *pDownload);
int cwmp_download_firmware(struct cwmp *cwmp, struct session *session);
int cwmp_download_web_content(struct cwmp *cwmp, struct session *session);
int cwmp_download_configuration_file(struct cwmp *cwmp, struct session *session);
void backup_session_insert_download(char *commandKey, char *url, time_t id);
void backup_session_delete_download(char *commandKey, char *url, time_t id);
void backup_session_insert_parameter_download(char *name, char *value, char *url, int id, char *commandKey);

typedef struct FILE_DOWNLOAD_TYPE
{
    char				*FILE_TYPE;
    char				*FILE_NAME;
    unsigned short	    APPLY_AFTER_REBOOT;
    unsigned int	    CHECK_SIZE;
    long int			*MEMORY_SIZE;
    int					(*UPDATE_VALUE)();
    int					(*DOWNLOAD)(struct cwmp *cwmp, struct session *session);
} FILE_DOWNLOAD_TYPE;

const struct FILE_DOWNLOAD_TYPE    FILE_DOWNLOAD_TYPE_ARRAY [] = {
    {"1 Firmware Upgrade Image"	  ,DOWNLOADED_FIRMWARE_FILE	 ,1,ENABLE_CHECK_SIZE ,&(cwmp_main.env.max_firmware_size),NULL,cwmp_download_firmware},
    {"2 Web Content"			  ,DOWNLOADED_WEBCONTENT_FILE,0,DISABLE_CHECK_SIZE,0								 ,NULL,cwmp_download_web_content},
    {"3 Vendor Configuration File",DOWNLOADED_CONFIG_FILE    ,1,DISABLE_CHECK_SIZE,0								 ,NULL,cwmp_download_configuration_file}
};

struct rpc_cpe *cwmp_add_session_rpc_cpe_download (struct session *session)
{
    struct rpc_cpe                  *rpc_cpe;
    struct soap_cwmp1_methods__rpc  *soap_methods;

    rpc_cpe = cwmp_add_session_rpc_cpe (session);
    if (rpc_cpe == NULL)
    {
        return NULL;
    }
    soap_methods                                    = &(rpc_cpe->soap_methods);
    rpc_cpe->method_data                            = (void *) calloc (1,sizeof(struct _cwmp1__Download));
    rpc_cpe->method                                 = cwmp_rpc_cpe_download;
    rpc_cpe->method_response_data                   = (void *) calloc (1,sizeof(struct _cwmp1__DownloadResponse));
    rpc_cpe->method_response_data_init              = cwmp_rpc_cpe_download_response_data_init;
    rpc_cpe->method_response                        = cwmp_rpc_cpe_download_response;
    rpc_cpe->method_end                             = cwmp_rpc_cpe_download_end;
    rpc_cpe->destructor                             = cwmp_session_rpc_cpe_destructor;
    soap_methods->envelope                          = "cwmp:Download";
    soap_methods->envelope_response                 = "cwmp:DownloadResponse";
    soap_methods->soap_serialize_cwmp1__send_data   = (void *) soap_serialize__cwmp1__DownloadResponse;
    soap_methods->soap_put_cwmp1__send_data         = (void *) soap_put__cwmp1__DownloadResponse;
    soap_methods->soap_get_cwmp1__rpc_received_data = (void *) soap_get__cwmp1__Download;
    return rpc_cpe;
}

void *thread_cwmp_rpc_cpe_download (void *v)
{
    struct cwmp                     			*cwmp = (struct cwmp *)v;
    struct download          					*download;
    struct timespec                 			download_timeout = {0, 0};
    time_t                          			current_time;
    int											error = FAULT_CPE_NO_FAULT_IDX;
    time_t										download_startTime;
    time_t										download_completeTime;
    char                                        startTime[64];
    char                                        completeTime[64];
    struct _cwmp1__TransferComplete             *p_soap_cwmp1__TransferComplete;
    extern struct FAULT_CPE                     FAULT_CPE_ARRAY [FAULT_CPE_ARRAY_SIZE];
    long int									time_of_grace = 3600,timeout;

    thread_download_is_working = TRUE;
    while (list_download.next!=&(list_download))
    {
    	download = list_entry(list_download.next,struct download, list);
        current_time    = time(NULL);
        timeout = current_time - download->scheduled_time;
        if((timeout >= 0)&&(timeout > time_of_grace))
		{
        	pthread_mutex_lock (&mutex_download);
        	backup_session_delete_download(download->CommandKey,download->URL,download->scheduled_time);
        	p_soap_cwmp1__TransferComplete = cwmp_set_data_rpc_acs_transferComplete ();
			if(p_soap_cwmp1__TransferComplete != NULL)
			{
				error = FAULT_CPE_DOWNLOAD_FAILURE_IDX;
				download_completeTime = time((time_t*)NULL);

				p_soap_cwmp1__TransferComplete->CommandKey					= strdup(download->CommandKey);
				p_soap_cwmp1__TransferComplete->StartTime 					= time((time_t*)NULL);
				p_soap_cwmp1__TransferComplete->CompleteTime				= p_soap_cwmp1__TransferComplete->StartTime;

				sprintf(startTime,"%li",p_soap_cwmp1__TransferComplete->StartTime);
				backup_session_insert_rpc("TransferComplete",p_soap_cwmp1__TransferComplete->CommandKey,RPC_NO_STATUS);
				backup_session_insert_parameter("StartTime",startTime,"rpc","TransferComplete",-1,p_soap_cwmp1__TransferComplete->CommandKey);
				backup_session_insert_parameter("CompleteTime",startTime,"rpc","TransferComplete",-1,p_soap_cwmp1__TransferComplete->CommandKey);

				p_soap_cwmp1__TransferComplete->FaultStruct 				= calloc(1,sizeof(struct cwmp1__FaultStruct));
				p_soap_cwmp1__TransferComplete->FaultStruct->FaultCode		= strdup(FAULT_CPE_ARRAY[error].CODE);
				p_soap_cwmp1__TransferComplete->FaultStruct->FaultString	= strdup(FAULT_CPE_ARRAY[error].DESCRIPTION);

				backup_session_insert_parameter("FaultCode",p_soap_cwmp1__TransferComplete->FaultStruct->FaultCode,"rpc","TransferComplete",-1,p_soap_cwmp1__TransferComplete->CommandKey);
				cwmp_root_cause_TransferComplete (cwmp);
			}
			list_del (&(download->list));
			count_download_queue--;
			cwmp_free_download_request(download);
			pthread_mutex_unlock (&mutex_download);
		}
        if((timeout >= 0)&&(timeout <= time_of_grace))
        {
        	pthread_mutex_lock (&(cwmp->mutex_session_send));
            CWMP_LOG(INFO,"Launch download file %s",download->URL);
            error = cwmp_launch_download(cwmp,NULL,download);
    		if((error != FAULT_CPE_NO_FAULT_IDX)||(download_end_func == NULL))
    		{
    			cwmp_root_cause_TransferComplete (cwmp);
    		}
    		run_download_end_func (cwmp);
    		pthread_mutex_unlock (&(cwmp->mutex_session_send));
			pthread_cond_signal (&(cwmp->threshold_session_send));
			pthread_mutex_lock (&mutex_download);
			list_del (&(download->list));
			count_download_queue--;
			cwmp_free_download_request(download);
			pthread_mutex_unlock (&mutex_download);
            continue;
        }
        pthread_mutex_lock (&mutex_download);
        download_timeout.tv_sec = download->scheduled_time;
        pthread_cond_timedwait(&threshold_download, &mutex_download, &download_timeout);
        pthread_mutex_unlock (&mutex_download);
    }
    thread_download_is_working = FALSE;
    return CWMP_OK;
}

size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    size_t written;

    written = fwrite(ptr, size, nmemb, stream);
    return written;
}

int cwmp_rpc_cpe_download (struct cwmp *cwmp, struct session *session, struct rpc_cpe *this)
{
    struct _cwmp1__DownloadResponse             *p_soap_cwmp1__DownloadResponse;
    struct _cwmp1__Download                     *p_soap_cwmp1__Download;
    int                                         error = FAULT_CPE_NO_FAULT_IDX;
    int											i;
    char                                        *url;
    long int                                    check_size;
	struct download                 			*download;
	time_t                          			scheduled_time;
	struct list_head                			*ilist;
	bool                            			cond_signal = FALSE;
	pthread_t                       			download_thread;
	char										scheduledTime[64];
	char										delaySeconds[64];
	bool										valid_file_type = FALSE;

    p_soap_cwmp1__Download                          = (struct _cwmp1__Download *)this->method_data;
    p_soap_cwmp1__DownloadResponse                  = (struct _cwmp1__DownloadResponse *)this->method_response_data;
    url 											= p_soap_cwmp1__Download->URL;
    p_soap_cwmp1__DownloadResponse->Status 			= _cwmp1__DownloadResponse_Status__1;

	for(i=0;i<sizearray(FILE_DOWNLOAD_TYPE_ARRAY);i++)
	{
		if(strcmp(FILE_DOWNLOAD_TYPE_ARRAY[i].FILE_TYPE,p_soap_cwmp1__Download->FileType) == 0)
		{
			valid_file_type = TRUE;
			if(FILE_DOWNLOAD_TYPE_ARRAY[i].CHECK_SIZE == ENABLE_CHECK_SIZE)
			{
				if(FILE_DOWNLOAD_TYPE_ARRAY[i].UPDATE_VALUE != NULL)
				{
					FILE_DOWNLOAD_TYPE_ARRAY[i].UPDATE_VALUE();
				}
				check_size = *(FILE_DOWNLOAD_TYPE_ARRAY[i].MEMORY_SIZE);
			}
			break;
		}
	}
    if((FILE_DOWNLOAD_TYPE_ARRAY[i].CHECK_SIZE == ENABLE_CHECK_SIZE)	&&
	   ((check_size>0)&&(check_size < (long int)p_soap_cwmp1__Download->FileSize)))
    {
        error = FAULT_CPE_DOWNLOAD_FAILURE_IDX;
    }
    else if(count_download_queue>=MAX_DOWNLOAD_QUEUE)
    {
    	error = FAULT_CPE_RESOURCES_EXCEEDED_IDX;
    }
    else if((url == NULL || ((url != NULL) && (strcmp(url,"")==0)))||
    		(!valid_file_type))
    {
        error = FAULT_CPE_REQUEST_DENIED_IDX;
    }
    else if(strstr(url,"@") != NULL)
    {
        error = FAULT_CPE_INVALID_ARGUMENTS_IDX;
    }
    else if(strncmp(url,DOWNLOAD_PROTOCOL_HTTP,strlen(DOWNLOAD_PROTOCOL_HTTP))!=0 &&
            strncmp(url,DOWNLOAD_PROTOCOL_FTP,strlen(DOWNLOAD_PROTOCOL_FTP))!=0)
    {
        error = FAULT_CPE_FILE_TRANSFER_UNSUPPORTED_PROTOCOL_IDX;
    }

    if(error != FAULT_CPE_NO_FAULT_IDX)
    {
        if (cwmp_add_session_rpc_cpe_Fault(session,error) == NULL)
        {
            return CWMP_GEN_ERR;
        }
        return CWMP_FAULT_CPE;
    }

    if(p_soap_cwmp1__Download->DelaySeconds == 0)
    {
    	download = calloc (1,sizeof(struct download));
    	if (download == NULL)
    	{
    	    return CWMP_GEN_ERR;
    	}
    	download->CommandKey		= strdup(p_soap_cwmp1__Download->CommandKey);
		download->FileType			= strdup(p_soap_cwmp1__Download->FileType);
		download->URL				= strdup(p_soap_cwmp1__Download->URL);
		download->Username			= strdup(p_soap_cwmp1__Download->Username);
		download->Password			= strdup(p_soap_cwmp1__Download->Password);
		download->scheduled_time	= 0;

    	error = cwmp_launch_download(cwmp,session,download);
		if((error != FAULT_CPE_NO_FAULT_IDX)||(FILE_DOWNLOAD_TYPE_ARRAY[i].APPLY_AFTER_REBOOT == 0))
		{
            if(cwmp_root_cause_TransferComplete (cwmp) != CWMP_OK)
            {
            	cwmp_free_download_request(download);
            	return CWMP_GEN_ERR;
            }
		}
		cwmp_free_download_request(download);
    }
    else
    {
    	pthread_mutex_lock (&mutex_download);

    	scheduled_time = time(NULL) + p_soap_cwmp1__Download->DelaySeconds;
    	list_for_each(ilist,&(list_download))
    	{
    	    download = list_entry(ilist,struct download, list);
    	    if (download->scheduled_time == scheduled_time)
    	    {
    	        pthread_mutex_unlock (&mutex_download);
    	        return CWMP_OK;
    	    }
    	    if (download->scheduled_time > scheduled_time)
    	    {
    	        cond_signal = TRUE;
    	        break;
    	    }
    	}
    	CWMP_LOG(INFO,"Download will start in %us",p_soap_cwmp1__Download->DelaySeconds);
    	download = calloc (1,sizeof(struct download));
    	if (download == NULL)
    	{
    	    pthread_mutex_unlock (&mutex_download);
    	    return CWMP_OK;
    	}

		download->CommandKey		= strdup(p_soap_cwmp1__Download->CommandKey);
		download->FileType			= strdup(p_soap_cwmp1__Download->FileType);
		download->URL				= strdup(p_soap_cwmp1__Download->URL);
		download->Username			= strdup(p_soap_cwmp1__Download->Username);
		download->Password			= strdup(p_soap_cwmp1__Download->Password);
		download->scheduled_time	= scheduled_time;

		backup_session_insert_download(download->CommandKey,download->URL,download->scheduled_time);
    	backup_session_insert_parameter_download("Password",download->Password,download->URL,download->scheduled_time,download->CommandKey);
    	backup_session_insert_parameter_download("Username",download->Username,download->URL,download->scheduled_time,download->CommandKey);
    	backup_session_insert_parameter_download("FileType",download->FileType,download->URL,download->scheduled_time,download->CommandKey);

    	list_add (&(download->list), ilist->prev);
    	count_download_queue++;
    	if (cond_signal)
    	{
    	    pthread_cond_signal(&threshold_download);
    	}
    	pthread_mutex_unlock (&mutex_download);

    	if (!thread_download_is_working)
    	{
    	    thread_download_is_working = TRUE;
    	    error = pthread_create(&download_thread, NULL, &thread_cwmp_rpc_cpe_download, (void *)cwmp);
    	    if (error<0)
    	    {
    	        CWMP_LOG(ERROR,"Error when creating the download thread!");
    	        thread_download_is_working = FALSE;
    	        return CWMP_OK;
    	    }
    	}
    }

    return CWMP_OK;
}

int cwmp_rpc_cpe_download_response_data_init(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this)
{
    return CWMP_OK;
}

int cwmp_rpc_cpe_download_response(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this)
{
    int error;
    error = cwmp_soap_send_rpc_cpe_response (cwmp, session, this);
    return error;
}

int cwmp_rpc_cpe_download_end(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this)
{
    return CWMP_OK;
}

int cwmp_launch_download(struct cwmp *cwmp, struct session *session, struct download *pDownload)
{
    CURL        								*curl;
    FILE        								*fp;
    CURLcode    								res;
    int         								error = FAULT_CPE_NO_FAULT_IDX;
    int											i;
    char        								filename[256];
    int											(*doDownload)(struct cwmp *cwmp, struct session *session);
    time_t										download_startTime;
    time_t										download_completeTime;
    char                                        startTime[64];
    char                                        completeTime[64];
    struct _cwmp1__TransferComplete             *p_soap_cwmp1__TransferComplete;
    extern struct FAULT_CPE                     FAULT_CPE_ARRAY [FAULT_CPE_ARRAY_SIZE];
    char										userpwd[256];

    download_startTime		= time((time_t*)NULL);

    if(pDownload->scheduled_time != 0)
    {
    	backup_session_delete_download(pDownload->CommandKey,pDownload->URL,pDownload->scheduled_time);
    }

	for(i=0;i<sizearray(FILE_DOWNLOAD_TYPE_ARRAY);i++)
	{
		if(strcmp(FILE_DOWNLOAD_TYPE_ARRAY[i].FILE_TYPE,pDownload->FileType) == 0)
		{
			doDownload = FILE_DOWNLOAD_TYPE_ARRAY[i].DOWNLOAD;
			strcpy(filename,FILE_DOWNLOAD_TYPE_ARRAY[i].FILE_NAME);
			break;
		}
	}

	curl = curl_easy_init();

	if (curl)
	{
		fp = fopen(filename,"wb");
		if((strcmp(pDownload->URL,"") != 0)&&
		   (pDownload->URL != NULL))
		{

#if LIBCURL_VERSION_NUM >= 0x071301
			if((strcmp(pDownload->Username,"") != 0)&&
			   (pDownload->Username != NULL))
			{
				curl_easy_setopt(curl, CURLOPT_USERNAME, pDownload->Username);
			}
			if((strcmp(pDownload->Password,"") != 0)&&
			   (pDownload->Password != NULL))
			{
				curl_easy_setopt(curl, CURLOPT_PASSWORD, pDownload->Password);
			}
#else
			if(((strcmp(pDownload->Username,"") != 0)&&(pDownload->Username != NULL))&&
				(strcmp(pDownload->Password,"") != 0)&&(pDownload->Password != NULL))
			{
				sprintf(userpwd,"%s:%s",pDownload->Username,pDownload->Password);
				curl_easy_setopt(curl, CURLOPT_USERPWD, userpwd);
			}
#endif
			curl_easy_setopt(curl, CURLOPT_URL, pDownload->URL);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
			curl_easy_setopt(curl, CURLOPT_FAILONERROR, TRUE);
			res = curl_easy_perform(curl);
			curl_easy_cleanup(curl);
		}
		fclose(fp);
	}

	if(res != CURLE_OK)
	{
		remove(filename);
	}

	switch(res)
	{
		case CURLE_OK:
			error = FAULT_CPE_NO_FAULT_IDX; /** No fault code **/
			break;
		case CURLE_UNSUPPORTED_PROTOCOL:
#if LIBCURL_VERSION_NUM >= 0x071100
		case CURLE_REMOTE_ACCESS_DENIED:
#endif
			error = FAULT_CPE_REQUEST_DENIED_IDX; /** Request denied **/
			break;
		case CURLE_OUT_OF_MEMORY:
			error = FAULT_CPE_INTERNAL_ERROR_IDX; /** Internal error **/
			break;
		case CURLE_FTP_CANT_GET_HOST:
		case CURLE_URL_MALFORMAT:
		case CURLE_COULDNT_RESOLVE_HOST:
		case CURLE_COULDNT_CONNECT:
			error = FAULT_CPE_DOWNLOAD_FAIL_CONTACT_SERVER_IDX; /** Download failure: unable to contact file server **/
			break;
		case CURLE_FTP_COULDNT_RETR_FILE:
#if LIBCURL_VERSION_NUM >= 0x071001
		case CURLE_REMOTE_FILE_NOT_FOUND:
#endif
#if LIBCURL_VERSION_NUM >= 0x070a03
		case CURLE_HTTP_RETURNED_ERROR:
#endif
#if LIBCURL_VERSION_NUM >= 0x071500
		case CURLE_FTP_BAD_FILE_LIST:
#endif
#if LIBCURL_VERSION_NUM >= 0x070901
		case CURLE_GOT_NOTHING:
#endif
			error = FAULT_CPE_DOWNLOAD_FAIL_ACCESS_FILE_IDX; /** Download failure: unable to access file **/
			break;
#if LIBCURL_VERSION_NUM >= 0x070a02
		case CURLE_OPERATION_TIMEDOUT:
			error = FAULT_CPE_DOWNLOAD_FAIL_COMPLETE_DOWNLOAD_IDX; /** Download failure: unable to complete download **/
			break;
#endif
#if LIBCURL_VERSION_NUM >= 0x070d01
		case CURLE_LOGIN_DENIED:
			error = FAULT_CPE_DOWNLOAD_FAIL_FILE_AUTHENTICATION_IDX; /** Download failure: file authentication failure **/
			break;
#endif
		default:
			error = FAULT_CPE_DOWNLOAD_FAILURE_IDX; /** Download failure **/
			break;
	}

	if(error == FAULT_CPE_NO_FAULT_IDX)
	{
		error = doDownload (cwmp,session);
	}

	p_soap_cwmp1__TransferComplete = cwmp_set_data_rpc_acs_transferComplete ();
	if(p_soap_cwmp1__TransferComplete == NULL)
	{
		error = FAULT_CPE_INTERNAL_ERROR_IDX;
		return error;
	}
	download_completeTime = time((time_t*)NULL);

	p_soap_cwmp1__TransferComplete->CommandKey					= strdup(pDownload->CommandKey);
	p_soap_cwmp1__TransferComplete->StartTime 					= download_startTime;
	p_soap_cwmp1__TransferComplete->CompleteTime				= download_completeTime;

	sprintf(startTime,"%li",p_soap_cwmp1__TransferComplete->StartTime);
	sprintf(completeTime,"%li",download_completeTime);

	backup_session_insert_rpc("TransferComplete",p_soap_cwmp1__TransferComplete->CommandKey,RPC_NO_STATUS);
	backup_session_insert_parameter("StartTime",startTime,"rpc","TransferComplete",-1,p_soap_cwmp1__TransferComplete->CommandKey);
	backup_session_insert_parameter("CompleteTime",completeTime,"rpc","TransferComplete",-1,p_soap_cwmp1__TransferComplete->CommandKey);

	if(error != FAULT_CPE_NO_FAULT_IDX)
	{
		p_soap_cwmp1__TransferComplete->FaultStruct 				= calloc(1,sizeof(struct cwmp1__FaultStruct));
		p_soap_cwmp1__TransferComplete->FaultStruct->FaultCode		= strdup(FAULT_CPE_ARRAY[error].CODE);
		p_soap_cwmp1__TransferComplete->FaultStruct->FaultString	= strdup(FAULT_CPE_ARRAY[error].DESCRIPTION);

		backup_session_insert_parameter("FaultCode",p_soap_cwmp1__TransferComplete->FaultStruct->FaultCode,"rpc","TransferComplete",-1,p_soap_cwmp1__TransferComplete->CommandKey);
	}

    return error;
}

int cwmp_download_firmware (struct cwmp *cwmp, struct session *session)
{
	int	error = FAULT_CPE_NO_FAULT_IDX;

	error = uci_upgrade_image(cwmp,session);

	return error;
}

int cwmp_download_web_content (struct cwmp *cwmp, struct session *session)
{
	int	error = FAULT_CPE_NO_FAULT_IDX;

	error = uci_apply_web_packages();

	return error;
}

int cwmp_download_configuration_file (struct cwmp *cwmp, struct session *session)
{
	int	error = FAULT_CPE_NO_FAULT_IDX;

	error = uci_apply_configuration();
	if(error == FAULT_CPE_NO_FAULT_IDX)
	{
		if(session != NULL)
		{
			add_session_end_func(session,cwmp_reboot,NULL,TRUE);
		}
		else
		{
			add_download_end_func(cwmp_reboot,NULL);
		}
	}

	return error;
}

int cwmp_free_download_request(struct download *download)
{
	if(download != NULL)
	{
		if(download->CommandKey != NULL)
		{
			free(download->CommandKey);
		}
		if(download->FileType != NULL)
		{
			free(download->FileType);
		}
		if(download->URL != NULL)
		{
			free(download->URL);
		}
		if(download->Username != NULL)
		{
			free(download->Username);
		}
		if(download->Password != NULL)
		{
			free(download->Password);
		}
		free(download);
	}
	return CWMP_OK;
}

int add_download_end_func (int (*func)(struct cwmp *cwmp, void *input),void *input)
{
    download_end_func = calloc(1,sizeof(struct download_end_func));
    if(download_end_func == NULL)
    {
    	return CWMP_MEM_ERR;
    }
    download_end_func->func  = func;
    download_end_func->input = input;

    return CWMP_OK;
}

int run_download_end_func (struct cwmp *cwmp)
{
    int                         error = CWMP_OK;

    if(download_end_func != NULL)
    {
    	error = download_end_func->func(cwmp,download_end_func->input);
    	free (download_end_func);
    	download_end_func = NULL;
    }

    return error;
}

int cwmp_scheduledDownload_remove_all()
{
	struct download          					*download;

	pthread_mutex_lock (&mutex_download);
	while (list_download.next!=&(list_download))
	{
		download = list_entry(list_download.next,struct download, list);
		list_del (&(download->list));
		backup_session_delete_download(download->CommandKey,download->URL,download->scheduled_time);
		count_download_queue--;
		cwmp_free_download_request(download);
	}
	pthread_mutex_unlock (&mutex_download);

	return CWMP_OK;
}
