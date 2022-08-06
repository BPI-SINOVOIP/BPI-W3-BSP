#!/bin/bash

case "$1" in
	start)
		if [ -e "/data/rockchip_test/auto_reboot.sh" ]; then
			echo "start recovery auto-reboot"
			mkdir -p /data/rockchip_test
			cp /rockchip_test/auto_reboot/auto_reboot.sh /data/rockchip_test/
		fi

		if [ -e "/data/rockchip_test/power_lost_test.sh" ]; then
			echo "start test flash power lost"
			source /data/rockchip_test/power_lost_test.sh &
		fi
		if [ -e "/data/rockchip_test/auto_reboot.sh" ]; then
			echo "start auto-reboot"
			source /data/rockchip_test/auto_reboot.sh `cat /data/rockchip_test/reboot_total_cnt`&
		fi

		;;
	stop)
		echo "stop auto-reboot finished"
		;;
	restart|reload)
		$0 stop
		$0 start
		;;
	*)
		echo "Usage: $0 {start|stop|restart}"
		exit 1
esac

exit 0
