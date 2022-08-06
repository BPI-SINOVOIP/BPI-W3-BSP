/*
 * Copyright (c) 2018 Rockchip, Inc. All Rights Reserved.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
#include <net/if.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <sys/signalfd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <time.h>
#include <pthread.h>

#include "bluez_ctrl.h"
#include <RkBle.h>
#include "utility.h"

#define BT_IS_BLE_SINK_COEXIST 1

volatile bt_control_t bt_control = {
	0,
	0,
	0,
	0,
	0,
	0,
	0,
};

static int bt_close_a2dp_server();

int bt_gethostname(char *hostname_buf, const size_t size)
{
	char hostname[HOSTNAME_MAX_LEN + 1];
	size_t buf_len = sizeof(hostname) - 1;

	memset(hostname_buf, 0, size);
	memset(hostname, 0, sizeof(hostname));

	if (gethostname(hostname, buf_len) != 0) {
		pr_err("bt_gethostname gethostname error !!!!!!!!\n");
		return -1;
	}

	/* Deny sending of these local hostnames */
	if (hostname[0] == '\0' || hostname[0] == '.' || strcmp(hostname, "(none)") == 0) {
		pr_err("bt_gethostname gethostname format error !!!\n");
		return -2;
	}

	strncpy(hostname_buf, hostname, strlen(hostname) > (size - 1) ? (size - 1) : strlen(hostname));
	return 0;
}

static int _bt_close_server(void)
{
	pr_info("=== _bt_close_server ===\n");

	if (kill_task("obexctl") < 0)
		return -1;

	if (kill_task("obexd") < 0)
		return -1;

	if (kill_task("bluealsa") < 0)
		return -1;

	if (kill_task("bluealsa-aplay") < 0)
		return -1;

	if (kill_task("bluetoothctl") < 0)
		return -1;

	if (kill_task("bluetoothd") < 0)
		return -1;

	if (get_ps_pid("rtk_hciattach")) {
		exec_command_system("hciconfig hci0 down");
		if (kill_task("rtk_hciattach") < 0)
			return -1;
		sleep(1);
	}

	if (get_ps_pid("brcm_patchram_plus1")) {
		exec_command_system("hciconfig hci0 down");
		if (kill_task("brcm_patchram_plus1") < 0)
			return -1;
		sleep(1);
	}


	return 0;
}

