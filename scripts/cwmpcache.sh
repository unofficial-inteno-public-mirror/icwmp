#!/bin/sh

local freecwmpscript="/usr/sbin/freecwmp"
local initscript=$(echo $1 | cut -d'/' -f4)
local action=$2

[ ! -f $freecwmpscript ] && exit
[ -z $action ] && exit

sleep 2

cwmp_cache() {
	local object=$1
	ps -w | grep freecwmp | grep $object >/dev/null || freecwmp get cache InternetGatewayDevice.$object. &
}

cwmp_voicecache() {
	local max_line=""
	while [ "$max_line" = "" -o "$max_line" = "0" ]; do
		max_line=`/usr/sbin/asterisk -rx "brcm show status" | grep -c "Line id"`
		if [ "$max_line" = "" -o "$max_line" = "0" ]; then
			sleep 1
		fi
	done
	cwmp_cache "Services"
}

cwmp_firewallcache() {
	cwmp_cache "X_INTENO_SE_IpAccCfg"
}

cwmp_icecache() {
	cwmp_cache "X_INTENO_SE_ICE"
}

cwmp_upnpcache() {
	cwmp_cache "UPnP"
}

cwmp_netcache() {
	local tm=1
	while [ "$(nvram get wlmngr)" != "done" ]; do
		sleep $tm
		[ $tm -ge 10 ] && break
		tm=$((tm+1))
	done
	cwmp_cache "LANDevice"
	cwmp_cache "Layer2Bridging"
	cwmp_cache "Layer3Forwarding"
	cwmp_cache "UPnP"
}

cwmp_passwordcache() {
	cwmp_cache "X_INTENO_SE_LoginCfg"
}

cwmp_ntpcache() {
	cwmp_cache "Time"
}

case $initscript in
	asterisk)
		case $action in
			boot|restart|reload) cwmp_voicecache ;;
		esac
	;;
	firewall)
		case $action in
			boot|start|restart|reload) cwmp_firewallcache ;;
		esac
	;;
	ice*)
		case $action in
			start|restart|reload) cwmp_icecache ;;
		esac
	;;
	ice*)
		case $action in
			start|restart|reload) cwmp_icecache ;;
		esac
	;;
	miniupnpd)
		case $action in
			start|restart|reload) cwmp_upnpcache ;;
		esac
	;;
	network)
		case $action in
			start|restart|reload) cwmp_netcache ;;
		esac
	;;
	passwords)
		case $action in
			start|restart|reload) cwmp_passwordcache ;;
		esac
	;;
	sysntpd)
		case $action in
			start|restart|reload) cwmp_ntpcache ;;
		esac
	;;
esac

