/*
 * (C) Copyright 2008-2018 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __SPP_SERVER_
#define __SPP_SERVER_

#include "RkBtSpp.h"

#ifdef __cplusplus
extern "C" {
#endif

int bt_spp_server_open();
void bt_spp_register_recv_callback(RK_BT_SPP_RECV_CALLBACK cb);
void bt_spp_register_status_callback(RK_BT_SPP_STATUS_CALLBACK cb);
void bt_spp_server_close();
int bt_spp_write(char *data, int len);
RK_BT_SPP_STATE bt_spp_get_status();
int bt_spp_set_channel(int channel);
int bt_spp_get_channel();

#ifdef __cplusplus
}
#endif

#endif /* __SPP_SERVER_ */
