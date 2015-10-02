#!/bin/sh
# Copyright (C) 2014 Inteno Broadband Technology AB
#  Author Mohamed Kallel <mohamed.kallel@pivasoftware.com>

watch_icwmpd_log() {
	local icwmpd_log_file=`uci get -q icwmp.cpe.log_file_name`
	icwmpd_log_file=${icwmpd_log_file:-"/var/log/icwmpd.log"}
	echo "*******************************************************************************"
	echo "`date`: Restart icwmpd by watchcwmpd"
	echo "Last 20 lines in the icwmpd log before the restart:"
	echo "-------------------------------------------------------------------------------"
	cat $icwmpd_log_file | tail -20
	echo "-------------------------------------------------------------------------------"
}
restart_icwmpd() {
	#Save the last 20 lines of icwmpd log
	watch_icwmpd_log >> /var/log/watchicwmpd.log
	#Restart icwmpd
	/etc/init.d/icwmpd restart
}
for cnt in 1 2 3 4 5 6; do
	icwmpdpid=`ps | grep /usr/sbin/icwmpd | grep -v grep | grep -v "[ \t\b\v]\+[ZzTt]\+[ \t\b\v]\+"`
	if [ "$icwmpdpid" != "" ]; then
		exit 0
	fi
	sleep 5
done

restart_icwmpd 2>/dev/null
exit 0
