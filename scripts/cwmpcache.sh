#!/bin/sh

local freecwmpscript="/usr/sbin/freecwmp"
local initscript=$(echo $1 | cut -d'/' -f4)
local action=$2

[ ! -f $freecwmpscript ] && exit
[ -z $action ] && exit

restart_service_maps=""
. /usr/share/freecwmp/functions/common "RESTART_SERVICE_MAP"
for ffile in `ls /usr/share/freecwmp/functions/`; do
[ "$ffile" = "common"  ] && continue
. /usr/share/freecwmp/functions/$ffile "RESTART_SERVICE_MAP"
done

sleep 2

cwmp_cache() {
	local object="$1"
	local service="$2"
	local action="$3"	
	ps -w | grep freecwmp | grep $object >/dev/null || freecwmp get cache "$object" &
}

cwmp_voicecache_wait_up() {
	local max_line=""
	while [ "$max_line" = "" -o "$max_line" = "0" ]; do
		max_line=`/usr/sbin/asterisk -rx "brcm show status" | grep -c "Line id"`
		if [ "$max_line" = "" -o "$max_line" = "0" ]; then
			sleep 1
		fi
	done
}

cwmp_netcache_wait_up() {
	local tm=1
	while [ "$(nvram get wlmngr)" != "done" ]; do
		sleep $tm
		[ $tm -ge 10 ] && break
		tm=$((tm+1))
	done
}

cwmp_servicecache_build() {
	local service="$1"
	local action="$2"
	case $action in
		boot|start|restart|reload)
			for rsm in $restart_service_maps; do
				local prefix=${rsm%%:*}
				local rss=${rsm#*:}
				rss=",$rss,"
				[ "${rss/,$service,/}" = "$rss" ] && continue
				case $service in
					*asterisk) cwmp_voicecache_wait_up ;;
					*network) cwmp_netcache_wait_up ;;
				esac
				cwmp_cache "$prefix" "$service" "$action"
			done
		;;
	esac
}

initscript=`echo $initscript | sed -e "s/^[A-Z][0-9]*//"`
if [ "${restart_service_maps/$initscript/}" != "$restart_service_maps" ] ;then 
	cwmp_servicecache_build "$initscript" "$action"
fi