static int _bt_open_server()
{
	char ret_buff[1024];
	char bt_buff[1024];

	pr_err("[BT_OPEN] _bt_open_server\n");

	exec_command_system("echo 0 > /sys/class/rfkill/rfkill0/state && usleep 10000");
	exec_command_system("echo 1 > /sys/class/rfkill/rfkill0/state && usleep 10000");

	/* check bt vendor (exteran/rkwifibt) */
	if (access("/usr/bin/bt_init.sh", F_OK)) {
		pr_err("[BT_OPEN]  bt_init.sh not exist !!!\n");
		if (access("/userdata/bt_pcba_test", F_OK)) {
			pr_err("[BT_OPEN] userdata bt_pcba_test not exist !!!\n");
			return -1;
		}
	}

	/* realtek init */
	exec_command("cat /usr/bin/bt_init.sh | grep rtk_hciattach", bt_buff, 1024);
	if (bt_buff[0]) {
		exec_command_system("insmod /usr/lib/modules/hci_uart.ko && usleep 50000");
		exec_command("lsmod", ret_buff, 1024);
		if (!strstr(ret_buff, "hci_uart")) {
			pr_err("open bt server: insmod hci_uart.ko failed!\n");
			return -1;
		}

		pr_err("bt_buff: %s \n", bt_buff);
		exec_command_system(bt_buff);

		sleep(1);
		if (!get_ps_pid("rtk_hciattach")) {
			pr_err("open bt server error: rtk_hciattach failed!\n");
			return -1;
		}
	}

	/* broadcom or cypress init */
	exec_command("cat /usr/bin/bt_init.sh | grep brcm_patchram_plus1 | grep -v killall", bt_buff, 1024);
	if (bt_buff[0]) {
		pr_err("bt_buff: %s \n", bt_buff);
		exec_command_system(bt_buff);
		sleep(1);
		exec_command("pidof brcm_patchram_plus1", ret_buff, 1024);
		if (!ret_buff[0]) {
			pr_err("open bt server failed! error: brcm_patchram_plus1 failed!\n");
			return -1;
		}
		sleep(1);
	}

#if 0
	exec_command_system("hcidump -i hci0 -w /data/btsnoop.log &");
	sleep(1);
	if (access("/data/btsnoop.log", F_OK)) {
		printf("save btsnoop error, retry\n");
		exec_command_system("hcidump -i hci0 -w /data/btsnoop.log &");
		sleep(1);
		if (access("/data/btsnoop.log", F_OK)) {
			printf("second save btsnoop error\n");
		} else {
			printf("save btsnoop successfully\n\n\n");
		}
	} else {
		printf("save btsnoop successfully\n\n\n");
	}

	msleep(50);
#endif

	/* run bluetoothd */
	if (run_task("bluetoothd", "/usr/libexec/bluetooth/bluetoothd -C -n -d -E &")) {
		pr_err("open bt server failed! error: bluetoothd failed!\n");
		return -1;
	}

	//set Bluetooth NoInputNoOutput mode
	//exec_command_system("bluetoothctl -a NoInputNoOutput &");
	msleep(10);

	//set vendor params for scan/conn x realtek
	exec_command("cat /usr/bin/bt_init.sh | grep rtk_hciattach", bt_buff, 1024);
	if (bt_buff[0]) {
		exec_command_system("hciconfig hci0 pageparms 18:1024");
		msleep(10);
		exec_command_system("hciconfig hci0 inqparms 18:2048");
		msleep(10);
		exec_command_system("hcitool cmd 0x03 0x47 0x01");
		msleep(10);
		exec_command_system("hcitool cmd 0x03 0x43 0x01");
		msleep(10);
	}

	pr_err("_bt_open_server end\n");
	return 0;
}

static int bt_start_a2dp_source()
{
	char ret_buff[1024];
	int count = 10;

	kill_task("bluealsa");
	kill_task("bluealsa-aplay");

	exec_command_system("bluealsa --profile=a2dp-source &");
	exec_command("pidof bluealsa", ret_buff, 1024);
	if (!ret_buff[0]) {
		pr_err("start a2dp source profile failed!\n");
		return -1;
	}

	while(count--) {
		msleep(10);
		if (access("/var/run/bluealsa/hci0", F_OK))
			pr_info("%s: wait for /var/run/bluealsa/hci0 to set up\n", __func__);
		else
			break;

		msleep(50);
	}

	exec_command_system("hciconfig hci0 class 0x480400");
	msleep(10);
	exec_command_system("hciconfig hci0 class 0x480400");
	msleep(10);

	return 0;
}

static int bt_start_a2dp_sink(int sink_only)
{
	char ret_buff[1024];
	int count = 10;

	kill_task("bluealsa");
	kill_task("bluealsa-aplay");
	msleep(500);

	if (sink_only)
		exec_command_system("bluealsa --profile=a2dp-sink --a2dp-volume &");
	else
		exec_command_system("bluealsa --a2dp-volume &");
	exec_command("pidof bluealsa", ret_buff, 1024);
	if (!ret_buff[0]) {
		pr_err("start a2dp sink profile failed!\n");
		return -1;
	}

	while(count--) {
		msleep(10);
		if (access("/var/run/bluealsa/hci0", F_OK))
			pr_info("%s: wait for /var/run/bluealsa/hci0 to set up\n", __func__);
		else
			break;

		msleep(50);
	}

	exec_command_system("bluealsa-aplay --profile-a2dp 00:00:00:00:00:00 &");
	exec_command("pidof bluealsa-aplay", ret_buff, 1024);
	if (!ret_buff[0]) {
		pr_err("start a2dp sink play server failed!\n");
		return -1;
	}

	exec_command_system("hciconfig hci0 class 0x240404");
	msleep(100);
	exec_command_system("hciconfig hci0 class 0x240404");
	msleep(100);
	pr_err("bt_start_a2dp_sink exit\n");

	return 0;
}

