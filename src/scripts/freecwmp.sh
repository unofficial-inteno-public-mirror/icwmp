#!/bin/sh
# Copyright (C) 2011-2012 Luka Perkov <freecwmp@lukaperkov.net>

. /lib/functions.sh
. /usr/share/shflags/shflags.sh
. /usr/share/freecwmp/defaults

# define a 'name' command-line string flag
DEFINE_boolean 'newline' false 'do not output the trailing newline' 'n'
DEFINE_boolean 'value' false 'output values only' 'v'
DEFINE_boolean 'empty' false 'output empty parameters' 'e'
DEFINE_boolean 'last' false 'output only last line ; for parameters that tend to have huge output' 'l'
DEFINE_boolean 'debug' false 'give debug output' 'd'
DEFINE_boolean 'dummy' false 'echo system commands' 'D'
DEFINE_boolean 'force' false 'force getting values for certain parameters' 'f'
DEFINE_string 'url' '' 'file to download [download only]' 'u'
DEFINE_string 'size' '' 'size of file to download [download only]' 's'
DEFINE_string 'type' '' 'type of file to download [download only]' 't'
DEFINE_string 'user' '' 'username for downloading file [download only]' 'a'
DEFINE_string 'pass' '' 'password for downloading file [download only]' 'p'
DEFINE_string 'delay' '' 'scheduled_time for downloading file [download only]' 'y'

FLAGS_HELP=`cat << EOF
USAGE: $0 [flags] command [parameter] [values]
command:
  get [value|notification|tags|all]
  set [value|notification|tag]
  download
  factory_reset
  reboot
EOF`

FLAGS "$@" || exit 1
eval set -- "${FLAGS_ARGV}"

if [ ${FLAGS_help} -eq ${FLAGS_TRUE} ]; then
	exit 1
fi

if [ ${FLAGS_newline} -eq ${FLAGS_TRUE} ]; then
	ECHO_newline='-n'
fi

case "$1" in
	set)
		if [ "$2" = "notification" ]; then
			__arg1="$3"
			__arg2="$4"
			__arg3="`echo $5| tr '[A-Z]' '[a-z]'`"
			action="set_notification"
		elif [ "$2" = "tag" ]; then
			__arg1="$3"
			__arg2="$4"
			action="set_tag"
		elif [ "$2" = "value" ]; then
			__arg1="$3"
			__arg2="$4"
			action="set_value"
		else
			__arg1="$2"
			__arg2="$3"
			action="set_value"
		fi
		;;
	get)
		if [ "$2" = "notification" ]; then
			__arg1="$3"
			action="get_notification"
		elif [ "$2" = "tags" ]; then
			__arg1="$3"
			action="get_tags"
		elif [ "$2" = "value" ]; then
			__arg1="$3"
			action="get_value"
		elif [ "$2" = "name" ]; then
			__arg1="$3"
			__arg2=`echo $4| tr '[A-Z]' '[a-z]'`
			action="get_name"
		elif [ "$2" = "all" ]; then
			__arg1="$3"
			action="get_all"
		else
			__arg1="$2"
			action="get_value"
		fi
		;;
	download)
		action="download"
		;;
	factory_reset)
		action="factory_reset"
		;;
	reboot)
		action="reboot"
		;;
	apply)
		if [ "$2" = "notification" ]; then
			action="apply_notification"
		elif [ "$2" = "value" ]; then
			action="apply_value"
		elif [ "$2" = "download" ]; then
			action="apply_download"
		else
			action="apply_value"
		fi
		;;
	add)
		if [ "$2" = "object" ]; then
			action="add_object"
			__arg1="$3"
		else
			action="add_object"
			__arg1="$3"
		fi
		;;
	delete)
		if [ "$2" = "object" ]; then
			action="delete_object"
			__arg1="$3"
		else
			action="delete_object"
			__arg1="$3"
		fi
		;;
	inform)
		action="inform"
		;;
	end_session)
		action="end_session"
		;;
esac

if [ -z "$action" ]; then
	echo invalid action \'$1\'
	exit 1
fi

if [ ${FLAGS_debug} -eq ${FLAGS_TRUE} ]; then
	echo "[debug] started at \"`date`\""
fi

load_script() {
	. $1 
}

