/*
 * Copyright (c) 2017 Rockchip, Inc. All Rights Reserved.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *	 http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <iostream>
#include <linux/input.h>
#include <linux/rtc.h>
#include <sys/un.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/socket.h>

#include <DeviceIo/Rk_shell.h>
#include <DeviceIo/Rk_system.h>
#include <DeviceIo/RkBtBase.h>
#include <DeviceIo/RkBtSink.h>
#include <DeviceIo/RkBtSource.h>

#include "rkbtsource_common.h"

/* Immediate wifi Service UUID */
#define BLE_UUID_SERVICE	"0000180A-0000-1000-8000-00805F9B34FB"
#define BLE_UUID_WIFI_CHAR	"00009999-0000-1000-8000-00805F9B34FB"
#define BLE_UUID_PROXIMITY	"7B931104-1810-4CBC-94DA-875C8067F845"
#define BLE_UUID_SEND		"dfd4416e-1810-47f7-8248-eb8be3dc47f9"
#define BLE_UUID_RECV		"9884d812-1810-4a24-94d3-b2c11a851fac"

#define SCAN_DEVICES_SAVE_COUNT 30

static int sockfd = 0;
static scan_devices_t scan_devices_bak[SCAN_DEVICES_SAVE_COUNT];
static int scan_devices_count = 0;

static void rk_bt_state_cb(RK_BT_STATE state)
{
	switch(state) {
		case RK_BT_STATE_TURNING_ON:
			printf("%s: RK_BT_STATE_TURNING_ON\n", PRINT_FLAG_RKBTSOURCE);
			break;
		case RK_BT_STATE_ON:
			printf("%s: RK_BT_STATE_ON\n", PRINT_FLAG_RKBTSOURCE);
			break;
		case RK_BT_STATE_TURNING_OFF:
			printf("%s: RK_BT_STATE_TURNING_OFF\n", PRINT_FLAG_RKBTSOURCE);
			break;
		case RK_BT_STATE_OFF:
			printf("%s: RK_BT_STATE_OFF\n", PRINT_FLAG_RKBTSOURCE);
			break;
	}
}

static void rk_bt_bond_state_cb(const char *bd_addr, const char *name, RK_BT_BOND_STATE state)
{
	switch(state) {
		case RK_BT_BOND_STATE_NONE:
			printf("%s: BT BOND NONE: %s, %s\n", PRINT_FLAG_RKBTSOURCE, name, bd_addr);
			break;
		case RK_BT_BOND_STATE_BONDING:
			printf("%s: BT BOND BONDING: %s, %s\n", PRINT_FLAG_RKBTSOURCE, name, bd_addr);
			break;
		case RK_BT_BOND_STATE_BONDED:
			printf("%s: BT BONDED: %s, %s\n", PRINT_FLAG_RKBTSOURCE, name, bd_addr);
			break;
	}
}

static int rk_bt_send_scan_msg(char *scan_msg, int scan_msg_len)
{
	int ret;
	struct sockaddr_un clientAddr;

	/* Set client address */
	clientAddr.sun_family = AF_UNIX;
	strcpy(clientAddr.sun_path, "/tmp/rockchip_btsource_client");

	ret = sendto(sockfd, scan_msg, scan_msg_len, 0, (struct sockaddr *)&clientAddr, sizeof(clientAddr));
	if (ret < 0) {
		printf("%s: sendto scan msg failed! ret = %d, scan_msg = %s\n", PRINT_FLAG_ERR, ret, scan_msg);
		return -1;
	}

	return 0;
}