static int bt_start_hfp()
{
	char ret_buff[1024];
	int count = 10;

	kill_task("bluealsa");
	kill_task("bluealsa-aplay");

	exec_command_system("bluealsa --profile=hfp-hf &");
	exec_command("pidof bluealsa", ret_buff, 1024);
	if (!ret_buff[0]) {
		pr_err("start hfp-hf profile failed!\n");
		return -1;
	}

	while(count--) {
		msleep(10);
		if (access("/var/run/bluealsa/hci0", F_OK))
			pr_info("%s: wait for /var/run/bluealsa/hci0 to set up\n", __func__);
		else
			break;

		msleep(50);
	}

	exec_command_system("hciconfig hci0 class 0x240404");
	msleep(10);
	exec_command_system("hciconfig hci0 class 0x240404");
	msleep(10);
	pr_err("%s exit\n", __func__);

	return 0;
}

bool bt_is_open()
{
	if (bt_control.is_bt_open) {
		if (get_ps_pid("bluetoothd")) {
			return true;
		} else {
			pr_err("bt has been opened but bluetoothd server exit.\n");
		}
	}

	return false;
}

bool bt_sink_is_open(void)
{
	if (bt_control.is_a2dp_sink_open) {
		if (get_ps_pid("bluetoothd")) {
			if(!get_ps_pid("bluealsa")) {
				pr_err("bt sink has been opened, but bluealsa exit\n");
				//return false;
			}

			if(!get_ps_pid("bluealsa-aplay")) {
				pr_err("bt sink has been opened, but bluealsa-aplay exit\n");
				//return false;
			}

			return true;
		} else {
			pr_err("bt sink has been opened, but bluetoothd server exit\n");
		}
	}

	return false;
}

bool bt_hfp_is_open(void)
{
	if (bt_control.is_hfp_open) {
		if (get_ps_pid("bluetoothd")) {
			if(!get_ps_pid("bluealsa")) {
				pr_err("bt hfp has been opened, but bluealsa exit\n");
				//return false;
			}

			return true;
		} else {
			pr_err("bt hfp has been opened, but bluetoothd server exit\n");
		}
	}

	return false;
}

bool bt_source_is_open(void)
{
	if (bt_control.is_a2dp_source_open) {
		if (get_ps_pid("bluetoothd")) {
			if(!get_ps_pid("bluealsa")) {
				pr_err("bt source has been opened, but bluealsa exit\n");
				//return false;
			}

			return true;
		} else {
			pr_err("bt source has been opened, but bluetoothd server exit\n");
		}
	}

	return false;
}

bool ble_is_open()
{
	if (bt_control.is_ble_open) {
		if (get_ps_pid("bluetoothd")) {
			return true;
		} else {
			pr_err("ble has been opened, but bluetoothd server exit\n");
		}
	}

	return false;
}

bool ble_client_is_open()
{
	if (bt_control.is_ble_client_open) {
		if (get_ps_pid("bluetoothd")) {
			return true;
		} else {
			pr_err("ble client has been opened, but bluetoothd server exit\n");
		}
	}

	return false;
}

int bt_control_cmd_send(enum BtControl bt_ctrl_cmd)
{
	char cmd[10];
	memset(cmd, 0, 10);
	sprintf(cmd, "%d", bt_ctrl_cmd);

	if (!bt_sink_is_open()) {
		pr_err("Not bluetooth play mode, don`t send bluetooth control commands\n");
		return -1;
	}

	pr_err("bt_control_cmd_send, cmd: %s, len: %d\n", cmd, strlen(cmd));
	switch (bt_ctrl_cmd) {
	case (BT_PLAY):
	case (BT_RESUME_PLAY):
		play_avrcp();
		break;
	case (BT_PAUSE_PLAY):
		pause_avrcp();
		break;
	case (BT_AVRCP_STOP):
		stop_avrcp();
		break;
	case (BT_AVRCP_BWD):
		previous_avrcp();
		break;
	case (BT_AVRCP_FWD):
		next_avrcp();
		break;
	default:
		break;
	}

	return 0;
}