get_value_functions=""
set_value_functions=""
handle_scripts() {
	local section="$1"
	config_get prefix "$section" "prefix"
	config_list_foreach "$section" 'location' load_script
	config_get get_value_functions "$section" "get_value_function"
	config_get get_name_functions "$section" "get_name_function"
	config_get get_notification_functions "$section" "get_notification_function"
	config_get set_value_functions "$section" "set_value_function"
	config_get set_notification_functions "$section" "set_notification_function"
	config_get add_object_functions "$section" "add_object_function"
	config_get delete_object_functions "$section" "delete_object_function"
}

config_load cwmp
config_foreach handle_scripts "scripts"

# Fault code

FAULT_CPE_NO_FAULT="0"
FAULT_CPE_REQUEST_DENIED="1"
FAULT_CPE_INTERNAL_ERROR="2"
FAULT_CPE_INVALID_ARGUMENTS="3"
FAULT_CPE_RESOURCES_EXCEEDED="4"
FAULT_CPE_INVALID_PARAMETER_NAME="5"
FAULT_CPE_INVALID_PARAMETER_TYPE="6"
FAULT_CPE_INVALID_PARAMETER_VALUE="7"
FAULT_CPE_NON_WRITABLE_PARAMETER="8"
FAULT_CPE_NOTIFICATION_REJECTED="9"
FAULT_CPE_DOWNLOAD_FAILURE="10"
FAULT_CPE_UPLOAD_FAILURE="11"
FAULT_CPE_FILE_TRANSFER_AUTHENTICATION_FAILURE="12"
FAULT_CPE_FILE_TRANSFER_UNSUPPORTED_PROTOCOL="13"
FAULT_CPE_DOWNLOAD_FAIL_MULTICAST_GROUP="14"
FAULT_CPE_DOWNLOAD_FAIL_CONTACT_SERVER="15"
FAULT_CPE_DOWNLOAD_FAIL_ACCESS_FILE="16"
FAULT_CPE_DOWNLOAD_FAIL_COMPLETE_DOWNLOAD="17"
FAULT_CPE_DOWNLOAD_FAIL_FILE_CORRUPTED="18"
FAULT_CPE_DOWNLOAD_FAIL_FILE_AUTHENTICATION="19"

if [ "$action" = "get_value" -o "$action" = "get_all" ]; then
	no_fault="0"
	freecwmp_check_fault "$__arg1"
	fault_code="$?"
	if [ "$fault_code" = "0" ]; then
		if [ \( "$__arg1" = "InternetGatewayDevice." \) -o \( "$__arg1" = "" \) ]; then
			__arg1="InternetGatewayDevice."
		fi
		for function_name in $get_value_functions
		do
			$function_name "$__arg1"
				fault_code="$?"
				if [ "$fault_code" = "0" ]; then
					no_fault="1"
				fi
				if [ "$fault_code" = "$FAULT_CPE_INVALID_ARGUMENTS" ]; then
					break
				fi
		done
		if [ "$no_fault" = "1" ]; then fault_code="0"; fi
	fi
	if [ "$fault_code" != "0" ]; then
		let fault_code=$fault_code+9000
		ubus_freecwmp_output "$__arg1" "" "" "$fault_code"
	fi
fi

if [ "$action" = "get_name" -o "$action" = "get_all" ]; then
	no_fault="0"
	freecwmp_check_fault "$__arg1"
	fault_code="$?"
	if [ "$fault_code" = "0" ]; then
		if [ \( "$__arg2" != "0" \) -a \( "$__arg2" != "1" \) -a \( "$__arg2" != "true" \) -a \( "$__arg2" != "false" \) ]; then
			fault_code="$FAULT_CPE_INVALID_ARGUMENTS"
		else
			if [ "$__arg2" = "true" ]; then
				__arg2="1"
			elif [ "$__arg2" = "false" ]; then
				__arg2="0"
			fi
		fi
		if [ "$fault_code" = "0" ]; then
			if [ \( "$__arg1" = "InternetGatewayDevice." \) -o \( "$__arg1" = "" \) ]; then
				ubus_freecwmp_output "InternetGatewayDevice." "0"
				if [ \( "$__arg1" = "" \) -a \( "$__arg2" = "1" \) ]; then
					exit 0
				fi
				__arg1="InternetGatewayDevice."
			fi
			for function_name in $get_name_functions
			do
				$function_name "$__arg1" "$__arg2"
				fault_code="$?"
				if [ "$fault_code" = "0" ]; then
					no_fault="1"
				fi
				if [ "$fault_code" = "$FAULT_CPE_INVALID_ARGUMENTS" ]; then
					break
				fi
	done
			if [ "$no_fault" = "1" ]; then fault_code="0"; fi
		fi
	fi
	if [ "$fault_code" != "0" ]; then
		let fault_code=$fault_code+9000
		ubus_freecwmp_output "$__arg1" "" "" "$fault_code"
	fi
