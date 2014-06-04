#!/bin/sh
# Copyright (C) 2014 Inteno Broadband Technology AB
#  Author Mohamed Kallel <mohamed.kallel@pivasoftware.com>

. /usr/share/libubox/jshn.sh

watch_cwmpd_log() {
	local cwmpd_log_file=`uci get -q cwmp.cpe.log_file_name`
	cwmpd_log_file=${cwmpd_log_file:-"/var/log/cwmpd.log"}
	echo "*******************************************************************************"
	echo "`date`: Restart cwmpd by watchcwmpd"
	echo "Last 20 lines in the cwmpd log before the restart:"
	echo "-------------------------------------------------------------------------------"
	cat $cwmpd_log_file | tail -20
	echo "-------------------------------------------------------------------------------"
}
restart_cwmpd() {
	#Save the last 20 lines of cwmpd log
	watch_cwmpd_log >> /var/log/watchcwmpd.log
	#Restart cwmpd
	/etc/init.d/cwmpd restart
}
for cnt in 1 2; do
	jmsg=`ubus call tr069 status 2>/dev/null`
	if [ "$jmsg" = "" ]; then
		break
	else
		local stime
		json_init
		json_load "$jmsg"
		json_select "next_session"
		json_get_var stime start_time
		stime=${stime%%+*}
		stime=${stime//T/ }
		stime=`date -d"$stime" +%s`
		[ "$stime" = "" ] && continue
		local ctime=`date +%s`
		if [ $ctime -gt $stime ]; then
			sleep 60
			continue
		else
			exit 0
		fi
	fi
done

restart_cwmpd 2>/dev/null
exit 0