void bt_close_ble(bool disconnect)
{
	pr_err("ble server close\n");

	gatt_set_stopping(true);
	if(disconnect) {
		if(!ble_disconnect())
			sleep(3);
	} else {
		if(!remove_ble_device())
			sleep(2);
	}

	ble_disable_adv();
	ble_deregister_state_callback();
	ble_deregister_mtu_callback();
	gatt_set_stopping(false);
	bt_control.is_ble_open = false;
}

int bt_close_sink(bool disconnect)
{
	int ret = 0;

	if (!bt_sink_is_open())
		return -1;

	pr_err("bt_close_sink\n");

	if (bt_hfp_is_open()) {
		release_avrcp_ctrl2();
		kill_task("bluealsa-aplay");
	} else {
		kill_task("bluealsa-aplay");

		if(disconnect) {
			if(!disconnect_current_devices())
				sleep(3);
		}

		release_avrcp_ctrl();
		kill_task("bluealsa");
	}

	bt_control.is_a2dp_sink_open = false;
	return ret;
}

int bt_close_source(bool disconnect)
{
	if (!bt_source_is_open())
		return -1;

	pr_info("%s\n", __func__);

	if(disconnect) {
		if (!disconnect_current_devices())
			sleep(3);
	}

	//source_stop_connecting();

	kill_task("bluealsa");
	return 0;
}

static int bt_a2dp_sink_open(void)
{
	pr_err("bt_a2dp_sink_server_open\n");
	if(bt_start_a2dp_sink(1) < 0)
		return -1;

	a2dp_sink_open();
	return 0;
}

static int bt_hfp_hf_open(void)
{
	pr_err("%s is called!\n", __func__);
	if (bt_start_hfp() < 0)
		return -1;

	system("hciconfig hci0 piscan");
	return 0;
}

static int bt_hfp_with_sink_open(void)
{
	pr_err("%s is called!\n", __func__);
	if (bt_start_a2dp_sink(0) < 0)
		return -1;

	a2dp_sink_open();
	return 0;
}

/* Load the Bluetooth firmware and turn on the Bluetooth SRC service. */
static int bt_a2dp_src_server_open(void)
{
	pr_err("%s\n", __func__);

	if(bt_start_a2dp_source() < 0)
		return -1;

	return 0;
}

int bt_interface(enum BtControl type, void *data)
{
	if (type == BT_SINK_OPEN) {
		pr_err("Open a2dp sink.");
		if (bt_a2dp_sink_open() < 0)
			return -1;
	} else if (type == BT_SOURCE_OPEN) {
		pr_err("Open a2dp source.");
		if (bt_a2dp_src_server_open() < 0)
			return -1;
	} else if (type == BT_BLE_OPEN) {
		pr_err("Open ble.");
		ble_clean();
		if(ble_enable_adv() < 0)
			return -1;
	} else if (type == BT_HFP_OPEN) {
		pr_err("Open bt hfp.");
		if(bt_hfp_hf_open() < 0)
			return -1;
	} else if (type == BT_HFP_SINK_OPEN) {
		pr_err("Open bt hfp with sink.");
		if(bt_hfp_with_sink_open() < 0)
			return -1;
	}

	return 0;
}

static int get_bt_mac(char *bt_mac)
{
	char ret_buff[1024] = {0};
	bool ret;

	exec_command("hciconfig hci0 | grep Address | awk '{print $3}'", ret_buff, 1024);
	if (!ret_buff[0]) {
		pr_err("get bt address failed.\n");
		return false;
	}
	strncpy(bt_mac, ret_buff, 17);
	return 0;
}

