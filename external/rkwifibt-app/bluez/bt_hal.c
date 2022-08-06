#include <errno.h>
#include <fcntl.h>
#include <paths.h>
#include <string.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>

#include <DeviceIo.h>
#include <Rk_wifi.h>
#include <RkBtBase.h>
#include <RkBle.h>
#include <RkBtSource.h>
#include <RkBtHfp.h>
#include <RkBleClient.h>
#include <RkBtObex.h>
#include <RkBtPan.h>

#include "avrcpctrl.h"
#include "bluez_ctrl.h"
#include "a2dp_source/a2dp_masterctrl.h"
#include "a2dp_source/shell.h"
#include "spp_server/spp_server.h"
#include "bluez_alsa_client/ctl-client.h"
#include "bluez_alsa_client/rfcomm_msg.h"
#include "obex_client.h"
#include "gatt_client.h"
#include "utility.h"
#include "slog.h"

extern volatile bt_control_t bt_control;

#define BT_CONFIG_FAILED 2
#define BT_CONFIG_OK 1

typedef struct {
	int sockfd;
	pthread_t tid;
	RK_BT_SINK_UNDERRUN_CB cb;
} underrun_handler_t;

static underrun_handler_t g_underrun_handler = {
	-1, 0, NULL,
};

static int bt_hal_source_close(bool disconnect);
static int bt_hal_hfp_close(bool disconnect);
static int bt_hal_ble_stop(bool disconnect);
static void bt_hal_ble_client_close(bool disconnect);
static int bt_hal_sink_close(bool disconnect);

#if 0
void rk_printf_system_time(char *tag)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);

	printf("---%s: time: %lld ms\n", tag, tv.tv_sec * 1000 + tv.tv_usec/1000 + tv.tv_usec%1000);
}
#endif

/*****************************************************************
 *            Rockchip bluetooth LE api                      *
 *****************************************************************/
int rk_bt_ble_set_visibility(const int visiable, const int connect)
{
	pr_info("bluez don't support %s\n", __func__);
	return 0;
}

int rk_ble_start(RkBleContent *ble_content)
{
	if (!bt_is_open()) {
		pr_info("%s: Please open bt!!!\n", __func__);
		return -1;
	}

	if(ble_is_open()) {
		pr_info("%s: ble has been opened\n", __func__);
		return -1;
	}

	pr_info("%s, tid(%lu)\n", __func__, pthread_self());
	if(ble_client_is_open()) {
		pr_info("ble client has been opened, close ble client\n");
		rk_ble_client_close();
	}

	if(rk_bt_control(BT_BLE_OPEN, NULL, 0) < 0)
		return -1;

	ble_state_send(RK_BLE_STATE_IDLE);
	return 0;
}

static int bt_hal_ble_stop(bool disconnect)
{
	if (!ble_is_open()) {
		pr_info("%s: ble has been closed\n", __func__);
		return -1;
	}

	pr_info("%s, tid(%lu)\n", __func__, pthread_self());
	bt_close_ble(disconnect);
	return 0;
}

int rk_ble_stop()
{
	return bt_hal_ble_stop(true);
}

int rk_ble_get_state(RK_BLE_STATE *p_state)
{
	ble_get_state(p_state);
	return 0;
}

int rk_ble_set_address(char *address)
{
	return ble_set_address(address);
}

int rk_ble_write(const char *uuid, char *data, int len)
{
	RkBleConfig ble_cfg;

	if (!ble_is_open()) {
		pr_info("ble isn't open, please open\n");
		return -1;
	}

	ble_cfg.len = len > BT_ATT_MAX_VALUE_LEN ? BT_ATT_MAX_VALUE_LEN : len;
	memcpy(ble_cfg.data, data, ble_cfg.len);
	strcpy(ble_cfg.uuid, uuid);
	rk_bt_control(BT_BLE_WRITE, &ble_cfg, sizeof(RkBleConfig));

	return 0;
}

int rk_ble_register_status_callback(RK_BLE_STATE_CALLBACK cb)
{
	ble_register_state_callback(cb);
	return 0;
}

void rk_ble_register_mtu_callback(RK_BT_MTU_CALLBACK cb)
{
	ble_register_mtu_callback(cb);
}

int rk_ble_register_recv_callback(RK_BLE_RECV_CALLBACK cb)
{
	if (cb) {
		pr_info("BlueZ does not support this interface."
			"Please set the callback function when init BT.\n");
	}

	return 0;
}

void rk_ble_register_request_data_callback(RK_BLE_REQUEST_DATA cb)
{
	if (cb) {
		pr_info("BlueZ does not support this interface."
			"Please set the callback function when init BT.\n");
	}
}

int rk_ble_disconnect()
{
	if (!ble_is_open()) {
		pr_info("%s: ble isn't open, please open\n", __func__);
		return -1;
	}

	pr_info("%s, tid(%lu)\n", __func__, pthread_self());
	return ble_disconnect();
}

void rk_ble_set_local_privacy(bool local_privacy)
{
	pr_info("bluez don't support %s\n", __func__);
}

int rk_ble_set_adv_interval(unsigned short adv_int_min, unsigned short adv_int_max)
{
	return ble_set_adv_interval(adv_int_min, adv_int_max);
}

/*****************************************************************
 *            Rockchip bluetooth LE Client api                      *
 *****************************************************************/
void rk_ble_client_register_state_callback(RK_BLE_CLIENT_STATE_CALLBACK cb)
{
	gatt_client_register_state_callback(cb);
}

void rk_ble_client_register_recv_callback(RK_BLE_CLIENT_RECV_CALLBACK cb)
{
	gatt_client_register_recv_callback(cb);
}

void rk_ble_client_register_mtu_callback(RK_BT_MTU_CALLBACK cb)
{
	ble_register_mtu_callback(cb);
}

int rk_ble_client_open(bool mtu_change)
{
	if (!bt_is_open()) {
		pr_info("%s: Please open bt!!!\n", __func__);
		return -1;
	}

	if(ble_client_is_open()) {
		pr_info("%s: ble client has been opened\n", __func__);
		return -1;
	}

	if(ble_is_open()) {
		pr_info("%s: ble has been opened, close ble\n", __func__);
		rk_ble_stop();
	}

	gatt_client_open();
	bt_control.is_ble_client_open = true;
	gatt_client_state_send(RK_BLE_CLIENT_STATE_IDLE);
	return 0;
}

static void bt_hal_ble_client_close(bool disconnect)
{
	if(!ble_client_is_open()) {
		pr_info("%s: ble client has been closed\n", __func__);
		return;
	}

	if(disconnect) {
		if(!ble_disconnect())
			sleep(3);
	}

	gatt_client_close();
	bt_control.is_ble_client_open = false;
}

void rk_ble_client_close()
{
	bt_hal_ble_client_close(true);
}

