#!/bin/sh
# Copyright (C) 2011-2012 Luka Perkov <freecwmp@lukaperkov.net>
# Copyright (C) 2013 Inteno Broadband Technology AB
#  Author Mohamed Kallel <mohamed.kallel@pivasoftware.com>
#  Author Ahmed Zribi <ahmed.zribi@pivasoftware.com>

. /lib/functions.sh
. /usr/share/libubox/jshn.sh
. /usr/share/shflags/shflags.sh
. /usr/share/freecwmp/defaults

# define a 'name' command-line string flag
DEFINE_boolean 'newline' false 'do not output the trailing newline' 'n'
DEFINE_boolean 'value' false 'output values only' 'v'
DEFINE_boolean 'json' false 'send values using json' 'j'
DEFINE_boolean 'empty' false 'output empty parameters' 'e'
DEFINE_boolean 'last' false 'output only last line ; for parameters that tend to have huge output' 'l'
DEFINE_boolean 'debug' false 'give debug output' 'd'
DEFINE_boolean 'dummy' false 'echo system commands' 'D'
DEFINE_boolean 'force' false 'force getting values for certain parameters' 'f'

FLAGS_HELP=`cat << EOF
USAGE: $0 [flags] command [parameter] [values]
command:
  get [value|notification|name|cache]
  set [value|notification]
  apply [value|notification|download]
  add [object]
  delete [object]
  download
  factory_reset
  reboot
  notify
  end_session
  inform
  wait [cache]
  clean [cache]
  json_continuous_input
EOF`

FLAGS "$@" || exit 1
eval set -- "${FLAGS_ARGV}"

if [ ${FLAGS_help} -eq ${FLAGS_TRUE} ]; then
	exit 1
fi

if [ ${FLAGS_newline} -eq ${FLAGS_TRUE} ]; then
	ECHO_newline='-n'
fi

UCI_GET="/sbin/uci ${UCI_CONFIG_DIR:+-c $UCI_CONFIG_DIR} get -q"
UCI_SET="/sbin/uci ${UCI_CONFIG_DIR:+-c $UCI_CONFIG_DIR} set -q"
UCI_BATCH="/sbin/uci ${UCI_CONFIG_DIR:+-c $UCI_CONFIG_DIR} batch -q"
UCI_ADD="/sbin/uci ${UCI_CONFIG_DIR:+-c $UCI_CONFIG_DIR} add -q"
UCI_ADD_LIST="/sbin/uci ${UCI_CONFIG_DIR:+-c $UCI_CONFIG_DIR} add_list -q"
UCI_GET_VARSTATE="/sbin/uci ${UCI_CONFIG_DIR:+-c $UCI_CONFIG_DIR} -P /var/state get -q"
UCI_SHOW="/sbin/uci ${UCI_CONFIG_DIR:+-c $UCI_CONFIG_DIR} show -q"
UCI_DELETE="/sbin/uci ${UCI_CONFIG_DIR:+-c $UCI_CONFIG_DIR} delete -q"
UCI_DEL_LIST="/sbin/uci ${UCI_CONFIG_DIR:+-c $UCI_CONFIG_DIR} del_list -q"
UCI_COMMIT="/sbin/uci ${UCI_CONFIG_DIR:+-c $UCI_CONFIG_DIR} commit -q"
UCI_RENAME="/sbin/uci ${UCI_CONFIG_DIR:+-c $UCI_CONFIG_DIR} rename -q"
NEW_LINE='\n'
cache_path="/etc/cwmpd/.cache"
tmp_cache="/tmp/.freecwmp_dm"
set_tmp_file="/tmp/.set_tmp_file"
set_fault_tmp_file="/tmp/.set_fault_tmp_file"
cache_linker_dynamic="/etc/cwmpd/.cache_linker_dynamic"
	
mkdir -p $cache_path
rm -f "$cache_path/"*"_dynamic"

