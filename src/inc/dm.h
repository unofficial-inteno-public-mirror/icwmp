/*
    dm.h

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
#include <linux/types.h>
#define DM_OK                                               0
#define DM_ERR                                              1
#define DM_GEN_ERR                                          2
#define DM_MEM_ERR                                          3
#define DM_ORF_ERR                                          4
#define NODE_PATH_SIZE                                      16
#define INDEX_PATH_SIZE                                     8

#define COUNT_TYPE_VALUES                                   44
#define SYSTEM_CMD_TIMEOUT                                  10

#define TYPE_VALUE_string_IDX                   			0
#define TYPE_VALUE_int_IDX                      			1
#define TYPE_VALUE_unsignedInt_IDX              			2
#define TYPE_VALUE_boolean_IDX                  			3
#define TYPE_VALUE_dateTime_IDX                 			4
#define TYPE_VALUE_base64Binary_IDX             			5
#define TYPE_VALUE_integer_IDX                  			6
#define TYPE_VALUE_unsignedByte_IDX             			7
#define TYPE_VALUE_unsignedLong_IDX             			8
#define TYPE_VALUE_unsignedShort_IDX            			9
#define TYPE_VALUE_anyURI_IDX                   			10
#define TYPE_VALUE_byte_IDX                     			11
#define TYPE_VALUE_date_IDX                     			12
#define TYPE_VALUE_time_IDX                     			13
#define TYPE_VALUE_decimal_IDX                  			14
#define TYPE_VALUE_double_IDX                   			15
#define TYPE_VALUE_duration_IDX                 			16
#define TYPE_VALUE_ENTITIES_IDX                 			17
#define TYPE_VALUE_ENTITY_IDX                   			18
#define TYPE_VALUE_float_IDX                    			19
#define TYPE_VALUE_gDay_IDX                     			20
#define TYPE_VALUE_gMonth_IDX                   			21
#define TYPE_VALUE_gMonthDay_IDX                			22
#define TYPE_VALUE_gYear_IDX                    			23
#define TYPE_VALUE_gYearMonth_IDX               			24
#define TYPE_VALUE_hexBinary_IDX                			25
#define TYPE_VALUE_ID_IDX                       			26
#define TYPE_VALUE_IDREF_IDX                    			27
#define TYPE_VALUE_IDREFS_IDX                   			28
#define TYPE_VALUE_language_IDX                 			29
#define TYPE_VALUE_long_IDX                     			30
#define TYPE_VALUE_Name_IDX                     			31
#define TYPE_VALUE_NCName_IDX                   			32
#define TYPE_VALUE_negativeInteger_IDX          			33
#define TYPE_VALUE_NMTOKEN_IDX                  			34
#define TYPE_VALUE_NMTOKENS_IDX                 			35
#define TYPE_VALUE_nonNegativeInteger_IDX       			36
#define TYPE_VALUE_nonPositiveInteger_IDX       			37
#define TYPE_VALUE_normalizedString_IDX         			38
#define TYPE_VALUE_NOTATION_IDX                 			39
#define TYPE_VALUE_positiveInteger_IDX          			40
#define TYPE_VALUE_QName_IDX                    			41
#define TYPE_VALUE_short_IDX                    			42
#define TYPE_VALUE_token_IDX                    			43

typedef struct dm_notification {
    __u8 can_deny               :1;
    __u8 force_enabled          :1;
    __u8 force_default_enabled  :1;
} dm_notification;

typedef enum node_type {
    DM_OBJECT,
    DM_INSTANCE,
    DM_PARAMETER
} node_type;

typedef enum param_type {
    DM_SYSTEM,
    DM_UCI,
    DM_STRING,
    DM_DEBUG,
    DM_CORRESPONDENCE,
    DM_DEFAULT_VALUE,
    DM_REGEXP
} param_type;

typedef enum action_type {
    DM_APPLY,
    DM_GET,
    DM_SET,
    DM_CANCEL,
    DM_ADD,
    DM_DEL
} action_type;

typedef enum permission {
    DM_READ,
    DM_READ_WRITE,
    DM_CREATE,
    DM_PRESENT
} permission;

typedef struct dm_data {
    unsigned short          line_size;
    unsigned short          column_size;
    char                    ***data;
} dm_data;

typedef struct dm_map {
    unsigned short          size;
    char                    **map;
} dm_map;

typedef struct dm_uci {
    __u8                    reboot_required;
    __u8                    end_session;
    char                    *cmd;
} dm_uci;

typedef struct dm_system {
    __u8                    type;
    __u8                    reboot_required;
    __u8                    end_session;
    struct dm_map           *dm_map;
    char                    *cmd;
} dm_system;

typedef struct dm_correspondence {
    struct dm_data          *dm_data;
    struct dm_map           *dm_map;
} dm_correspondence;

typedef struct dm_debug {
    char                    *data;
} dm_debug;

typedef struct data_handler {
    enum param_type         type;
    void                    *handler;
} data_handler;

struct list_data_handler {
    struct list_head        list;
    struct data_handler     *data_handler;
} list_data_handler;

typedef struct dm_node {
    struct list_head        list;
    char                    *name;
    struct data_handler     **data_handler;
    __u8                    type:4;
    __u8                    permission:4;
    __u8                    size_data_handler;
    struct dm_notification  active_notify;
    /* the position of above parameters should be the same of struct dm_node_leaf*/
    struct list_head        head_child;
} dm_node;

typedef struct dm_node_leaf {
    struct list_head        list;
    char                    *name;
    struct data_handler     **data_handler;
    __u8                    type:4;
    __u8                    permission:4;
    __u8                    size_data_handler;
    struct dm_notification  active_notify;
    /* the position of above parameters should be the same of struct dm_node*/
    __u8                    value_type;
} dm_node_leaf;

typedef struct forced_inform_parameter {
    char                    *name;
    struct list_head        list;
    struct dm_node_leaf     *node;
} _forced_inform_parameter;

enum dm_data_type {
    DM_XML_DATA,
    DM_SYSTEM_DATA
};

typedef struct dm_index_path {
    struct dm_map           *map;
    struct dm_data          *data;
    enum dm_data_type       data_type;
    char                    *index;
    int                     indice;
    struct dm_node_path     *node_path;
} dm_index_path;

typedef struct dm_node_path {
    struct dm_node          *node;
    struct dm_index_path    *index_path;
} dm_node_path;

typedef struct dm_rpc {
    void                    *input;
    int                     (*method)(struct dm_node *node, struct dm_rpc *dm_rpc, struct dm_node_path *node_path,  int pos_np, struct dm_index_path *index_path, int pos_xp);
    bool                    subtree;
} dm_rpc;

#ifdef WITH_DM_DEBUG
# ifndef DM_LOG
#  define DM_LOG(SEV,MESSAGE,args...) puts_log(SEV,MESSAGE,##args);
# endif
#else
# define DM_LOG(SEV,MESSAGE,args...)
#endif

#ifdef WITH_DM_XML_DEBUG
# ifndef DM_XML_LOG
#  define DM_XML_LOG(SEV,MESSAGE,args...) puts_log(SEV,MESSAGE,##args);
#  define DM_XML_GET_HANDLER_DEBUG(NODE,INDEX_PATH,POS_XP) dm_get_handler_debug(NODE,INDEX_PATH,POS_XP);
# endif
#else
# define DM_XML_LOG(SEV,MESSAGE,args...)
# define DM_XML_GET_HANDLER_DEBUG(NODE,INDEX_PATH,POS_XP)
#endif