RK_BLE_CLIENT_STATE rk_ble_client_get_state()
{
	if(!ble_client_is_open()) {
		pr_info("%s: ble client isn't open, please open\n", __func__);
		return -1;
	}

	return gatt_client_get_state();
}

int rk_ble_client_connect(char *address)
{
	if(!ble_client_is_open()) {
		pr_info("%s: ble client isn't open, please open\n", __func__);
		return -1;
	}

	return connect_by_address(address);
}

int rk_ble_client_disconnect(char *address)
{
	if(!ble_client_is_open()) {
		pr_info("%s: ble client isn't open, please open\n", __func__);
		return -1;
	}

	return ble_disconnect();
}

int rk_ble_client_get_service_info(char *address, RK_BLE_CLIENT_SERVICE_INFO *info)
{
	if(!ble_client_is_open()) {
		pr_info("%s: ble client isn't open, please open\n", __func__);
		return -1;
	}

	return gatt_client_get_service_info(address, info);
}

int rk_ble_client_read(const char *uuid)
{
	if(!ble_client_is_open()) {
		pr_info("%s: ble client isn't open, please open\n", __func__);
		return -1;
	}

	return gatt_client_read(uuid, 0);
}

int rk_ble_client_write(const char *uuid, char *data, int data_len)
{
	if(!ble_client_is_open()) {
		pr_info("%s: ble client isn't open, please open\n", __func__);
		return -1;
	}

	return gatt_client_write(uuid, data, data_len, 0);
}

bool rk_ble_client_is_notifying(const char *uuid)
{
	if(!ble_client_is_open()) {
		pr_info("%s: ble client isn't open, please open\n", __func__);
		return -1;
	}

	return gatt_client_is_notifying(uuid);
}

int rk_ble_client_notify(const char *uuid, bool is_indicate, bool enable)
{
	if(!ble_client_is_open()) {
		pr_info("%s: ble client isn't open, please open\n", __func__);
		return -1;
	}

	return gatt_client_notify(uuid, enable);
}

int rk_ble_client_get_eir_data(char *address, char *eir_data, int len)
{
	if(!ble_client_is_open()) {
		pr_err("ble client isn't open, please open\n");
		return -1;
	}

	return gatt_client_get_eir_data(address, eir_data, len);
}

int rk_ble_client_default_data_length()
{
	pr_info("bluez don't support %s\n", __func__);
	return -1;
}

/*****************************************************************
 *            Rockchip bluetooth master api                      *
 *****************************************************************/
static pthread_t g_btmaster_thread = 0;

static void* _btmaster_autoscan_and_connect(void *data)
{
	BtScanParam scan_param;
	BtDeviceInfo *start;
	int max_rssi = -100;
	int ret = 0;
	char target_address[17] = {0};
	bool target_vaild = false;
	int scan_cnt, i;

	/* Scan bluetooth devices */
	scan_param.mseconds = 10000; /* 10s for default */
	scan_param.item_cnt = 0;
	scan_cnt = 3;

	prctl(PR_SET_NAME,"_btmaster_autoscan_and_connect");

scan_retry:
	pr_info("=== BT_SOURCE_SCAN ===\n");
	ret = rk_bt_source_scan(&scan_param);
	if (ret && (scan_cnt--)) {
		sleep(1);
		goto scan_retry;
	} else if (ret) {
		pr_info("%s: ERROR: Scan error!\n", __func__);
		a2dp_master_event_send(BT_SOURCE_EVENT_CONNECT_FAILED, "", "");
		return NULL;
	}

	/*
	 * Find the audioSink device from the device list,
	 * which has the largest rssi value.
	 */
	max_rssi = -100;
	for (i = 0; i < scan_param.item_cnt; i++) {
		start = &scan_param.devices[i];
		if (start->rssi_valid && (start->rssi > max_rssi) &&
			(!strcmp(start->playrole, "Audio Sink"))) {
			pr_info("#%02d Name:%s\n", i, start->name);
			pr_info("\tAddress:%s\n", start->address);
			pr_info("\tRSSI:%d\n", start->rssi);
			pr_info("\tPlayrole:%s\n", start->playrole);
			max_rssi = start->rssi;

			memcpy(target_address, start->address, 17);
			target_vaild = true;
		}
	}

	if (!target_vaild) {
		pr_info("=== Cannot find audio Sink devices. ===\n");
		a2dp_master_event_send(BT_SOURCE_EVENT_CONNECT_FAILED, "", "");
		return NULL;
	} else if (max_rssi < -80) {
		pr_info("=== BT SOURCE RSSI is is too weak !!! ===\n");
		a2dp_master_event_send(BT_SOURCE_EVENT_CONNECT_FAILED, "", "");
		return NULL;
	}

	/* Connect target device */
	if (!a2dp_master_status(NULL, 0, NULL, 0))
		a2dp_master_connect(target_address);

	pr_info("%s: Exit _btmaster_autoscan_and_connect thread!\n", __func__);
	return NULL;
}

/*
 * Turn on Bluetooth and scan SINK devices.
 * Features:
 *     1. turn on Bluetooth
 *     2. enter the bt source mode
 *     3. Scan surrounding SINK type devices
 *     4. If the SINK device is found, the device with the strongest
 *        signal is automatically connected.
 * Return:
 *     0: The function is executed successfully and needs to listen
 *        for Bluetooth connection events.
 *    -1: Function execution failed.
 */
int rk_bt_source_auto_connect_start(void *userdata, RK_BT_SOURCE_CALLBACK cb)
{
	int ret;

	if (!bt_is_open()) {
		pr_info("%s: Please open bt!!!\n", __func__);
		return -1;
	}

	if (g_btmaster_thread) {
		pr_info("%s: The last operation is still in progress, please stop then start\n", __func__);
		return -1;
	}

	/* Register callback and userdata */
	rk_bt_source_register_status_cb(userdata, cb);

	ret = rk_bt_source_open();
	if (ret < 0)
		return ret;

	/* Create thread to do connect task. */
	ret = pthread_create(&g_btmaster_thread, NULL,
						 _btmaster_autoscan_and_connect, NULL);
	if (ret) {
		pr_info("%s: _btmaster_autoscan_and_connect thread create failed!\n", __func__);
		return -1;
	}

	return 0;
}

int rk_bt_source_auto_connect_stop(void)
{
	if (g_btmaster_thread) {
		pthread_join(g_btmaster_thread, NULL);
		g_btmaster_thread = 0;
	}

	return rk_bt_source_close();
}