int rk_bt_control(enum BtControl cmd, void *data, int len)
{
	int ret = 0;
	RkBleConfig *ble_cfg;
	bool scan;

	pr_err("controlBt, cmd: %d\n", cmd);

	switch (cmd) {
	case BT_OPEN:
		if (_bt_close_server() < 0) {
			pr_err("_bt_close_server failed\n");
			return -1;
		}

		if (_bt_open_server() < 0) {
			pr_err("_bt_open_server failed\n");
			return -1;
		}

		if (bt_open((RkBtContent *)data) < 0) {
			pr_err("bt_open failed\n");
			return -1;
		}

		bt_control.is_bt_open = true;
		break;

	case BT_SINK_OPEN:
		if (!bt_is_open())
			return -1;

		if (bt_sink_is_open())
			return 1;

		if(bt_source_is_open()) {
			pr_info("bt source has been opened, close bt source");
			rk_bt_source_close();
		}

		if (bt_interface(BT_SINK_OPEN, NULL) < 0){
			bt_control.is_a2dp_sink_open = false;
			ret = -1;
		}

		bt_control.is_a2dp_sink_open = true;
		break;

	case BT_BLE_OPEN:
		if (bt_interface(BT_BLE_OPEN, data) < 0) {
			bt_control.is_ble_open = false;
			return -1;
		}

		bt_control.is_ble_open = true;
		pr_err("=== BT_BLE_OPEN ok ===\n");
		break;

	case BT_SOURCE_OPEN:
		rk_bt_source_open();
		break;

	case BT_SOURCE_SCAN:
		ret = a2dp_master_scan(data, len, SCAN_TYPE_AUTO);
		break;

	case BT_SOURCE_CONNECT:
		ret = a2dp_master_connect((char *)data);
		break;

	case BT_SOURCE_DISCONNECT:
		ret = rk_bt_source_disconnect_by_addr((char *)data);
		break;

	case BT_SOURCE_STATUS:
		ret = a2dp_master_status(NULL, 0, NULL, 0);
		break;

	case BT_SOURCE_REMOVE:
		ret = remove_by_address((char *)data);
		break;

	case BT_SINK_CLOSE:
		rk_bt_sink_close();
		break;

	case BT_SOURCE_CLOSE:
		rk_bt_source_close();
		break;

	case BT_BLE_COLSE:
		ret = rk_ble_stop();
		break;

	case BT_SINK_IS_OPENED:
		ret = bt_sink_is_open();
		break;

	case BT_SOURCE_IS_OPENED:
		ret = bt_source_is_open();
		break;

	case BT_BLE_IS_OPENED:
		ret = ble_is_open();
		break;

	case GET_BT_MAC:
		if (get_bt_mac((char *)data) <= 0)
			ret = -1;

		break;

	case BT_VOLUME_UP:
		if (bt_control_cmd_send(BT_VOLUME_UP) < 0) {
			pr_err("Bt send volume up cmd failed\n");
			ret = -1;
		}

		break;

	case BT_VOLUME_DOWN:
		if (bt_control_cmd_send(BT_VOLUME_UP) < 0) {
			pr_err("Bt send volume down cmd failed\n");
			ret = -1;
		}

		break;

	case BT_PLAY:
	case BT_RESUME_PLAY:
		if (bt_control_cmd_send(BT_RESUME_PLAY) < 0) {
			pr_err("Bt send play cmd failed\n");
			ret = -1;
		}

		break;
	case BT_PAUSE_PLAY:
		if (bt_control_cmd_send(BT_PAUSE_PLAY) < 0) {
			pr_err("Bt send pause cmd failed\n");
			ret = -1;
		}

		break;

	case BT_AVRCP_FWD:
		if (bt_control_cmd_send(BT_AVRCP_FWD) < 0) {
			pr_err("Bt send previous track cmd failed\n");
			ret = -1;
		}

		break;

	case BT_AVRCP_BWD:
		if (bt_control_cmd_send(BT_AVRCP_BWD) < 0) {
			pr_err("Bt socket send next track cmd failed\n");
			ret = -1;
		}

		break;

	case BT_BLE_WRITE:
		ble_cfg = (RkBleConfig *)data;
		ret = gatt_write_data(ble_cfg->uuid, ble_cfg->data, ble_cfg->len);

		break;
	case BT_VISIBILITY:
		scan = (*(bool *)data);
		rkbt_inquiry_scan(scan);
		break;
	case BT_BLE_DISCONNECT:
		ret = ble_disconnect();
		break;
	default:
 		pr_err("%s, cmd <%d> is not implemented.\n", __func__, cmd);
		break;
	}

	return ret;
}