case "$1" in
	set)
		if [ "$2" = "notification" ]; then
			__arg1="$3"
			__arg2="$4"
			__arg3="$5"
			action="set_notification"
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
		elif [ "$2" = "value" ]; then
			__arg1="$3"
			action="get_value"
		elif [ "$2" = "name" ]; then
			__arg1="$3"
			__arg2="$4"
			action="get_name"
		elif [ "$2" = "cache" ]; then
			__arg1="$3"
			action="get_cache"
		else
			__arg1="$2"
			action="get_value"
		fi
		;;
	download)
		__arg1="$2"
		__arg2="$3"
		__arg3="$4"
		__arg4="$5"
		__arg5="$6"
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
			__arg1="$3"
			action="apply_value"
		elif [ "$2" = "download" ]; then
			__arg1="$3"
			action="apply_download"
		else
			__arg1="$2"
			action="apply_value"
		fi
		;;
	add)
		__arg1="$3"
		__arg2="$4"
		action="add_object"
		;;
	delete)
		__arg1="$3"
		__arg2="$4"
		action="delete_object"
		;;
	inform)
		action="inform"
		;;
	notify)
		action="notify"
		__arg1="$2"
		__arg2="$3"
		__arg3="$4"
		;;
	end_session)
		action="end_session"
		;;
	allow_cr_ip)
		action="allow_cr_ip"
		__arg1="$2"
		;;
	json_continuous_input)
		action="json_continuous_input"
		;;
	end)
		echo "EOF"
		;;
	wait)
		if [ "$2" = "cache" ]; then
			action="wait_cache"
		fi
		;;
	clean)
		if [ "$2" = "cache" ]; then
			action="clean_cache"
		fi
		;;
	exit)
		exit 0
	;;
esac

if [ -z "$action" ]; then
	echo invalid action \'$1\'
	exit 1
fi

if [ ${FLAGS_debug} -eq ${FLAGS_TRUE} ]; then
	echo "[debug] started at \"`date`\""
fi

prefix_list=""
prefix_list_skip_wait_cache=""

. /lib/functions/network.sh
for ffile in `ls /usr/share/freecwmp/functions/`; do
. /usr/share/freecwmp/functions/$ffile
done

config_load cwmp

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

handle_get_cache() {
	local param="$1"
	local exact="$2"
	local pid=""
	local ls_cache=`ls $tmp_cache`
	for pid in $ls_cache; do
		if [ ! -d /proc/$pid ]; then
			rm -rf "$tmp_cache/$pid"
		fi
	done
	pid="$$"
	mkdir -p "$tmp_cache/$pid"

	for prefix in $prefix_list; do
		case $prefix in $param*)
			if [ "$exact" = "1" -a "$param" != "$prefix" ]; then continue; fi
			local f=${prefix%.}
			f=${f//./_}
			f="get_cache_""$f"
			$f > "$tmp_cache/$pid/$prefix"
			mv "$tmp_cache/$pid/$prefix" "$cache_path/$prefix"
			;;
		esac
	done
	
	rm -rf "$tmp_cache/$pid"
	ls_cache=`ls $tmp_cache`
	for pid in $ls_cache; do
		if [ ! -d /proc/$pid ]; then
			rm -rf "$tmp_cache/$pid"
		fi
	done
	ls_cache=`ls $tmp_cache`
	if [ "_$ls_cache" = "_" ]; then
		rm -rf "$tmp_cache"
	fi
}