fi

if [ "$action" = "set_value" ]; then
	no_fault="0"
	freecwmp_check_fault "$__arg1"
	fault_code="$?"
	if [ "$fault_code" = "0" ]; then
		if [ \( "$__arg1" = "InternetGatewayDevice." \) -o \( "$__arg1" = "" \) ]; then
			__arg1="InternetGatewayDevice."
		fi
		for function_name in $set_value_functions
		do
			$function_name "$__arg1" "$__arg2"
			fault_code="$?"
			if [ "$fault_code" = "0" ]; then
				no_fault="1"
			fi
			if [ "$fault_code" = "$FAULT_CPE_INVALID_ARGUMENTS" ]; then
				break
			fi
		done
		if [ "$no_fault" = "1" ]; then fault_code="0"; fi
	fi
	if [ "$fault_code" != "0" ]; then
		let fault_code=$fault_code+9000
		freecwmp_set_parameter_fault "$__arg1" "$fault_code"
	fi
fi

if [ "$action" = "get_notification" -o "$action" = "get_all" ]; then
	no_fault="0"
	freecwmp_check_fault "$__arg1"
	fault_code="$?"
	if [ "$fault_code" = "0" ]; then
		if [ \( "$__arg1" = "InternetGatewayDevice." \) -o \( "$__arg1" = "" \) ]; then
			__arg1="InternetGatewayDevice."
		fi
		for function_name in $get_notification_functions
		do
			$function_name "$__arg1"
			fault_code="$?"
			if [ "$fault_code" = "0" ]; then
				no_fault="1"
			fi
			if [ "$fault_code" = "$FAULT_CPE_INVALID_ARGUMENTS" ]; then
				break
			fi
		done
		if [ "$no_fault" = "1" ]; then fault_code="0"; fi
	fi
	if [ "$fault_code" != "0" ]; then
		let fault_code=$fault_code+9000
		ubus_freecwmp_output "$__arg1" "" "" "$fault_code"
	fi
fi

if [ "$action" = "set_notification" ]; then
	if [ "$__arg3" = "true" ]; then
		__arg3="1"
	elif [ "$__arg3" = "false" ]; then
		__arg3="0"
	fi
	if [ "$__arg3" = "1" ]; then
		no_fault="0"
		freecwmp_check_fault "$__arg1"
		fault_code="$?"
		if [ "$fault_code" = "0" ]; then
			if [ \( "$__arg1" = "InternetGatewayDevice." \) -o \( "$__arg1" = "" \) ]; then
				__arg1="InternetGatewayDevice."
			fi
			for function_name in $set_notification_functions
			do
				$function_name "$__arg1" "$__arg2"
				fault_code="$?"
				if [ "$fault_code" = "0" ]; then
					no_fault="1"
				fi
				if [ "$fault_code" = "$FAULT_CPE_INVALID_ARGUMENTS" ]; then
					break
				fi
			done
			if [ "$no_fault" = "1" ]; then fault_code="0"; fi
		fi
		if [ "$fault_code" != "0" ]; then
			let fault_code=$fault_code+9000
			freecwmp_set_parameter_fault "$__arg1" "$fault_code"
		fi
	fi
fi

if [ "$action" = "get_tags" -o "$action" = "get_all" ]; then
	freecwmp_get_parameter_tags "x_tags" "$__arg1"
	freecwmp_tags_output "$__arg1" "$x_tags"
fi

if [ "$action" = "set_tag" ]; then
	freecwmp_set_parameter_tag "$__arg1" "$__arg2"
	/sbin/uci ${UCI_CONFIG_DIR:+-c $UCI_CONFIG_DIR} commit
fi