int rk_bt_source_open()
{
	pr_info("%s, tid(%lu)\n", __func__, pthread_self());
	if (!bt_is_open()) {
		pr_err("%s: Please open bt!!!\n", __func__);
		return -1;
	}

	if (g_btmaster_thread) {
		pr_err("%s: The last operation is still in progress\n", __func__);
		return -1;
	}

	if (bt_source_is_open()) {
		pr_info("%s: bt source has been opened\n", __func__);
		return -1;
	}

	if (bt_sink_is_open()){
		pr_info("%s: bt sink has been opened, close bt sink", __func__);
		rk_bt_sink_close();
	}

	if (bt_interface(BT_SOURCE_OPEN, NULL) < 0) {
		bt_control.is_a2dp_source_open = false;
		return -1;
	}

	reconn_last_devices(BT_DEVICES_A2DP_SINK);
	bt_control.is_a2dp_source_open = true;

	return 0;
}

static int bt_hal_source_close(bool disconnect)
{
	pr_info("%s, tid(%lu)\n", __func__, pthread_self());
	if(rk_bt_is_discovering())
		rk_bt_cancel_discovery();

	if (!bt_source_is_open()) {
		pr_info("%s: bt source has been closed\n", __func__);
		return -1;
	}

	a2dp_master_save_status(NULL);
	source_set_reconnect_tag(false);
	bt_close_source(disconnect);
	a2dp_master_deregister_cb();

	bt_control.is_a2dp_source_open = false;
	return 0;
}

int rk_bt_source_close()
{
	return bt_hal_source_close(true);
}

int rk_bt_source_scan(BtScanParam *data)
{
	if (!bt_source_is_open()) {
		pr_info("%s: bt source isn't open, please open\n", __func__);
		return -1;
	}

	source_set_reconnect_tag(false);
	return a2dp_master_scan(data, sizeof(BtScanParam), SCAN_TYPE_BREDR);
}

int rk_bt_source_connect_by_addr(char *address)
{
	char bd_addr[18];
	memset(bd_addr, 0, 18);

	if (!bt_source_is_open()) {
		pr_info("%s: bt source isn't open, please open\n", __func__);
		return -1;
	}

	pr_info("%s thread tid = %lu\n", __func__, pthread_self());
	return a2dp_master_connect(address);
}

int rk_bt_source_disconnect_by_addr(char *address)
{
	if (!bt_source_is_open()) {
		pr_info("%s: bt source isn't open, please open\n", __func__);
		return -1;
	}

	pr_info("%s thread tid = %lu\n", __func__, pthread_self());
	return disconnect_by_address(address);
}

int rk_bt_source_disconnect()
{
	if (!bt_source_is_open()) {
		pr_info("%s: bt source isn't open, please open\n", __func__);
		return -1;
	}

	return disconnect_current_devices();
}

int rk_bt_source_remove(char *address)
{
	if (!bt_source_is_open()) {
		pr_info("%s: bt source isn't open, please open\n", __func__);
		return -1;
	}

	return remove_by_address(address);
}

int rk_bt_source_register_status_cb(void *userdata, RK_BT_SOURCE_CALLBACK cb)
{
	a2dp_master_register_cb(userdata,  cb);
	return 0;
}

int rk_bt_source_get_device_name(char *name, int len)
{
	return rk_bt_get_device_name(name, len);
}

int rk_bt_source_get_device_addr(char *addr, int len)
{
	return rk_bt_get_device_addr(addr, len);
}

int rk_bt_source_get_status(RK_BT_SOURCE_STATUS *pstatus, char *name, int name_len,
                                    char *address, int addr_len)
{
	if (!pstatus)
		return 0;

	if (a2dp_master_status(address, addr_len, name, name_len))
		*pstatus = BT_SOURCE_STATUS_CONNECTED;
	else
		*pstatus = BT_SOURCE_STATUS_DISCONNECTED;

	return 0;
}

int rk_bt_source_resume()
{
	pr_info("bluez don't support %s\n", __func__);
	return 0;
}

int rk_bt_source_stop()
{
	pr_info("bluez don't support %s\n", __func__);
	return 0;
}

int rk_bt_source_pause()
{
	pr_info("bluez don't support %s\n", __func__);
	return 0;
}

int rk_bt_source_vol_up()
{
	pr_info("bluez don't support %s\n", __func__);
	return 0;
}

int rk_bt_source_vol_down()
{
	pr_info("bluez don't support %s\n", __func__);
	return 0;
}

int rk_bt_source_set_vol(int vol)
{
	pr_info("bluez don't support %s\n", __func__);
	return 0;
}

/*****************************************************************
 *            Rockchip bluetooth sink api                        *
 *****************************************************************/
