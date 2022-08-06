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

static void show_help(char *bin_name) {
	unsigned int i;
	printf("%s [Usage]:\n", bin_name);
	for (i = 0; i < sizeof(bt_command_table)/sizeof(bt_command_t); i++)
		printf("\t\"%s %s\":%s.\n", bin_name, bt_command_table[i].cmd, bt_command_table[i].desc);
}

int main(int argc, char *argv[])
{
	char buff[100] = {0};
	char ret_buff[4] = {0};
	struct sockaddr_un serverAddr;
	struct sockaddr_un clientAddr;
	int sockfd, i, item_cnt, ret;
	char scan_result[256];
	scan_devices_t *scan_devices;
	scan_msg_t *scan_msg;
	bt_msg_t *msg;

	if (argc < 2) {
		printf("%s: Invalid argument!\n", PRINT_FLAG_ERR);
		show_help(argv[0]);
		return -1;
	}

	item_cnt = sizeof(bt_command_table) / sizeof(bt_command_t);
	for (i = 0; i < item_cnt; i++) {
		if (!strncmp(argv[1], bt_command_table[i].cmd, strlen(bt_command_table[i].cmd))) {
			if ((i > 3) && ((argc != 3) || (strlen(argv[2]) != 17))) {
				printf("%s: Invalid argument!\n", PRINT_FLAG_ERR);
				show_help(argv[0]);
				return -1;
			}
			break;
		}
	}

	if (i >= item_cnt) {
		printf("%s: Invalid argument!\n", PRINT_FLAG_ERR);
		show_help(argv[0]);
		return -1;
	}

	memset(buff, 0, sizeof(buff));
	msg = (bt_msg_t *)buff;
	memcpy(msg->cmd, bt_command_table[i].cmd, strlen(bt_command_table[i].cmd));
	if (!strncmp(argv[1], "init", 4)) {
		exec_command("pidof rksource_server", ret_buff, 4);
		if (!ret_buff[0]) {
			system("rkbtsource_server > /tmp/rkbtsource_server.log 2>&1 &");
			//system("rkbtsource_server &");
			msleep(100);
		}
		if (argc == 3)
			memcpy(msg->name, argv[2], strlen(argv[2]));
	} else if (i > 3) {
		memcpy(msg->addr, argv[2], 17);
	}

	sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		printf("%s: Socket create failed!\n", PRINT_FLAG_ERR);
		return -1;
	}

	/* Set server address */
	serverAddr.sun_family = AF_UNIX;
	strcpy(serverAddr.sun_path, "/tmp/rockchip_btsource_server");

	/* Set client address */
	system("rm /tmp/rockchip_btsource_client -rf");
	clientAddr.sun_family = AF_UNIX;
	strcpy(clientAddr.sun_path, "/tmp/rockchip_btsource_client");
	ret = bind(sockfd, (struct sockaddr *)&clientAddr, sizeof(clientAddr));
	if (ret < 0) {
		printf("%s: Socket bind failed! ret = %d\n", PRINT_FLAG_ERR, ret);
		close(sockfd);
		return -1;
	}

	ret = connect(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
	if (ret < 0) {
		printf("%s: Socket connect failed! ret = %d\n", PRINT_FLAG_ERR, ret);
		close(sockfd);
		return -1;
	}

	ret = send(sockfd, buff, sizeof(bt_msg_t), MSG_DONTWAIT);
	if (ret < 0) {
		printf("%s: Socket send failed! ret = %d\n", PRINT_FLAG_ERR, ret);
		close(sockfd);
		return -1;
	}

	if (!strncmp(argv[1], "scan", 4)) {
		while(1) {
			memset(scan_result, 0, sizeof(scan_result));
			ret = recv(sockfd, scan_result, sizeof(scan_result), 0);
			if (ret <= 0) {
				printf("%s: Socket recv failed!\n", PRINT_FLAG_ERR);
				goto OUT;
			}

			if(!strncmp(scan_result, "rkbt scan off", strlen("rkbt scan off"))) {
				//printf("%s:scan off\n", PRINT_FLAG_SCAN);
				goto OUT;
			}

			scan_msg = (scan_msg_t *)scan_result;
			if ((scan_msg->magic[0] != 'r') || (scan_msg->magic[1] != 'k') ||
				(scan_msg->magic[2] != 'b') || (scan_msg->magic[3] != 't') ||
				(scan_msg->start_flag != 0x01) || (scan_result[ret - 1] != 0x04)) {
				printf("%s: Scan msg format error!\n", PRINT_FLAG_ERR);
				goto OUT;
			}

			scan_devices = (scan_devices_t *) (scan_result + sizeof(scan_msg_t));
			printf("%s:%s,%d,%s;\n", PRINT_FLAG_SCAN, scan_devices->name, scan_devices->rssi, scan_devices->addr);
		}
	}

OUT:
	if(sockfd) {
		close(sockfd);
		//printf("%s: close client socket\n", PRINT_FLAG_RKBTSOURCE);
		sockfd = 0;
	}

	return 0;
}