if [ "$action" = "download" ]; then
	local fault_code="9000"
	if [ "${FLAGS_user}" = "" -o "${FLAGS_pass}" = "" ];then
		wget -O /tmp/freecwmp_download "${FLAGS_url}" > /dev/null
		if [ "$?" != "0" ];then
			let fault_code=$fault_code+$FAULT_CPE_DOWNLOAD_FAILURE
			ubus_freecwmp_fault_output "" "$fault_code"
			exit 1
		fi
	else
		local url="http://${FLAGS_user}:${FLAGS_pass}@`echo ${FLAGS_url}|sed 's/http:\/\///g'`"
		wget -O /tmp/freecwmp_download "$url" > /dev/null
		if [ "$?" != "0" ];then
			let fault_code=$fault_code+$FAULT_CPE_DOWNLOAD_FAILURE
			ubus_freecwmp_fault_output "" "$fault_code"
			exit 1
		fi
	fi

	dl_size=`ls -l /tmp/freecwmp_download | awk '{ print $5 }'`
	if [ ! "$dl_size" -eq "${FLAGS_size}" ]; then
		let fault_code=$fault_code+$FAULT_CPE_DOWNLOAD_FAILURE
		rm /tmp/freecwmp_download 2> /dev/null
		ubus_freecwmp_fault_output "" "$fault_code"
	else
		if [ "${FLAGS_type}" = "1" ];then
			mv /tmp/freecwmp_download /tmp/firmware_upgrade_image 2> /dev/null
			freecwmp_check_image
			if [ "$?" = "0" ];then
				local flashsize="`freecwmp_check_flash_size`"
				local filesize="$dl_size"
				if [ $flashsize -gt 0 -a $filesize -gt $flashsize ];then
					let fault_code=$fault_code+$FAULT_CPE_DOWNLOAD_FAIL_FILE_CORRUPTED
					rm /tmp/firmware_upgrade_image 2> /dev/null
					ubus_freecwmp_fault_output "" "$fault_code"
				else
					rm /tmp/firmware_upgrade_image_last_valid 2> /dev/null
					mv /tmp/firmware_upgrade_image /tmp/firmware_upgrade_image_last_valid 2> /dev/null
					ubus_freecwmp_fault_output "" "$FAULT_CPE_NO_FAULT"
					if [ "${FLAGS_delay}" = "0" ];then
						echo "/bin/sh /usr/sbin/freecwmp apply download \"${FLAGS_type}\"" > /tmp/end_session.sh
						ubus ${UBUS_SOCKET:+-s $UBUS_SOCKET} call tr069 command '{ "name": "action_end_session" }' 2> /dev/null
					fi
				fi
			else
				let fault_code=$fault_code+$FAULT_CPE_DOWNLOAD_FAIL_FILE_CORRUPTED
				rm /tmp/firmware_upgrade_image 2> /dev/null
				ubus_freecwmp_fault_output "" "$fault_code"
			fi
		elif [ "${FLAGS_type}" = "2" ];then
			mv /tmp/freecwmp_download /tmp/web_content.ipk 2> /dev/null
			ubus_freecwmp_fault_output "" "$FAULT_CPE_NO_FAULT"
			if [ "${FLAGS_delay}" = "0" ];then
				echo "/bin/sh /usr/sbin/freecwmp apply download \"${FLAGS_type}\"" > /tmp/end_session.sh
				ubus ${UBUS_SOCKET:+-s $UBUS_SOCKET} call tr069 command '{ "name": "action_end_session" }' 2> /dev/null
			fi
		elif [ "${FLAGS_type}" = "3" ];then
			mv /tmp/freecwmp_download /tmp/vendor_configuration_file.cfg 2> /dev/null
			ubus_freecwmp_fault_output "" "$FAULT_CPE_NO_FAULT"
			if [ "${FLAGS_delay}" = "0" ];then
				echo "/bin/sh /usr/sbin/freecwmp apply download \"${FLAGS_type}\"" > /tmp/end_session.sh
				ubus ${UBUS_SOCKET:+-s $UBUS_SOCKET} call tr069 command '{ "name": "action_end_session" }' 2> /dev/null
			fi
		else
			let fault_code=$fault_code+$FAULT_CPE_DOWNLOAD_FAILURE
			ubus_freecwmp_fault_output "" "$fault_code"
			rm /tmp/freecwmp_download 2> /dev/null
		fi
	fi
fi

if [ "$action" = "apply_download" ]; then
	case "${FLAGS_type}" in
		1) freecwmp_apply_firmware ;;
		2) freecwmp_apply_web_content ;;
		3) freecwmp_apply_vendor_configuration ;;
	esac
fi

if [ "$action" = "factory_reset" ]; then
	if [ ${FLAGS_dummy} -eq ${FLAGS_TRUE} ]; then
		echo "# factory_reset"
	else
		jffs2_mark_erase "rootfs_data"
		sync
		ubus ${UBUS_SOCKET:+-s $UBUS_SOCKET} call tr069 command '{ "name": "reboot_end_session" }' 2> /dev/null
	fi
fi