void *sink_underrun_listen(void *arg)
{
	int ret = 0;
	char buff[100] = {0};
	struct sockaddr_un clientAddr;
	struct sockaddr_un serverAddr;
	socklen_t addr_len;

	g_underrun_handler.sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (g_underrun_handler.sockfd < 0) {
		pr_err("%s: Create socket failed!\n", __func__);
		return NULL;
	}

	serverAddr.sun_family = AF_UNIX;
	strcpy(serverAddr.sun_path, "/tmp/rk_deviceio_a2dp_underrun");

	system("rm -rf /tmp/rk_deviceio_a2dp_underrun");
	ret = bind(g_underrun_handler.sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
	if (ret < 0) {
		pr_err("%s: Bind Local addr failed!\n", __func__);
		return NULL;
	}

	pr_info("%s: underrun listen...\n", __func__);
	while(1) {
		memset(buff, 0, sizeof(buff));
		ret = recvfrom(g_underrun_handler.sockfd, buff, sizeof(buff), 0, (struct sockaddr *)&clientAddr, &addr_len);
		if (ret <= 0) {
			if (ret == 0)
				pr_info("%s: socket closed!\n", __func__);
			break;
		}
		pr_info("%s: recv a message(%s)\n", __func__, buff);

		if (!bt_sink_is_open())
			break;

		if (!strstr(buff, "a2dp underrun;")) {
			pr_warning("%s: recv a unsupport msg:%s\n", __func__, buff);
			continue;
		}

		if (g_underrun_handler.cb)
			g_underrun_handler.cb();
		else
			break;
	}

	pr_info("%s: Exit underrun listen thread!\n", __func__);
	return NULL;
}

static int underrun_listen_thread_create(RK_BT_SINK_UNDERRUN_CB cb)
{
	pr_info("%s: underrun_listen_thread_create\n", __func__);

	g_underrun_handler.cb = cb;

	/* Create a thread to listen for bluez-alsa sink underrun. */
	if (!g_underrun_handler.tid) {
		if (pthread_create(&g_underrun_handler.tid, NULL, sink_underrun_listen, NULL)) {
			pr_err("%s: Create underrun listen pthread failed\n", __func__);
			return -1;
		}

		pthread_setname_np(g_underrun_handler.tid, "underrun_listen");
	}

	return 0;
}

static void underrun_listen_thread_delete()
{
	pr_debug("%s enter\n", __func__);
	if (g_underrun_handler.sockfd >= 0) {
		shutdown(g_underrun_handler.sockfd, SHUT_RDWR);
		g_underrun_handler.sockfd = -1;
	}

	if(g_underrun_handler.tid) {
		pthread_join(g_underrun_handler.tid, NULL);
		g_underrun_handler.tid = 0;
	}

	g_underrun_handler.cb = NULL;
	pr_debug("%s exit\n", __func__);
}
int rk_bt_sink_register_callback(RK_BT_SINK_CALLBACK cb)
{
	a2dp_sink_register_cb(cb);
	return 0;
}

void rk_bt_sink_register_underurn_callback(RK_BT_SINK_UNDERRUN_CB cb)
{
	underrun_listen_thread_create(cb);
}

int rk_bt_sink_register_volume_callback(RK_BT_SINK_VOLUME_CALLBACK cb)
{
	a2dp_sink_register_volume_cb(cb);
	return 0;
}

int rk_bt_sink_register_track_callback(RK_BT_AVRCP_TRACK_CHANGE_CB cb)
{
	a2dp_sink_register_track_cb(cb);
	return 0;
}

int rk_bt_sink_register_position_callback(RK_BT_AVRCP_PLAY_POSITION_CB cb)
{
	a2dp_sink_register_position_cb(cb);
	return 0;
}

int rk_bt_sink_get_default_dev_addr(char *addr, int len)
{
	return bt_get_default_dev_addr(addr, len);
}

int rk_bt_sink_open()
{
	if (!bt_is_open()) {
		pr_info("%s: Please open bt!!!\n", __func__);
		return -1;
	}

	if (bt_sink_is_open()) {
		pr_info("%s: bt sink has been opened\n", __func__);
		return -1;
	}

	if(bt_source_is_open()) {
		pr_info("%s: bt source has been opened, close bt source", __func__);
		rk_bt_source_close();
	}

	if (bt_interface(BT_SINK_OPEN, NULL) < 0) {
		bt_control.is_a2dp_sink_open = false;
		return -1;
	}

	reconn_last_devices(BT_DEVICES_A2DP_SOURCE);

	bt_control.is_a2dp_sink_open = true;

	return 0;
}

static int bt_hal_sink_close(bool disconnect)
{
	if (!bt_sink_is_open()) {
		pr_info("%s: bt sink has been closed\n", __func__);
		return -1;
	}

	bt_close_sink(disconnect);
	underrun_listen_thread_delete();
	a2dp_sink_clear_cb();
	return 0;
}

int rk_bt_sink_close()
{
	return bt_hal_sink_close(true);
}

int rk_bt_sink_get_state(RK_BT_SINK_STATE *pState)
{
	return a2dp_sink_status(pState);
}

int rk_bt_sink_play(void)
{
	if (bt_control_cmd_send(BT_RESUME_PLAY) < 0)
		return -1;

	return 0;
}

int rk_bt_sink_pause(void)
{
	if (bt_control_cmd_send(BT_PAUSE_PLAY) < 0)
		return -1;

	return 0;
}

int rk_bt_sink_prev(void)
{
	if (bt_control_cmd_send(BT_AVRCP_BWD) < 0)
		return -1;

	return 0;
}

int rk_bt_sink_next(void)
{
	if (bt_control_cmd_send(BT_AVRCP_FWD) < 0)
		return -1;

	return 0;
}

int rk_bt_sink_stop(void)
{
	if (bt_control_cmd_send(BT_AVRCP_STOP) < 0)
		return -1;

	return 0;
}

int rk_bt_sink_get_play_status()
{
	if (!bt_sink_is_open()) {
		pr_info("%s: bt sink isn't open, please open\n", __func__);
		return -1;
	}

	return get_play_status_avrcp();
}

bool rk_bt_sink_get_poschange()
{
	if (!bt_sink_is_open()) {
		pr_info("%s: bt sink isn't open, please open\n", __func__);
		return false;
	}

	return get_poschange_avrcp();
}

int rk_bt_sink_disconnect()
{
	if (!bt_sink_is_open()) {
		pr_info("%s: bt sink isn't open, please open\n", __func__);
		return -1;
	}

	return disconnect_current_devices();
}

int rk_bt_sink_connect_by_addr(char *addr)
{
	if (!bt_sink_is_open()) {
		pr_info("%s: bt sink isn't open, please open\n", __func__);
		return -1;
	}

	return connect_by_address(addr);
}

int rk_bt_sink_disconnect_by_addr(char *addr)
{
	if(bt_sink_is_open()) {
		return disconnect_by_address(addr);
	}

	return -1;
}

static int _get_bluealsa_plugin_volume_ctrl_info(char *name, int *value)
{
	char buff[1024] = {0};
	char ctrl_name[128] = {0};
	int ctrl_value = 0;
	char *start = NULL;
	char *end = NULL;

	if (!name && !value)
		return -1;

	if (name) {
		exec_command("amixer -D bluealsa scontents", buff, sizeof(buff));
		start = strstr(buff, "Simple mixer control ");
		end = strstr(buff, "A2DP'");
		if (!start || (!strstr(start, "A2DP")))
			return -1;

		start += strlen("Simple mixer control '");
		end += strlen("A2DP");
		if ((end - start) < strlen(" - A2DP"))
			return -1;

		memcpy(ctrl_name, start, end-start);
		memcpy(name, ctrl_name, strlen(ctrl_name));
	}

	if (value) {
		start = strstr(buff, "Front Left: Capture ");
		if (!start)
			return -1;

		start += strlen("Front Left: Capture ");
		if ((*start < '0') || (*start > '9'))
			return -1;

		/* Max volume value:127, the length of volume value string must be <= 3 */
		ctrl_value += (*start - '0');
		start++;
		if ((*start >= '0') && (*start <= '9'))
			ctrl_value = 10 * ctrl_value + (*start - '0');
		start++;
		if ((*start >= '0') && (*start <= '9'))
			ctrl_value = 10 * ctrl_value + (*start - '0');

		*value = ctrl_value;
	}

	return 0;
}

static int _set_bluealsa_plugin_volume_ctrl_info(char *name, int value)
{
	char buff[1024] = {0};
	char cmd[256] = {0};
	char ctrl_name[128] = {0};
	int new_volume = 0;

	if (!name)
		return -1;

	sprintf(cmd, "amixer -D bluealsa sset \"%s\" %d", name, value);
	exec_command(cmd, buff, sizeof(buff));

	if (_get_bluealsa_plugin_volume_ctrl_info(ctrl_name, &new_volume) == -1)
		return -1;
	if (new_volume != value)
		return -1;

	return 0;
}

int rk_bt_sink_volume_up(void)
{
	char ctrl_name[128] = {0};
	int current_volume = 0;
	int ret = 0;

	if (!bt_sink_is_open()) {
		pr_info("%s: bt sink isn't open, please open\n", __func__);
		return -1;
	}

	ret = _get_bluealsa_plugin_volume_ctrl_info(ctrl_name, &current_volume);
	if (ret)
		return ret;

	ret = _set_bluealsa_plugin_volume_ctrl_info(ctrl_name, current_volume + 8);
	return ret;
}

int rk_bt_sink_volume_down(void)
{
	char ctrl_name[128] = {0};
	int current_volume = 0;
	int ret = 0;

	if (!bt_sink_is_open()) {
		pr_info("%s: bt sink isn't open, please open\n", __func__);
		return -1;
	}

	ret = _get_bluealsa_plugin_volume_ctrl_info(ctrl_name, &current_volume);
	if (ret)
		return ret;

	if (current_volume < 8)
		current_volume = 0;
	else
		current_volume -= 8;

	ret = _set_bluealsa_plugin_volume_ctrl_info(ctrl_name, current_volume);
	return ret;
}

int rk_bt_sink_set_volume(int volume)
{
	char ctrl_name[128] = {0};
	int new_volume = 0;
	int ret = 0;

	if (!bt_sink_is_open()) {
		pr_info("%s: bt sink isn't open, please open\n", __func__);
		return -1;
	}

	if (volume < 0)
		new_volume = 0;
	else if (volume > 127)
		new_volume = 127;
	else
		new_volume = volume;

	ret = _get_bluealsa_plugin_volume_ctrl_info(ctrl_name, NULL);
	if (ret)
		return ret;

	ret = _set_bluealsa_plugin_volume_ctrl_info(ctrl_name, new_volume);
	return ret;
}

void rk_bt_sink_set_alsa_device(char *alsa_dev)
{
	pr_info("bluez don't support %s\n", __func__);
}

/*****************************************************************
 *            Rockchip bluetooth spp api                         *
 *****************************************************************/
int rk_bt_spp_open()
{
	int ret = 0;

	if (!bt_is_open()) {
		pr_info("%s: Please open bt!!!\n", __func__);
		return -1;
	}

	ret = bt_spp_server_open();
	return ret;
}

int rk_bt_spp_register_status_cb(RK_BT_SPP_STATUS_CALLBACK cb)
{
	bt_spp_register_status_callback(cb);
	return 0;
}

int rk_bt_spp_register_recv_cb(RK_BT_SPP_RECV_CALLBACK cb)
{
	bt_spp_register_recv_callback(cb);
	return 0;
}

int rk_bt_spp_close(void)
{
	bt_spp_server_close();
	return 0;
}

int rk_bt_spp_get_state(RK_BT_SPP_STATE *pState)
{
	if (pState)
		*pState = bt_spp_get_status();

	return 0;
}

int rk_bt_spp_write(char *data, int len)
{
	pr_info("bluez don't support %s\n", __func__);
	return 0;
}

int rk_bt_spp_listen()
{
	pr_info("bluez don't support %s\n", __func__);

	return 0;
}

int rk_bt_spp_connect(char *address)
{
	pr_info("bluez don't support %s\n", __func__);

	return 0;
}

int rk_bt_spp_disconnect(char *address)
{
	pr_info("bluez don't support %s\n", __func__);

	return 0;
}

//====================================================//
static GMainLoop *bt_main_loop = NULL;
static pthread_t main_loop_thread = 0;
GMainContext *Bluez_Context = NULL;

static void *main_loop_init_thread(void *data)
{
#ifdef DefGContext
	bt_main_loop = g_main_loop_new(NULL, FALSE);
	pr_info("%s: bt mainloop run with default context\n", __func__);
#else
	Bluez_Context = g_main_context_new();
	bt_main_loop = g_main_loop_new(Bluez_Context, FALSE);
	pr_info("%s: bt mainloop run with Bluez_Context\n", __func__);
#endif

	g_main_loop_run(bt_main_loop);

	g_main_loop_unref(bt_main_loop);
	bt_main_loop = NULL;

	pr_info("%s: bt mainloop exit\n", __func__);
	return NULL;
}

static int main_loop_init()
{
	if (main_loop_thread)
		return 0;

	if (pthread_create(&main_loop_thread, NULL, main_loop_init_thread, NULL)) {
		pr_err("%s: Create bt mainloop thread failed\n", __func__);
		return -1;
	}

	pthread_setname_np(main_loop_thread, "main_loop_thread");
	return 0;
}

static int main_loop_deinit()
{
	if(bt_main_loop) {
		pr_info("%s bt mainloop quit\n", __func__);
		g_main_loop_quit(bt_main_loop);
	}

	if(main_loop_thread) {
		if (pthread_join(main_loop_thread, NULL)) {
			pr_err("%s: bt mainloop exit failed!\n", __func__);
			return -1;
		} else {
			pr_info("%s: bt mainloop thread exit ok\n", __func__);
		}
		main_loop_thread = 0;
	}

	return 0;
}

static pthread_t rk_bt_init_thread = 0;
static int _rk_bt_init(void *p)
{
	RkBtContent *p_bt_content = p;

	if (bt_is_open()) {
		pr_info("%s: bluetooth has been opened!\n", __func__);
		return -1;
	}

	pr_info("enter %s, tid(%lu)\n", __func__, pthread_self());
	bt_state_send(RK_BT_STATE_TURNING_ON);

	main_loop_init();

	setenv("DBUS_SESSION_BUS_ADDRESS", "unix:path=/var/run/dbus/system_bus_socket", 1);
	if (rk_bt_control(BT_OPEN, p_bt_content, sizeof(RkBtContent))) {
		bt_state_send(RK_BT_STATE_OFF);
		return -1;
	}

	pr_info("exit %s\n", __func__);
	return 0;
}

int rk_bt_init(RkBtContent *p_bt_content)
{
	if (rk_bt_init_thread)
		return 0;

	if (pthread_create(&rk_bt_init_thread, NULL, _rk_bt_init, (void *)p_bt_content)) {
		pr_err("%s: Create rk_bt_init_thread thread failed\n", __func__);
		return -1;
	}

	pthread_setname_np(rk_bt_init_thread, "rk_bt_init_thread");
	pthread_detach(rk_bt_init_thread);
	return 0;
}

int _rk_bt_deinit(void *p)
{
	rk_bt_init_thread = 0;

	if (!bt_is_open()) {
		pr_info("%s: bluetooth has been closed!\n", __func__);
		return -1;
	}

	pr_info("enter %s, tid(%lu)\n", __func__, pthread_self());
	bt_state_send(RK_BT_STATE_TURNING_OFF);

	bt_hal_hfp_close(false);
	bt_hal_sink_close(false);
	bt_hal_source_close(false);
	rk_bt_spp_close();
	bt_hal_ble_stop(false);
	bt_hal_ble_client_close(false);
	rk_bt_obex_pbap_deinit();
	bt_close();

	kill_task("obexd");
	kill_task("bluealsa");
	kill_task("bluealsa-aplay");
	kill_task("bluetoothctl");
	msleep(600);
	kill_task("bluetoothd");
	exec_command_system("hciconfig hci0 down");
	kill_task("rtk_hciattach");
	kill_task("brcm_patchram_plus1");

	main_loop_deinit();

	bt_deregister_bond_callback();
	bt_deregister_discovery_callback();
	bt_deregister_dev_found_callback();
	bt_deregister_name_change_callback();
	bt_control.is_bt_open = false;

	bt_state_send(RK_BT_STATE_OFF);
	bt_deregister_state_callback();
	pr_info("exit %s\n", __func__);

	return 0;
}

static pthread_t rk_bt_deinit_thread = 0;
int rk_bt_deinit()
{
	if (pthread_create(&rk_bt_deinit_thread, NULL, _rk_bt_deinit, NULL)){
		pr_err("%s: Create rk_bt_init_thread thread failed\n", __func__);
		return -1;
	}

	pthread_setname_np(rk_bt_deinit_thread, "rk_bt_deinit_thread");
	pthread_detach(rk_bt_deinit_thread);
	return 0;
}

void rk_bt_register_state_callback(RK_BT_STATE_CALLBACK cb)
{
	bt_register_state_callback(cb);
}

void rk_bt_register_bond_callback(RK_BT_BOND_CALLBACK cb)
{
	bt_register_bond_callback(cb);
}

void rk_bt_register_discovery_callback(RK_BT_DISCOVERY_CALLBACK cb)
{
	bt_register_discovery_callback(cb);
}

void rk_bt_register_dev_found_callback(RK_BT_DEV_FOUND_CALLBACK cb)
{
	bt_register_dev_found_callback(cb);
}

void rk_bt_register_name_change_callback(RK_BT_NAME_CHANGE_CALLBACK cb)
{
	bt_register_name_change_callback(cb);
}

/*
 * / # hcitool con
 * Connections:
 *      > ACL 64:A2:F9:68:1E:7E handle 1 state 1 lm SLAVE AUTH ENCRYPT
 *      > LE 60:9C:59:31:7F:B9 handle 16 state 1 lm SLAVE
 */
int rk_bt_is_connected(void)
{
	return bt_is_connected();
}

int rk_bt_set_class(int value)
{
	char cmd[100] = {0};

	pr_info("#%s value:0x%x\n", __func__, value);
	sprintf(cmd, "hciconfig hci0 class 0x%x", value);
	exec_command_system(cmd);
	msleep(100);

	return 0;
}

int rk_bt_set_sleep_mode()
{
	pr_info("bluez don't support %s\n", __func__);
	return 0;
}

int rk_bt_enable_reconnect(int value)
{
	int ret = 0;
	int fd = 0;

	fd = open("/userdata/cfg/bt_reconnect", O_RDWR | O_CREAT | O_TRUNC, 0666);
	if (fd < 0) {
		pr_err("open /userdata/cfg/bt_reconnect failed!\n");
		return -1;
	}

	if (value)
		ret = write(fd, "bluez-reconnect:enable", strlen("bluez-reconnect:enable"));
	else
		ret = write(fd, "bluez-reconnect:disable", strlen("bluez-reconnect:disable"));

	close(fd);
	return (ret < 0) ? -1 : 0;
}

int rk_bt_start_discovery(unsigned int mseconds, RK_BT_SCAN_TYPE scan_type)
{
	if (!bt_is_open()) {
		pr_info("%s: Please open bt!!!\n", __func__);
		return -1;
	}

	pr_info("%s, tid(%lu)\n", __func__, pthread_self());
	return bt_start_discovery(mseconds, scan_type);
}

int rk_bt_cancel_discovery()
{
	if (!bt_is_open()) {
		pr_info("%s: Please open bt!!!\n", __func__);
		return -1;
	}

	pr_info("%s, tid(%lu)\n", __func__, pthread_self());
	return bt_cancel_discovery(RK_BT_DISC_STOPPED_BY_USER);
}

bool rk_bt_is_discovering()
{
	if (!bt_is_open()) {
		pr_info("%s: Please open bt!!!\n", __func__);
		return false;
	}

	return bt_is_scaning();
}

int rk_bt_get_scaned_devices(RkBtScanedDevice **dev_list, int *count)
{
	*count = 0;
	if (!bt_is_open()) {
		pr_info("%s: Please open bt!!!\n", __func__);
		return -1;
	}

	return bt_get_scaned_devices(dev_list, count, false);
}

int rk_bt_free_scaned_devices(RkBtScanedDevice *dev_list)
{
	if (!bt_is_open()) {
		pr_info("%s: Please open bt!!!\n", __func__);
		return -1;
	}

	return bt_free_scaned_devices(dev_list);
}

void rk_bt_display_devices()
{
	if (!bt_is_open()) {
		pr_info("%s: Please open bt!!!\n", __func__);
		return;
	}

	bt_display_devices();
}

void rk_bt_display_paired_devices()
{
	if (!bt_is_open()) {
		pr_info("%s: Please open bt!!!\n", __func__);
		return;
	}

	bt_display_paired_devices();
}

int rk_bt_get_eir_data(char *address, char *eir_data, int len)
{
	if (!bt_is_open()) {
		pr_err("bt isn't open, please open\n");
		return -1;
	}

	return bt_get_eir_data(address, eir_data, len);
}

int rk_bt_pair_by_addr(char *addr)
{
	if (!bt_is_open()) {
		pr_info("%s: Please open bt!!!\n", __func__);
		return -1;
	}

	return pair_by_addr(addr);
}

int rk_bt_unpair_by_addr(char *addr)
{
	if (!bt_is_open()) {
		pr_info("%s: Please open bt!!!\n", __func__);
		return -1;
	}

	return unpair_by_addr(addr);
}

int rk_bt_set_device_name(char *name)
{
	if (!bt_is_open()) {
		pr_info("%s: Please open bt!!!\n", __func__);
		return -1;
	}

	return bt_set_device_name(name);
}

int rk_bt_get_device_name(char *name, int len)
{
	if (!bt_is_open()) {
		pr_info("%s: Please open bt!!!\n", __func__);
		return -1;
	}

	return bt_get_device_name(name, len);
}

int rk_bt_get_device_addr(char *addr, int len)
{
	if (!bt_is_open()) {
		pr_info("%s: Please open bt!!!\n", __func__);
		return -1;
	}

	return bt_get_device_addr(addr, len);
}

int rk_bt_get_paired_devices(RkBtScanedDevice **dev_list, int *count)
{
	*count = 0;
	if (!bt_is_open()) {
		pr_info("%s: Please open bt!!!\n", __func__);
		return -1;
	}

	return bt_get_scaned_devices(dev_list, count, true);
}

int rk_bt_free_paired_devices(RkBtScanedDevice *dev_list)
{
	if (!bt_is_open()) {
		pr_info("%s: Please open bt!!!\n", __func__);
		return -1;
	}

	return bt_free_scaned_devices(dev_list);
}

RK_BT_PLAYROLE_TYPE rk_bt_get_playrole_by_addr(char *addr)
{
	if (!bt_is_open()) {
		pr_info("%s: Please open bt!!!\n", __func__);
		return -1;
	}

	return bt_get_playrole_by_addr(addr);
}

int rk_bt_set_visibility(const int visiable, const int connectable)
{
	if (visiable && connectable) {
		exec_command_system("hciconfig hci0 piscan");
		return 0;
	}

	exec_command_system("hciconfig hci0 noscan");
	usleep(20000);//20ms
	if (visiable)
		exec_command_system("hciconfig hci0 iscan");
	if (connectable)
		exec_command_system("hciconfig hci0 pscan");

	return 0;
}

void rk_bt_set_bsa_server_path(char *path)
{
	pr_info("bluez don't support %s\n", __func__);
}

bool rk_bt_get_connected_properties(char *addr)
{
	if (!bt_is_open()) {
		pr_info("%s: Please open bt!!!\n", __func__);
		return false;
	}

	return get_device_connected_properties(addr);
}

int rk_bt_read_remote_device_name(char *addr, int transport)
{
	pr_info("bluez don't support %s\n", __func__);
	return -1;
}

RK_BT_DEV_PLATFORM_TYPE rk_bt_get_dev_platform(char *addr)
{
	if (!bt_is_open()) {
		pr_info("%s: Please open bt!!!\n", __func__);
		return -1;
	}

	return (RK_BT_DEV_PLATFORM_TYPE)get_dev_platform(addr);
}

/*****************************************************************
 *            Rockchip bluetooth hfp-hf api                      *
 *****************************************************************/
static int g_ba_hfp_client = -1;

void rk_bt_hfp_register_callback(RK_BT_HFP_CALLBACK cb)
{
	rfcomm_hfp_hf_regist_cb(cb);
}

int rk_bt_hfp_open()
{
	if (!bt_is_open()) {
		pr_info("%s: Please open bt!!!\n", __func__);
		return -1;
	}

	if (bt_hfp_is_open()) {
		pr_err("%s: bt hfp has already been opened!!!\n", __func__);
		return -1;
	}

	if(bt_source_is_open()) {
		pr_info("%s: bt source has been opened, close bt source", __func__);
		rk_bt_source_close();
	}

	if (bt_interface(BT_HFP_OPEN, NULL) < 0) {
		bt_control.is_hfp_open = false;
		return -1;
	}

	g_ba_hfp_client = bluealsa_open("hci0");
	if (g_ba_hfp_client < 0) {
		pr_err("%s: bt hfp connect to bluealsa server failed!", __func__);
		return -1;
	}

	rfcomm_listen_ba_msg_start();
	bt_control.is_hfp_open = true;

	reconn_last_devices(BT_DEVICES_HFP);

	return 0;
}

int rk_bt_hfp_sink_open(void)
{
	if (!bt_is_open()) {
		pr_info("%s: Please open bt!!!\n", __func__);
		return -1;
	}

	/* Already in sink mode or hfp mode? */
	if (bt_sink_is_open() || bt_hfp_is_open()) {
		pr_err("\"rk_bt_sink_open\" or \"rk_bt_hfp_open\" is called before calling this interface."
			"This situation is not allowed.");
		return -1;
	}

	if(bt_source_is_open()) {
		pr_info("%s: bt source has been opened, close bt source", __func__);
		rk_bt_source_close();
	}

	if (bt_interface(BT_HFP_SINK_OPEN, NULL) < 0) {
		bt_control.is_a2dp_sink_open = false;
		bt_control.is_hfp_open = false;
		return -1;
	}

	g_ba_hfp_client = bluealsa_open("hci0");
	if (g_ba_hfp_client < 0) {
		pr_err("%s: bt hfp connect to bluealsa server failed!", __func__);
		return -1;
	}

	rfcomm_listen_ba_msg_start();

	bt_control.is_a2dp_sink_open = true;
	bt_control.is_hfp_open = true;

	return 0;
}

static int bt_hal_hfp_close(bool disconnect)
{
	rfcomm_listen_ba_msg_stop();
	if (g_ba_hfp_client >= 0) {
		shutdown(g_ba_hfp_client, SHUT_RDWR);
		g_ba_hfp_client = -1;
	}

	if (!bt_hfp_is_open())
		return -1;

	rfcomm_hfp_send_event(RK_BT_HFP_DISCONNECT_EVT, NULL);
	rfcomm_hfp_hf_regist_cb(NULL);
	bt_control.is_hfp_open = false;

	if (bt_sink_is_open())
		return 0;

	if(disconnect) {
		if(!disconnect_current_devices())
			sleep(3);
	}

	exec_command_system("hciconfig hci0 noscan");
	kill_task("bluealsa-aplay");
	kill_task("bluealsa");

	return 0;
}

int rk_bt_hfp_close(void)
{
	return bt_hal_hfp_close(true);
}

static char *build_rfcomm_command(const char *cmd)
{

	static char command[512];
	bool at;

	command[0] = '\0';
	if (!(at = strncmp(cmd, "AT", 2) == 0))
		strcpy(command, "\r\n");

	strcat(command, cmd);
	strcat(command, "\r");
	if (!at)
		strcat(command, "\n");

	return command;
}

static int rk_bt_hfp_hp_send_cmd(char *cmd)
{
	char dev_addr[18] = {0};
	char result_buf[256] = {0};
	int ret = 0;
	bdaddr_t ba_addr;
	char *start = NULL;

	if (g_ba_hfp_client <= 0) {
		pr_err("%s ba hfp client is not valid!", __func__);
		return -1;
	}

	exec_command("hcitool con", result_buf, sizeof(result_buf));
	if ((start = strstr(result_buf, "ACL ")) != NULL) {
		start += 4; /* skip space */
		memcpy(dev_addr, start, 17);
	}

	pr_err("%s send cmd:%s to addr:%s", __func__, cmd, dev_addr);
	ret = str2ba(dev_addr, &ba_addr);
	if (ret) {
		pr_err("%s no valid hfp connection!", __func__);
		return -1;
	}

	ret = bluealsa_send_rfcomm_command(g_ba_hfp_client, ba_addr, build_rfcomm_command(cmd));
	if (ret)
		pr_err("%s ba hfp client cmd:/'%s/' failed!", __func__, cmd);

	return ret;
}

int rk_bt_hfp_pickup(void)
{
	if(rk_bt_hfp_hp_send_cmd("ATA")) {
		pr_info("%s: send ATA cmd error\n", __func__);
		return -1;
	}

	return 0;
}

int rk_bt_hfp_hangup(void)
{
	if(rk_bt_hfp_hp_send_cmd("AT+CHUP")) {
		pr_info("%s: send AT+CHUP cmd error\n", __func__);
		return -1;
	}

	return 0;
}

int rk_bt_hfp_dial_number(char *number)
{
	char buf[256];

	if(number == NULL || (strlen(number) == 0)) {
		pr_err("%s: empty number string\n", __func__);
		return -1;
	}

	if(strlen(number) > 250) {
		pr_err("%s: Invalid phone number(%s)\n", __func__, number);
		return -1;
	}

	memset(buf, 0, 256);
	sprintf(buf, "%s%s%s", "ATD", number, ";");

	return rk_bt_hfp_hp_send_cmd(buf);
}

int rk_bt_hfp_redial(void)
{
	return rk_bt_hfp_hp_send_cmd("AT+BLDN");
}

int rk_bt_hfp_report_battery(int value)
{
	int ret = 0;
	char at_cmd[100] = {0};
	static int done = 0;

	if ((value < 0) || (value > 9)) {
		pr_info("%s: ERROR: Invalid value, should within [0, 9]\n", __func__);
		return -1;
	}

	if (done == 0) {
		ret = rk_bt_hfp_hp_send_cmd("AT+XAPL=ABCD-1234-0100,2");
		done = 1;
	}

	if (ret == 0) {
		sprintf(at_cmd, "AT+IPHONEACCEV=1,1,%d", value);
		ret =  rk_bt_hfp_hp_send_cmd(at_cmd);
	}

	return ret;
}

int rk_bt_hfp_set_volume(int volume)
{
	int ret = 0;
	char at_cmd[100] = {0};

	if (volume > 15)
		volume = 15;
	else if (volume < 0)
		volume = 0;

	sprintf(at_cmd, "AT+VGS=%d", volume);
	ret =  rk_bt_hfp_hp_send_cmd(at_cmd);

	return ret;
}

void rk_bt_hfp_enable_cvsd(void)
{
	//for compile
	pr_info("bluez don't support %s\n", __func__);
}

void rk_bt_hfp_disable_cvsd(void)
{
	//for compile
	pr_info("bluez don't support %s\n", __func__);
}

int rk_bt_hfp_disconnect()
{
	return disconnect_current_devices();
}

/*****************************************************************
 *            Rockchip bluetooth obex api                        *
 *****************************************************************/
static pthread_t g_obex_thread = 0;
int rk_bt_obex_init(char *path)
{
	char buf[100];

	if (!bt_is_open()) {
		pr_info("%s: Please open bt!!!\n", __func__);
		return -1;
	}

	if (get_ps_pid("obexd")) {
		pr_info("%s: obexd has been opened\n", __func__);
		return 0;
	}

	if(!path) {
		pr_info("%s: error, path == NULL\n", __func__);
		return -1;
	}

	/* run obexd */
	memset(buf, 0, 100);
	sprintf(buf, "%s%s%s", "/usr/libexec/bluetooth/obexd -d -n -l -a -r /", path, "/ &");
	pr_info("%s: run obexd(%s)\n", __func__, buf);

	setenv("DBUS_SESSION_BUS_ADDRESS", "unix:path=/var/run/dbus/system_bus_socket", 1);
	usleep(6000);
	if (run_task("obexd", buf)) {
		pr_err("%s: run obexd failed!\n", __func__);
		return -1;
	}

	return 0;
}

int rk_bt_obex_pbap_init()
{
	if (!get_ps_pid("obexd")) {
		pr_info("%s: Please open obex!!!\n", __func__);
		return -1;
	}

	if(g_obex_thread) {
		pr_info("%s: g_obex_thread has been initialized\n", __func__);
		return -1;
	}

	/* Create thread to do connect task. */
	if (pthread_create(&g_obex_thread, NULL, obex_main_thread, NULL)) {
		pr_info("%s: obex_main_thread thread create failed!\n", __func__);
		return -1;
	}

	pthread_setname_np(g_obex_thread, "obex_main_thread");
	return 0;
}

void rk_bt_obex_register_status_cb(RK_BT_OBEX_STATE_CALLBACK cb)
{
	obex_pbap_register_status_cb(cb);
}

int rk_bt_obex_pbap_connect(char *btaddr)
{
	if(!g_obex_thread) {
		pr_err("%s: obex don't inited, please init\n", __func__);
		return -1;
	}

	if (!btaddr || (strlen(btaddr) < 17)) {
		pr_err("%s: Invalid address\n", __func__);
		return -1;
	}

	pr_info("[enter %s]\n", __func__);
	return obex_connect_pbap(btaddr);
}

int rk_bt_obex_pbap_get_vcf(char *dir_name, char *dir_file)
{
	if(!g_obex_thread) {
		pr_err("%s: obex don't inited, please init\n", __func__);
		return -1;
	}

	if (!dir_name || !dir_file) {
		pr_err("%s: Invalid dir_name or dir_file\n", __func__);
		return -1;
	}

	pr_info("[enter %s]\n", __func__);
	return obex_get_pbap_pb(dir_name, dir_file);
}

int rk_bt_obex_pbap_disconnect(char *btaddr)
{
	if(!g_obex_thread) {
		pr_err("%s: obex don't inited, please init\n", __func__);
		return -1;
	}

	pr_info("[enter %s]\n", __func__);
	return obex_disconnect(1, NULL);
}

int rk_bt_obex_pbap_deinit()
{
	if(!g_obex_thread) {
		pr_info("%s: obex has been closed\n", __func__);
		return -1;
	}

	pr_info("[enter %s]\n", __func__);

	obex_quit();

	pthread_join(g_obex_thread, NULL);
	g_obex_thread = 0;

	pr_info("[exit %s]\n", __func__);
	return 0;
}

int rk_bt_obex_deinit()
{
	return kill_task("obexd");
}

/*****************************************************************
 *            Rockchip bluetooth pan api                         *
 *****************************************************************/
void rk_bt_pan_register_event_cb(RK_BT_PAN_EVENT_CALLBACK cb)
{
	pr_info("bluez don't support %s\n", __func__);
}

int rk_bt_pan_open()
{
	pr_info("bluez don't support %s\n", __func__);
	return -1;
}

int rk_bt_pan_close()
{
	pr_info("bluez don't support %s\n", __func__);
	return -1;
}

int rk_bt_pan_connect(char *address)
{
	pr_info("bluez don't support %s\n", __func__);
	return -1;
}

int rk_bt_pan_disconnect(char *address)
{
	pr_info("bluez don't support %s\n", __func__);
	return -1;
}