handle_action() {
	local fault_code=$FAULT_CPE_NO_FAULT
	if [ "$action" = "get_cache" ]; then
		if [ "$__arg1" != "" ]; then
			local found=0
			for prefix in $prefix_list; do
				if [ "$prefix" = "$__arg1" ]; then
					found=1
					break
				fi
			done
			if [ "$found" != "1" ]; then 
				echo "Invalid object argument"
				return
			fi
		fi
		handle_get_cache "$__arg1"
	fi
	if [ "$action" = "wait_cache" ]; then
		local found=0
		handle_get_cache "InternetGatewayDevice." "1"
		handle_get_cache "InternetGatewayDevice.ManagementServer."
		handle_get_cache "InternetGatewayDevice.DeviceInfo."
		handle_get_cache "InternetGatewayDevice.X_INTENO_SE_LoginCfg."
		local ls_cache=""
		while [ "$found" = "0" ]; do
			ls_prefix=`ls $cache_path`
			for prefix in $prefix_list; do
				[ "${prefix_list_skip_wait_cache/$prefix/}" != "$prefix_list_skip_wait_cache" ] && continue
				found=0
				for ls_p in $ls_prefix; do
					if [ "$prefix" = "$ls_p" ]; then
						found=1
						break
					fi
				done
				if [ "$found" = "0" ]; then 
					sleep 1
					break
				fi
			done
			if [ "$found" = "1" ]; then
				local cache_running=1
				while [ "$cache_running" = "1" ]; do
					ls_cache=`ls $tmp_cache`
					cache_running=0
					for pid in $ls_cache; do
						if [ -d /proc/$pid ]; then
							cache_running=1
							sleep 1
							break
						fi
					done
				done
			fi
		done
	fi
	
	if [ "$action" = "clean_cache" ]; then
		rm -rf "$cache_path/"*
	fi
	
	if [ "$action" = "get_value" ]; then
		get_param_value_generic "$__arg1"
		fault_code="$?"
		if [ "$fault_code" != "0" ]; then
			let fault_code=$fault_code+9000
			freecwmp_output "$__arg1" "" "" "" "" "$fault_code"
		fi
	fi

	if [ "$action" = "get_name" ]; then
		__arg2=`echo $__arg2|tr '[A-Z]' '[a-z]'`
		if [ "$__arg2" = "true" ]; then
			__arg2=1
		elif [ "$__arg2" = "false" ]; then
			__arg2=0
		fi
		if [ "$__arg2" != "0" -a "$__arg2" != "1"  ]; then
			fault_code="$FAULT_CPE_INVALID_ARGUMENTS"
		else
			get_param_name_generic "$__arg1" "$__arg2"
			fault_code="$?"
		fi
		if [ "$fault_code" != "0" ]; then
			let fault_code=$fault_code+9000
			freecwmp_output "$__arg1" "" "" "" "" "$fault_code"
		fi
	fi

	if [ "$action" = "get_notification" ]; then
		get_param_notification_generic "$__arg1"
		fault_code="$?"
		if [ "$fault_code" != "0" ]; then
			let fault_code=$fault_code+9000
			freecwmp_output "$__arg1" "" "" "" "" "$fault_code"
		fi
	fi

	if [ "$action" = "set_value" ]; then	
		set_param_value_generic "$__arg1" "$__arg2"
		fault_code="$?"
		if [ "$fault_code" != "0" ]; then
			let fault_code=$fault_code+9000
			freecwmp_set_parameter_fault "$__arg1" "$fault_code"
		fi
	fi
	
	if [ "$action" = "set_notification" ]; then
		__arg3=`echo $__arg3|tr '[A-Z]' '[a-z]'`
		if [ "$__arg3" = "true" ]; then
			__arg3=1
		fi
		if [ "$__arg3" = "1" ]; then
			set_param_notification_generic "$__arg1" "$__arg2"
			fault_code="$?"
			if [ "$fault_code" != "0" ]; then
				let fault_code=$fault_code+9000
				freecwmp_set_parameter_fault "$__arg1" "$fault_code"
			fi
		fi
	fi


	if [ "$action" = "add_object" ]; then
		object_fn_generic "$__arg1"
		fault_code="$?"
		if [ "$fault_code" != "0" ]; then
			let fault_code=$fault_code+9000
			freecwmp_output "" "" "" "" "" "$fault_code"
		else
			$UCI_SET cwmp.acs.ParameterKey=$__arg2
			$UCI_COMMIT
		fi
	fi

	if [ "$action" = "delete_object" ]; then
		object_fn_generic "$__arg1"
		fault_code="$?"
		if [ "$fault_code" != "0" ]; then
			let fault_code=$fault_code+9000
			freecwmp_output "" "" "" "" "" "$fault_code"
		else
			$UCI_SET cwmp.acs.ParameterKey=$__arg2
			$UCI_COMMIT
		fi
	fi

	if [ "$action" = "download" ]; then
		local fault_code="9000"
		if [ "$__arg4" = "" -o "$__arg5" = "" ];then
			wget -O /tmp/freecwmp_download "$__arg1" 2> /dev/null
			if [ "$?" != "0" ];then
				let fault_code=$fault_code+$FAULT_CPE_DOWNLOAD_FAILURE
				freecwmp_fault_output "" "$fault_code"
				return 1
			fi
		else
			local url="http://$__arg4:$__arg5@`echo $__arg1|sed 's/http:\/\///g'`"
			wget -O /tmp/freecwmp_download "$url" 2> /dev/null
			if [ "$?" != "0" ];then
				let fault_code=$fault_code+$FAULT_CPE_DOWNLOAD_FAILURE
				freecwmp_fault_output "" "$fault_code"
				return 1
			fi
		fi

		local flashsize="`freecwmp_check_flash_size`"
		local filesize=`ls -l /tmp/freecwmp_download | awk '{ print $5 }'`
		if [ $flashsize -gt 0 -a $flashsize -lt $__arg2 ]; then
			let fault_code=$fault_code+$FAULT_CPE_DOWNLOAD_FAILURE
			rm /tmp/freecwmp_download 2> /dev/null
			freecwmp_fault_output "" "$fault_code"
		else
			if [ "$__arg3" = "1" ];then
				mv /tmp/freecwmp_download /tmp/firmware_upgrade_image 2> /dev/null
				freecwmp_check_image
				if [ "$?" = "0" ];then
					if [ $flashsize -gt 0 -a $filesize -gt $flashsize ];then
						let fault_code=$fault_code+$FAULT_CPE_DOWNLOAD_FAIL_FILE_CORRUPTED
						rm /tmp/firmware_upgrade_image 2> /dev/null
						freecwmp_fault_output "" "$fault_code"
					else
						rm /tmp/firmware_upgrade_image_last_valid 2> /dev/null
						mv /tmp/firmware_upgrade_image /tmp/firmware_upgrade_image_last_valid 2> /dev/null
						freecwmp_fault_output "" "$FAULT_CPE_NO_FAULT"
					fi
				else
					let fault_code=$fault_code+$FAULT_CPE_DOWNLOAD_FAIL_FILE_CORRUPTED
					rm /tmp/firmware_upgrade_image 2> /dev/null
					freecwmp_fault_output "" "$fault_code"
				fi
			elif [ "$__arg3" = "2" ];then
				mv /tmp/freecwmp_download /tmp/web_content.ipk 2> /dev/null
				freecwmp_fault_output "" "$FAULT_CPE_NO_FAULT"
			elif [ "$__arg3" = "3" ];then
				mv /tmp/freecwmp_download /tmp/vendor_configuration_file.cfg 2> /dev/null
				freecwmp_fault_output "" "$FAULT_CPE_NO_FAULT"
			else
				let fault_code=$fault_code+$FAULT_CPE_DOWNLOAD_FAILURE
				freecwmp_fault_output "" "$fault_code"
				rm /tmp/freecwmp_download 2> /dev/null
			fi
		fi
	fi

	if [ "$action" = "apply_download" ]; then
		case "$__arg1" in
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
			ACTION=add INTERFACE=resetbutton /sbin/hotplug-call button
			reboot
		fi
	fi

	if [ "$action" = "reboot" ]; then
		if [ ${FLAGS_dummy} -eq ${FLAGS_TRUE} ]; then
			echo "# reboot"
		else
			sync
			reboot
		fi
	fi

	if [ "$action" = "apply_notification" -o "$action" = "apply_value" ]; then
		if [ ! -f $set_fault_tmp_file ]; then
			# applying
			local prefix=""
			local filename=""
			local max_len=0
			local len=0

			case $action in
				apply_notification)
				cat $set_tmp_file | while read line; do
					json_init
					json_load "$line"
					json_get_var parameter parameter
					json_get_var notification notification
					max_len=0
					for prefix in $prefix_list; do
						case  "$parameter" in "$prefix"*)
							len=${#prefix}
							if [ $len -gt $max_len ]; then
								max_len=$len
								filename="$prefix"
							fi
						esac
					done
					local l=${#parameter}
					let l--
					if [ "${parameter:$l:1}" != "." ]; then
						sed -i "/\<$parameter\>/s%.*%$line%" $cache_path/$filename
					else
						cat $cache_path/$filename|grep "$parameter"|grep "\"notification\""| while read line; do
							json_init
							json_load "$line"
							json_get_var parameter_name parameter
							json_add_string "notification" "$notification"
							json_close_object
							param=`json_dump`
							sed -i "/\<$parameter_name\>/s%.*%$param%" $cache_path/$filename
						done
					fi
				done
				freecwmp_output "" "" "" "" "" "" "" "" "0"
				;;
				apply_value)
				local val
				local param
				cat $set_tmp_file | while read line; do
					json_init
					json_load "$line"
					json_get_var param parameter
					json_get_var val value
					json_get_var secret_value secret_value
					json_get_var notification notification
					json_get_var type type
					json_get_var set_cmd set_cmd
					eval "$set_cmd"
					max_len=0
					for prefix in $prefix_list; do
						case  "$param" in "$prefix"*)
							len=${#prefix}
							if [ $len -gt $max_len ]; then
								max_len=$len
								filename="$prefix"
							fi
						esac
					done
					if [ "$secret_value" != "1" ]; then
						sed -i "/\<$param\>/s%.*%$line%" $cache_path/$filename
					fi
				done
				$UCI_SET cwmp.acs.ParameterKey=$__arg1
				freecwmp_output "" "" "" "" "" "" "1"
				;;
			esac
			$UCI_COMMIT
		else
			if [ "$action" = "apply_notification" ]; then
				cat $set_fault_tmp_file | head -1 
			else
				cat $set_fault_tmp_file
			fi
			rm -f $set_fault_tmp_file
			/sbin/uci ${UCI_CONFIG_DIR:+-c $UCI_CONFIG_DIR} -q revert cwmp
		fi
		rm -f $set_tmp_file
	fi

	if [ "$action" = "inform" ]; then
		rm -f "$cache_linker_dynamic"
		forced_param=`get_param_name_generic "" 0 | grep "\"forced_inform\""`
		echo "$forced_param" | grep -v "\"get_cmd\""
		echo "$forced_param" | grep "\"get_cmd\"" | while read line; do
			json_init
			json_load "$line"
			json_get_var exec_get_cmd get_cmd
			json_get_var param parameter
			json_get_var type type
			val=`eval "$exec_get_cmd"`
			freecwmp_output "$param" "$val" "" "" "$type"
		done
		rm -f "$cache_linker_dynamic"
	fi

	if [ "$action" = "notify" ]; then
		freecwmp_notify "$__arg1" "$__arg2"
	fi
	
	if [ "$action" = "end_session" ]; then	
		echo 'rm -f /tmp/end_session.sh' >> /tmp/end_session.sh
		/bin/sh /tmp/end_session.sh
	fi
	
	if [ "$action" = "allow_cr_ip" ]; then
		local port=`$UCI_GET cwmp.cpe.port`
		local if_wan=`$UCI_GET cwmp.cpe.default_wan_interface`
		local zone=`$UCI_SHOW firewall | grep "firewall\.@zone\[[0-9]\+\]\.network=.*$if_wan" | head -1 | cut -f2 -d.`
		local zone_name=`$UCI_GET firewall.$zone.name`
		[ "$zone_name" = "" ] && return
		# update iptables rule
		sed -i "s,^.*Open ACS port.*,iptables -I zone_${zone_name}_input -p tcp -s $__arg1 --dport $port -j ACCEPT -m comment --comment=\"Open ACS port\",g" /etc/firewall.cwmp
		fw3 reload
	fi
	
	if [ "$action" = "json_continuous_input" ]; then
		echo "EOF"
		while read CMD; do
			[ -z "$CMD" ] && continue
			result=""
			json_init
			json_load "$CMD"
			json_get_var command command
			json_get_var action action
			case "$command" in
				set)
					if [ "$action" = "notification" ]; then
						json_get_var __arg1 parameter
						json_get_var __arg2 value
						json_get_var __arg3 change
						action="set_notification"
					elif [ "$action" = "value" ]; then
						json_get_var __arg1 parameter
						json_get_var __arg2 value
						action="set_value"
					else
						json_get_var __arg1 parameter
						json_get_var __arg2 value
						action="set_value"
					fi
					;;
				get)
					if [ "$action" = "cache" ]; then
						json_get_var __arg1 parameter
						action="get_cache"
					elif [ "$action" = "notification" ]; then
						json_get_var __arg1 parameter
						action="get_notification"
					elif [ "$action" = "value" ]; then
						json_get_var __arg1 parameter
						action="get_value"
					elif [ "$action" = "name" ]; then
						json_get_var __arg1 parameter
						json_get_var __arg2 next_level
						action="get_name"
					else
						json_get_var __arg1 parameter
						action="get_value"
					fi
					;;
				download)
					json_get_var __arg1 url
					json_get_var __arg2 size
					json_get_var __arg3 type
					json_get_var __arg4 user
					json_get_var __arg5 pass
					action="download"
					;;
				factory_reset)
					action="factory_reset"
					;;
				reboot)
					action="reboot"
					;;
				apply)
					if [ "$action" = "notification" ]; then
						action="apply_notification"
					elif [ "$action" = "value" ]; then
						json_get_var __arg1 arg
						action="apply_value"
					elif [ "$action" = "download" ]; then
						json_get_var __arg1 arg
						action="apply_download"
					else
						json_get_var __arg1 arg
						action="apply_value"
					fi
					;;
				add)
					json_get_var __arg1 parameter
					json_get_var __arg2 parameter_key
					action="add_object"
					;;
				delete)
					json_get_var __arg1 parameter
					json_get_var __arg2 parameter_key
					action="delete_object"
					;;
				inform)
					action="inform"
					;;
				end_session)
					action="end_session"
					;;
				allow_cr_ip)
					action="allow_cr_ip"
					json_get_var __arg1 arg
					;;
				end)
					echo "EOF"
					;;
				exit)
					exit 0
					;;
				*)
					continue
					;;
			esac
			handle_action
		done
	
		exit 0;
	fi
}

handle_action 2> /dev/null

if [ ${FLAGS_debug} -eq ${FLAGS_TRUE} ]; then
	echo "[debug] exited at \"`date`\""
fi
