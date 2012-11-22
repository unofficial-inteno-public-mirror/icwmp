/*
    dm_rpc.h

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

typedef struct dm_indice {
    int                                 type;
    char                                *indice;
} dm_indice;

typedef struct sub_path {
    char                                *name;
    struct dm_node                      *node;
    struct dm_indice                    dm_indice;
} sub_path;

typedef struct dm_input_getParameterValues {
    int                                 *n;
    struct list_head                    *list;
} dm_input_getParameterValues;

typedef struct dm_input_getParameterNames {
    int                                 *n;
    struct list_head                    *list;
    enum xsd__boolean                   NextLevel;
} dm_input_getParameterNames;

typedef struct dm_input_getParameterAttributes {
    int                                 *n;
    struct list_head                    *list;
} dm_input_getParameterAttributes;

typedef struct dm_input_setParameterAttributes {
    struct cwmp1__SetParameterAttributesStruct  *ParameterAttributesStruct;
} dm_input_setParameterAttributes;

typedef struct dm_input_getParameterPaths_by_correspondence {
    struct list_head                    *list;
    struct list_head                    *elist;
    int                                 *n;
    struct dm_node                      *prefix_node;
    struct sub_path                     *sub_path;
    int                                 sub_path_size;
    bool                                get_value;
    bool                                get_attribute;
    bool                                is_actif;
} dm_input_getParameterPaths_by_correspondence;

typedef struct dm_input_setParameterValues {
    char                                *value;
    struct dm_set_handler               *dm_set_handler;
} dm_input_setParameterValues;

typedef struct dm_input_addObject {
    int                                 *n;
    char                                *path;
    struct dm_set_handler               *dm_set_handler;
} dm_input_addObject;

typedef struct dm_input_deleteObject {
    char                                *path;
    struct dm_set_handler               *dm_set_handler;
} dm_input_deleteObject;

typedef struct handler_ParameterInfoStruct {
    struct list_head                    list;
    struct cwmp1__ParameterInfoStruct   *ParameterInfoStruct;
} handler_ParameterInfoStruct;

typedef struct handler_ParameterValueStruct {
    struct list_head                    list;
    struct cwmp1__ParameterValueStruct  *ParameterValueStruct;
} handler_ParameterValueStruct;

typedef struct dm_set_handler {
    struct list_head                    list;
    struct list_head                    cmd_list;
    struct list_head                    cancel_list;
    struct list_head                    service_list;
    bool                                reboot_required;
    bool                                cwmp_reload;
    bool                                uci;
} dm_set_handler;

typedef struct service_handler {
    struct list_head                    list;
    char                                *service;
} service_handler;

typedef struct cmd_handler {
    struct list_head                    list;
    char                                *cmd;
    enum param_type                     type;
    __u8                                end_session;
} cmd_handler;

typedef struct cancel_handler {
    struct list_head                    list;
    char                                *cmd;
} cancel_handler;

typedef struct handler_ParameterAttributeStruct {
    struct list_head                        list;
    struct cwmp1__ParameterAttributeStruct  *ParameterAttributeStruct;
} handler_ParameterAttributeStruct;

#define DM_INDICE_INDEX                 1
#define DM_INDICE_CORRESPONDENCE        2
#define DM_MAX_INDICE                   16
