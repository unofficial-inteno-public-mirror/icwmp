#!/bin/sh
# Copyright (C) 2014 Inteno Broadband Technology AB
#  Author Mohamed Kallel <mohamed.kallel@pivasoftware.com>

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
for cnt in 1 2 3 4 5 6; do
	cwmpdpid=`ps | grep /usr/sbin/cwmpd | grep -v grep | grep -v "[ \t\b\v]\+[ZzTt]\+[ \t\b\v]\+"`
	if [ "$cwmpdpid" != "" ]; then
		exit 0
	fi
	sleep 5
done

restart_cwmpd 2>/dev/null
exit 0
