/*
    kcwmp.c

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

#include <linux/module.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <stdarg.h>
#include <linux/slab.h>
#include "inc/cwmp_kernel.h"

typedef struct kernel_cwmp_input {
    char                        **argv;
    int                         argc;
    int                         handler;
    struct mutex                mutex;
    wait_queue_head_t           thresholdq;
} KERNEL_CWMP_INPUT;

static struct sock *nl_sk = NULL;
static struct kernel_cwmp_input kernel_cwmp_input;

MODULE_LICENSE("INTENO");

static void kernel_api_cwmp_value_change_listener(struct sk_buff *skb)
{

    struct nlmsghdr         *nlh;
    int                     pid;
    struct sk_buff          *skb_out;
    int                     msg_size;
    char                    *msg;
    char                    *recv;
    int                     i,res;

    nlh     = (struct nlmsghdr*)skb->data;
    recv    = (char*)nlmsg_data(nlh);

    if (strcmp(recv,NETLINK_CWMP_ID)!=0)
    {
        return;
    }
    pid     = nlh->nlmsg_pid; /*pid of sending process */


    while (kernel_cwmp_input.argc==0)
    {
        if ( wait_event_interruptible( kernel_cwmp_input.thresholdq, kernel_cwmp_input.handler)) {
             return ;
        }
    }
    kernel_cwmp_input.handler = 0;

    mutex_lock (&(kernel_cwmp_input.mutex));

    for (i=0;i<=kernel_cwmp_input.argc;i++)
    {
        if (i<kernel_cwmp_input.argc)
        {
            msg     = kernel_cwmp_input.argv[i];
        }
        else
        {
            msg     = NETLINK_END_DATA;
        }
        msg_size    = strlen(msg);
        skb_out     = nlmsg_new(msg_size,0);
        if(!skb_out)
        {
            printk(KERN_ERR "Failed to allocate new skb\n");
            return;
        }

        nlh=nlmsg_put(skb_out,0,0,NLMSG_DONE,msg_size,0);
        NETLINK_CB(skb_out).dst_group = 0; /* not in mcast group */
        strncpy(nlmsg_data(nlh),msg,msg_size);
        res=nlmsg_unicast(nl_sk,skb_out,pid);

        if (i<kernel_cwmp_input.argc)
        {
            kfree(kernel_cwmp_input.argv[i]);
        }

    }
    if (kernel_cwmp_input.argv!=NULL)
    {
        kfree(kernel_cwmp_input.argv);
        kernel_cwmp_input.argv = NULL;
    }

    kernel_cwmp_input.argc = 0;
    mutex_unlock (&(kernel_cwmp_input.mutex));
}

static int /*__init*/ kernel_api_cwmp_value_change_init(void)
{

    printk(KERN_INFO "Entering kernel cwmp module: %s\n",__FUNCTION__);
    memset (&kernel_cwmp_input, 0, sizeof(struct kernel_cwmp_input));
    init_waitqueue_head(&kernel_cwmp_input.thresholdq);
    mutex_init(&kernel_cwmp_input.mutex);
    nl_sk=netlink_kernel_create(&init_net, NETLINK_USER, 0, kernel_api_cwmp_value_change_listener, NULL, THIS_MODULE);
    if(!nl_sk)
    {
        printk(KERN_ALERT "Error creating socket.\n");
        return -10;
    }
    return 0;
}

static void /*__exit*/ kernel_api_cwmp_value_change_exit(void)
{
    printk(KERN_INFO "Exiting kernel cwmp module: %s\n",__FUNCTION__);
    netlink_kernel_release(nl_sk);
}

int kernel_api_cwmp_value_change_call (int count, ...)
{
    int             i;
    va_list         args;
    char            *s;

    if (kernel_cwmp_input.argc>0)
    {
        kernel_cwmp_input.handler = 1;
        wake_up_interruptible(&(kernel_cwmp_input.thresholdq));
        return 1;
    }

    mutex_lock (&(kernel_cwmp_input.mutex));
    kernel_cwmp_input.argv = kmalloc(count*sizeof(char *),GFP_KERNEL);

    if (kernel_cwmp_input.argv==NULL)
    {
        goto kernel_api_cwmp_error;
    }

    va_start(args, count);
    for (i=0;i<count;i++)
    {
        s = (char *) va_arg(args, char *);
        if (s==NULL)
        {
            s = NETLINK_NULL;
        }
        kernel_cwmp_input.argv[i]   = kmalloc(strlen(s),GFP_KERNEL);
        if (kernel_cwmp_input.argv[i]==NULL)
        {
            goto kernel_api_cwmp_error;
        }
        strcpy(kernel_cwmp_input.argv[i],s);
        kernel_cwmp_input.argc++;
    }
    va_end(args);
    mutex_unlock (&(kernel_cwmp_input.mutex));
    kernel_cwmp_input.handler = 1;
    wake_up_interruptible(&(kernel_cwmp_input.thresholdq));
    return 1;

kernel_api_cwmp_error:

    if (kernel_cwmp_input.argv!=NULL)
    {
        for (i=0;i<kernel_cwmp_input.argc;i++)
        {
            if (kernel_cwmp_input.argv[i]!=NULL)
            {
                kfree(kernel_cwmp_input.argv[i]);
            }
        }
        kfree(kernel_cwmp_input.argv);
        kernel_cwmp_input.argv = NULL;
    }
    kernel_cwmp_input.argc = 0;
    mutex_unlock (&(kernel_cwmp_input.mutex));
    return 1;
}
EXPORT_SYMBOL(kernel_api_cwmp_value_change_call);

module_init(kernel_api_cwmp_value_change_init); module_exit(kernel_api_cwmp_value_change_exit);