static void rk_bt_discovery_status_cb(RK_BT_DISCOVERY_STATE status)
{
	switch(status) {
		case RK_BT_DISC_STARTED:
			printf("%s: RK_BT_DISC_STARTED\n", PRINT_FLAG_RKBTSOURCE);
			break;
		case RK_BT_DISC_STOPPED_AUTO:
			printf("%s: RK_BT_DISC_STOPPED_AUTO\n", PRINT_FLAG_RKBTSOURCE);
			rk_bt_send_scan_msg("rkbt scan off", strlen("rkbt scan off"));
			break;
		case RK_BT_DISC_START_FAILED:
			printf("%s: RK_BT_DISC_START_FAILED\n", PRINT_FLAG_RKBTSOURCE);
			rk_bt_send_scan_msg("rkbt scan off", strlen("rkbt scan off"));
			break;
		case RK_BT_DISC_STOPPED_BY_USER:
			printf("%s: RK_BT_DISC_STOPPED_BY_USER\n", PRINT_FLAG_RKBTSOURCE);
			rk_bt_send_scan_msg("rkbt scan off", strlen("rkbt scan off"));
			break;
	}
}

static void rk_bt_dev_found_cb(const char *address, const char *name, unsigned int bt_class, int rssi)
{
	/*INVALID = 0, SOURCE = 1, SINK = 2*/
	if(rk_bt_get_playrole_by_addr(address) == PLAYROLE_TYPE_SINK) {
		int i, name_len, scan_msg_len = 0;
		scan_devices_t *scan_devices;
		scan_msg_t *scan_msg;
		char buff[2048];
		char *offset;

		printf("%s: device is found\n", PRINT_FLAG_RKBTSOURCE);
		printf("		address: %s\n", address);
		printf("		name: %s\n", name);
		printf("		class: 0x%x\n", bt_class);
		printf("		rssi: %d\n", rssi);

		for(i = 0; i < scan_devices_count; i++) {
			if(!strcmp(scan_devices_bak[i].name, name) && !strcmp(scan_devices_bak[i].addr, address)) {
				printf("%s: just rssi change(%s, %s)\n", PRINT_FLAG_RKBTSOURCE, name, address);
				return;
			}
		}

		memset(buff, 0, sizeof(buff));
		scan_msg = (scan_msg_t *)buff;
		scan_msg->magic[0] = 'r';
		scan_msg->magic[1] = 'k';
		scan_msg->magic[2] = 'b';
		scan_msg->magic[3] = 't';
		scan_msg->start_flag = 0x01;
		scan_msg->devices_cnt = 1;
		offset = buff + sizeof(scan_msg_t);

		scan_devices = (scan_devices_t *)(offset);
		name_len = strlen(name) > (sizeof(scan_devices->name) - 1) ? (sizeof(scan_devices->name) - 1) : strlen(name);
		memcpy(scan_devices->name, name, name_len);
		scan_devices->name[name_len] = '\0';

		memcpy(scan_devices->addr, address, 17);
		scan_devices->addr[17] = '\0';

		if(rssi == 0x7FFF)
			scan_devices->rssi = 0xFF;
		else
			scan_devices->rssi = rssi;

		if(scan_devices_count < SCAN_DEVICES_SAVE_COUNT) {
			memcpy(&(scan_devices_bak[scan_devices_count]), scan_devices, sizeof(scan_devices_t));
			scan_devices_count++;
		}

		offset += sizeof(scan_devices_t);
		*offset = 0x04;
		scan_msg_len = sizeof(scan_msg_t) + scan_msg->devices_cnt * sizeof(scan_devices_t) + 1;

		rk_bt_send_scan_msg(buff, scan_msg_len);
	}
}

