#ifndef __BLUEZ_CTRL_P__
#define __BLUEZ_CTRL_P__

#include "avrcpctrl.h"
#include "gatt_config.h"
#include "a2dp_source/a2dp_masterctrl.h"
#include "a2dp_source/shell.h"
#include "DeviceIo.h"
#include "RkBtSource.h"
#include "RkBtSink.h"

#define HOSTNAME_MAX_LEN	250	/* 255 - 3 (FQDN) - 2 (DNS enc) */

typedef struct {
	pthread_t tid;
	bool is_bt_open;
	bool is_ble_open;
	bool is_ble_client_open;
	bool is_a2dp_sink_open;
	bool is_a2dp_source_open;
	bool is_hfp_open;
} bt_control_t;

bool bt_is_open();
bool ble_is_open();
bool ble_client_is_open();
bool bt_source_is_open(void);
bool bt_sink_is_open(void);
bool bt_hfp_is_open(void);
int bt_interface(enum BtControl type, void *data);
void bt_close_ble(bool disconnect);
int bt_close_sink(bool disconnect);
int bt_close_source(bool disconnect);
int bt_control_cmd_send(enum BtControl bt_ctrl_cmd);
int bt_gethostname(char *hostname_buf, const size_t size);
int rk_bt_control(enum BtControl cmd, void *data, int len);

//#define msleep(x) usleep(x * 1000)

#endif /* __BLUEZ_CTRL_P__ */