if [ "$action" = "reboot" ]; then
	if [ ${FLAGS_dummy} -eq ${FLAGS_TRUE} ]; then
		echo "# reboot"
	else
		sync
		ubus ${UBUS_SOCKET:+-s $UBUS_SOCKET} call tr069 command '{ "name": "reboot_end_session" }' 2> /dev/null
	fi
fi

if [ \( "$action" = "apply_notification" \) -o \( "$action" = "apply_value" \) ]; then
	__fault_count=`cat /var/state/cwmp 2> /dev/null |wc -l 2> /dev/null`
	let __fault_count=$__fault_count/3
	if [ "$__fault_count" = "0" ]; then
		# applying
		/sbin/uci ${UCI_CONFIG_DIR:+-c $UCI_CONFIG_DIR} commit
		if [ "$action" = "apply_value" ]; then ubus call tr069 SetParameterValuesStatus '{"status": "0"}'; fi
		if [ "$action" = "apply_notification" ]; then ubus call tr069 SetParameterAttributes '{"success": "0", "fault_code":""}' 2> /dev/null; fi
	else
		let n=$__fault_count-1
		for i in `seq 0 $n`
		do
			local parm=`/sbin/uci ${UCI_CONFIG_DIR:+-c $UCI_CONFIG_DIR} -P /var/state get cwmp.@fault[$i].parameter 2> /dev/null`
			local fault_code=`/sbin/uci ${UCI_CONFIG_DIR:+-c $UCI_CONFIG_DIR} -P /var/state get cwmp.@fault[$i].fault_code 2> /dev/null`
			ubus_freecwmp_fault_output "$parm" "$fault_code"
			if [ "$action" = "apply_notification" ]; then break; fi
		done
		rm -rf /var/state/cwmp 2> /dev/null
		/sbin/uci ${UCI_CONFIG_DIR:+-c $UCI_CONFIG_DIR} revert cwmp
	fi
fi

if [ "$action" = "add_object" ]; then
	no_fault="0"
	freecwmp_check_fault "$__arg1"
	fault_code="$?"
	if [ "$fault_code" = "0" ]; then
		if [ \( "$__arg1" = "InternetGatewayDevice." \) -o \( "$__arg1" = "" \) ]; then
			__arg1="InternetGatewayDevice."
		fi
		for function_name in $add_object_functions
		do
			$function_name "$__arg1"
			fault_code="$?"
			if [ "$fault_code" = "0" ]; then
				no_fault="1"
			fi
			if [ "$fault_code" = "$FAULT_CPE_INVALID_ARGUMENTS" ]; then
				break
			fi
		done
		if [ "$no_fault" = "1" ]; then fault_code="0"; fi
	fi
	if [ "$fault_code" != "0" ]; then
		let fault_code=$fault_code+9000
		ubus_freecwmp_output "" "" "" "$fault_code"
	fi
fi

if [ "$action" = "delete_object" ]; then
	no_fault="0"
	freecwmp_check_fault "$__arg1"
	fault_code="$?"
	if [ "$fault_code" = "0" ]; then
		if [ \( "$__arg1" = "InternetGatewayDevice." \) -o \( "$__arg1" = "" \) ]; then
			__arg1="InternetGatewayDevice."
		fi
		for function_name in $delete_object_functions
		do
			$function_name "$__arg1"
			fault_code="$?"
			if [ "$fault_code" = "0" ]; then
				no_fault="1"
			fi
			if [ "$fault_code" = "$FAULT_CPE_INVALID_ARGUMENTS" ]; then
				break
			fi
		done
		if [ "$no_fault" = "1" ]; then fault_code="0"; fi
	fi
	if [ "$fault_code" != "0" ]; then
		let fault_code=$fault_code+9000
		ubus_freecwmp_output "" "" "" "$fault_code"
	fi
fi

if [ "$action" = "inform" ]; then
	action="get_value"
	
	get_device_info_manufacturer
	get_device_info_oui
	get_device_info_product_class
	get_device_info_serial_number
	get_device_info_hardware_version
	get_device_info_software_version
	get_device_info_generic "InternetGatewayDevice.DeviceInfo.ProvisioningCode"
	get_wan_device_mng_interface_ip
	get_management_server_connection_request_url
	get_management_server_parameter_key
fi

if [ "$action" = "end_session" ]; then	
	/bin/sh /tmp/end_session.sh
	rm /tmp/end_session.sh
fi

if [ ${FLAGS_debug} -eq ${FLAGS_TRUE} ]; then
	echo "[debug] exited at \"`date`\""
fi