void rk_bt_source_status_callback(void *userdata, const RK_BT_SOURCE_EVENT enEvent)
{
	switch(enEvent) {
		case BT_SOURCE_EVENT_CONNECT_FAILED:
			printf("%s: BT_SOURCE_EVENT_CONNECT_FAILED\n", PRINT_FLAG_RKBTSOURCE);
			break;
		case BT_SOURCE_EVENT_CONNECTED:
			printf("%s: BT_SOURCE_EVENT_CONNECTED\n", PRINT_FLAG_RKBTSOURCE);
			break;
		case BT_SOURCE_EVENT_DISCONNECTED:
			printf("%s: BT_SOURCE_EVENT_DISCONNECTED\n", PRINT_FLAG_RKBTSOURCE);
			break;
		case BT_SOURCE_EVENT_RC_PLAY:
			printf("%s: BT_SOURCE_EVENT_RC_PLAY\n", PRINT_FLAG_RKBTSOURCE);
			break;
		case BT_SOURCE_EVENT_RC_STOP:
			printf("%s: BT_SOURCE_EVENT_RC_STOP\n", PRINT_FLAG_RKBTSOURCE);
			break;
		case BT_SOURCE_EVENT_RC_PAUSE:
			printf("%s: BT_SOURCE_EVENT_RC_PAUSE\n", PRINT_FLAG_RKBTSOURCE);
			break;
		case BT_SOURCE_EVENT_RC_FORWARD:
			printf("%s: BT_SOURCE_EVENT_RC_FORWARD\n", PRINT_FLAG_RKBTSOURCE);
			break;
		case BT_SOURCE_EVENT_RC_BACKWARD:
			printf("%s: BT_SOURCE_EVENT_RC_BACKWARD\n", PRINT_FLAG_RKBTSOURCE);
			break;
		case BT_SOURCE_EVENT_RC_VOL_UP:
			printf("%s: BT_SOURCE_EVENT_RC_VOL_UP\n", PRINT_FLAG_RKBTSOURCE);
			break;
		case BT_SOURCE_EVENT_RC_VOL_DOWN:
			printf("%s: BT_SOURCE_EVENT_RC_VOL_DOWN\n", PRINT_FLAG_RKBTSOURCE);
			break;
	}
}

static int rk_bt_server_init(const char *name)
{
	static RkBtContent bt_content;

	printf("%s: BT SERVER INIT(%s)\n", PRINT_FLAG_RKBTSOURCE, name);
	memset (&bt_content, 0, sizeof(bt_content));
	bt_content.bt_name = name;
	bt_content.ble_content.ble_name = "ROCKCHIP_AUDIO BLE";
	bt_content.ble_content.server_uuid.uuid = BLE_UUID_SERVICE;
	bt_content.ble_content.server_uuid.len = UUID_128;
	bt_content.ble_content.chr_uuid[0].uuid = BLE_UUID_WIFI_CHAR;
	bt_content.ble_content.chr_uuid[0].len = UUID_128;
	bt_content.ble_content.chr_uuid[1].uuid = BLE_UUID_SEND;
	bt_content.ble_content.chr_uuid[1].len = UUID_128;
	bt_content.ble_content.chr_uuid[2].uuid = BLE_UUID_RECV;
	bt_content.ble_content.chr_uuid[2].len = UUID_128;
	bt_content.ble_content.chr_cnt = 3;
	bt_content.ble_content.advDataType = BLE_ADVDATA_TYPE_SYSTEM;
	bt_content.ble_content.cb_ble_recv_fun = NULL;
	bt_content.ble_content.cb_ble_request_data = NULL;

	rk_bt_register_state_callback(rk_bt_state_cb);
	rk_bt_register_bond_callback(rk_bt_bond_state_cb);
	rk_bt_register_discovery_callback(rk_bt_discovery_status_cb);
	rk_bt_register_dev_found_callback(rk_bt_dev_found_cb);
	return rk_bt_init(&bt_content);
}

static int rk_bt_server_deinit()
{
	printf("%s: BT SERVER DEINIT\n", PRINT_FLAG_RKBTSOURCE);
	return rk_bt_deinit();
}

int main(int argc, char *argv[])
{
	int i, item_cnt, ret;
	char buff[128] = {0};
	bt_msg_t *msg;
	struct sockaddr_un serverAddr;

	RK_read_version(buff, 128);
	printf("====== Version:%s =====\n", buff);
	memset(buff, 0, sizeof(buff));

	sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		printf("%s: Create socket failed!\n", PRINT_FLAG_ERR);
		return -1;
	}

	/* Set server address */
	system("rm -rf /tmp/rockchip_btsource_server");
	serverAddr.sun_family = AF_UNIX;
	strcpy(serverAddr.sun_path, "/tmp/rockchip_btsource_server");
	ret = bind(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
	if (ret < 0) {
		printf("%s: Bind Local addr failed!\n", PRINT_FLAG_ERR);
		goto fail;
	}

	msg = (bt_msg_t *)buff;
	item_cnt = sizeof(bt_command_table) / sizeof(bt_command_t);

	while(1) {
		memset(buff, 0, sizeof(buff));
		ret = recv(sockfd, buff, sizeof(buff), 0);
		if (ret <= 0) {
			printf("%s: Recv cmd failed! ret = %d\n", PRINT_FLAG_ERR, ret);
			goto fail;
		}

		for (i = 0; i < item_cnt; i++) {
			if (!strncmp(msg->cmd, bt_command_table[i].cmd, strlen(bt_command_table[i].cmd)))
				break;
		}

		if (i >= item_cnt) {
			printf("%s: Invalid cmd(%s) recved!\n", PRINT_FLAG_ERR, buff);
			continue;
		}

		switch (bt_command_table[i].cmd_id) {
		case RK_BT_SOURCE_INIT:
			if (strlen(msg->name))
				ret = rk_bt_server_init(msg->name);
			else
				ret = rk_bt_server_init("ROCKCHIP_AUDIO");

			if(ret < 0) {
				printf("%s: bt server init failed!\n", PRINT_FLAG_ERR);
				goto fail;
			}

			rk_bt_source_register_status_cb(NULL, rk_bt_source_status_callback);
			if(rk_bt_source_open() < 0) {
				printf("%s: bt source open failed!\n", PRINT_FLAG_ERR);
				goto fail;
			}

			printf("%s: bt server init sucessful!\n", PRINT_FLAG_SUCESS);
			//system("echo 'bt server init sucessful' > /tmp/rk_bt.log");
			break;

		case RK_BT_SOURCE_CONNECT:
			if (rk_bt_source_connect_by_addr(msg->addr) < 0)
				printf("%s: rk_bt_source_connect_by_addr %s failed!\n", PRINT_FLAG_ERR, msg->addr);
			break;

		case RK_BT_SOURCE_SCAN_ON:
			memset(scan_devices_bak, 0, sizeof(scan_devices_t) * SCAN_DEVICES_SAVE_COUNT);
			scan_devices_count = 0;
			/* Scan bluetooth devices, 10s for default*/
			if(rk_bt_start_discovery(10000, SCAN_TYPE_AUTO) < 0)
				printf("%s: rk_bt_start_discovery failed\n", PRINT_FLAG_ERR);
			break;

		case RK_BT_SOURCE_SCAN_OFF:
			rk_bt_cancel_discovery();
			break;

		case RK_BT_SOURCE_DISCONNECT:
			if (rk_bt_source_disconnect_by_addr(msg->addr) < 0)
				printf("%s: rk_bt_source_disconnect_by_addr failed!\n", PRINT_FLAG_ERR);
			break;

		case RK_BT_SOURCE_REMOVE:
			if (rk_bt_source_remove(msg->addr) < 0)
				printf("%s: remove failed!\n", PRINT_FLAG_ERR);
			else
				printf("%s: remove sucess!\n", PRINT_FLAG_SUCESS);
			break;

		case RK_BT_SOURCE_DEINIT:
			rk_bt_server_deinit();

			if(sockfd) {
				close(sockfd);
				printf("%s: close server socket\n", PRINT_FLAG_RKBTSOURCE);
				sockfd = 0;
			}

			printf("%s:deinit bt server sucess!\n", PRINT_FLAG_SUCESS);
			return 0;

		default:
			break;
		}
	}

fail:
	printf("%s: rkbtsource_server failed and exit\n", PRINT_FLAG_ERR);
	if(sockfd) {
		close(sockfd);
		printf("%s: close server socket\n", PRINT_FLAG_RKBTSOURCE);
		sockfd = 0;
	}

	rk_bt_server_deinit();
	return -1;
}
