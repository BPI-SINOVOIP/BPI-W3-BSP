#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <DeviceIo.h>
#include <RkBtBase.h>
#include <RkBtSink.h>
#include <RkBtHfp.h>
#include "utility.h"
#include "slog.h"

#include <bt_manager_1s2.h>

static btmg_callback_t *g_btmg_cb = NULL;
static bool g_btmg_enable = false;
static btmg_state_t g_bt_state = BTMG_STATE_OFF;

int bt_manager_set_loglevel(btmg_log_level_t log_level)
{
	return 0;
}

/* get the bt_manager printing level*/
btmg_log_level_t bt_manager_get_loglevel(void)
{
	return BTMG_LOG_LEVEL_NONE;
}

void bt_manager_debug_open_syslog(void)
{
}

void bt_manager_debug_close_syslog(void)
{
}

int bt_manager_debug_open_file(const char *path)
{
	return 0;
}

void bt_manager_debug_close_file(void)
{

}

static void btmg_gap_status_cb(RK_BT_STATE state)
{
	switch(state) {
		case RK_BT_STATE_OFF:
			g_bt_state = BTMG_STATE_OFF;
			break;
		case RK_BT_STATE_ON:
			g_bt_state = BTMG_STATE_ON;
			break;
		case RK_BT_STATE_TURNING_ON:
			g_bt_state = BTMG_STATE_TURNING_ON;
			break;
		case RK_BT_STATE_TURNING_OFF:
			g_bt_state = BTMG_STATE_TURNING_OFF;
			break;
		default:
			pr_info("unknown bt state: %d\n", state);
			return;
	}

	if(g_btmg_cb && g_btmg_cb->btmg_gap_cb.gap_status_cb)
		g_btmg_cb->btmg_gap_cb.gap_status_cb(g_bt_state);
}

static void btmg_gap_bond_state_cb(const char *bd_addr, const char *name, RK_BT_BOND_STATE state)
{
	btmg_bond_state_t bond_state;

	switch(state) {
		case RK_BT_BOND_STATE_NONE:
			bond_state = BTMG_BOND_STATE_NONE;
			break;
		case RK_BT_BOND_STATE_BONDING:
			bond_state = BTMG_BOND_STATE_BONDING;
			break;
		case RK_BT_BOND_STATE_BONDED:
			bond_state = BTMG_BOND_STATE_BONDED;
			break;
		default:
			pr_info("unknown bt bond state: %d\n", state);
			return;
	}

	if(g_btmg_cb && g_btmg_cb->btmg_gap_cb.gap_bond_state_cb)
		g_btmg_cb->btmg_gap_cb.gap_bond_state_cb(bond_state, bd_addr, name);
}

static void btmg_gap_discovery_status_cb(RK_BT_DISCOVERY_STATE state)
{
	btmg_discovery_state_t disc_state;

	switch(state) {
		case RK_BT_DISC_STARTED:
			disc_state = BTMG_DISC_STARTED;
			break;
		case RK_BT_DISC_STOPPED_AUTO:
			disc_state = BTMG_DISC_STOPPED_AUTO;
			break;
		case RK_BT_DISC_START_FAILED:
			disc_state = BTMG_DISC_START_FAILED;
			break;
		case RK_BT_DISC_STOPPED_BY_USER:
			disc_state = BTMG_DISC_STOPPED_BY_USER;
			break;
		default:
			pr_info("unknown bt discovery state: %d\n", state);
			return;
	}

	if(g_btmg_cb && g_btmg_cb->btmg_gap_cb.gap_disc_status_cb)
		g_btmg_cb->btmg_gap_cb.gap_disc_status_cb(disc_state);
}

static void btmg_dev_found_cb(const char *address,const char *name, unsigned int bt_class, int rssi)
{
	if(g_btmg_cb && g_btmg_cb->btmg_gap_cb.gap_dev_found_cb)
		g_btmg_cb->btmg_gap_cb.gap_dev_found_cb(address, name, bt_class, rssi);
}

static void _btmg_sink_conn_state_cb(const char *bd_addr, btmg_a2dp_sink_connection_state_t state)
{
	if(g_btmg_cb && g_btmg_cb->btmg_a2dp_sink_cb.a2dp_sink_connection_state_cb)
		g_btmg_cb->btmg_a2dp_sink_cb.a2dp_sink_connection_state_cb(bd_addr, state);
}

static void _btmg_avrcp_play_state_cb(const char *bd_addr, btmg_avrcp_play_state_t state)
{
	if(g_btmg_cb && g_btmg_cb->btmg_avrcp_cb.avrcp_play_state_cb)
		g_btmg_cb->btmg_avrcp_cb.avrcp_play_state_cb(bd_addr, state);
}

static void _btmg_sink_audio_state_cb(const char *bd_addr, btmg_a2dp_sink_audio_state_t state)
{
	if(g_btmg_cb && g_btmg_cb->btmg_a2dp_sink_cb.a2dp_sink_audio_state_cb)
		g_btmg_cb->btmg_a2dp_sink_cb.a2dp_sink_audio_state_cb(bd_addr, state);
}

static void btmg_sink_audio_underrun_cb(void)
{
	if(g_btmg_cb && g_btmg_cb->btmg_a2dp_sink_cb.a2dp_sink_audio_underrun_cb)
		g_btmg_cb->btmg_a2dp_sink_cb.a2dp_sink_audio_underrun_cb();
}

static int btmg_sink_callback(RK_BT_SINK_STATE state)
{
	char bd_addr[18];
	memset(bd_addr, 0, 18);
	rk_bt_sink_get_default_dev_addr(bd_addr, 18);

	switch(state) {
		case RK_BT_SINK_STATE_IDLE:
			break;
		case RK_BT_SINK_STATE_CONNECT:
			_btmg_sink_conn_state_cb(bd_addr, BTMG_A2DP_SINK_CONNECTED);
			break;
		case RK_BT_SINK_STATE_DISCONNECT:
			_btmg_sink_conn_state_cb(bd_addr, BTMG_A2DP_SINK_DISCONNECTED);
			break;
		//avrcp
		case RK_BT_SINK_STATE_PLAY:
			_btmg_avrcp_play_state_cb(bd_addr, BTMG_AVRCP_PLAYSTATE_PLAYING);
			break;
		case RK_BT_SINK_STATE_PAUSE:
			_btmg_avrcp_play_state_cb(bd_addr, BTMG_AVRCP_PLAYSTATE_PAUSED);
			break;
		case RK_BT_SINK_STATE_STOP:
			_btmg_avrcp_play_state_cb(bd_addr, BTMG_AVRCP_PLAYSTATE_STOPPED);
			break;
		//avdtp(a2dp)
		case RK_BT_A2DP_SINK_STARTED:
			_btmg_sink_audio_state_cb(bd_addr, BTMG_A2DP_SINK_AUDIO_STARTED);
			break;
		case RK_BT_A2DP_SINK_SUSPENDED:
			_btmg_sink_audio_state_cb(bd_addr, BTMG_A2DP_SINK_AUDIO_SUSPENDED);
			break;
		case RK_BT_A2DP_SINK_STOPPED:
			_btmg_sink_audio_state_cb(bd_addr, BTMG_A2DP_SINK_AUDIO_STOPPED);
			break;
	}

	return 0;
}

static void btmg_sink_track_change_callback(const char *bd_addr, BtTrackInfo track_info)
{
	if(g_btmg_cb && g_btmg_cb->btmg_avrcp_cb.avrcp_track_changed_cb)
		g_btmg_cb->btmg_avrcp_cb.avrcp_track_changed_cb(bd_addr, track_info);
}

static void btmg_sink_position_change_callback(const char *bd_addr, int song_len, int song_pos)
{
	if(g_btmg_cb && g_btmg_cb->btmg_avrcp_cb.avrcp_play_position_cb)
		g_btmg_cb->btmg_avrcp_cb.avrcp_play_position_cb(bd_addr, song_len, song_pos);
}

/*preinit function, to allocate room for callback struct, which will be free by bt_manager_deinit*/
int bt_manager_preinit(btmg_callback_t **btmg_cb)
{
	*btmg_cb = (btmg_callback_t *)malloc(sizeof(btmg_callback_t));
	if(*btmg_cb == NULL) {
		pr_info("malloc bt manager callback failed\n");
		return -1;
	}

	memset(*btmg_cb, 0, sizeof(btmg_callback_t));
	return 0;
}

/*init function, the callback functions will be registered*/
int bt_manager_init(btmg_callback_t *btmg_cb)
{
	g_btmg_cb = btmg_cb;
	return 0;
}

/*deinit function, must be called before exit*/
int bt_manager_deinit(btmg_callback_t *btmg_cb)
{
	if(bt_manager_is_enabled())
		bt_manager_enable(false);

	if(btmg_cb)
		free(btmg_cb);

	g_btmg_cb = NULL;
	return 0;
}

/*enable BT*/
int bt_manager_enable(bool enable)
{
	int ret;
	if(enable) {
		rk_bt_register_state_callback(btmg_gap_status_cb);
		rk_bt_register_bond_callback(btmg_gap_bond_state_cb);
		rk_bt_register_discovery_callback(btmg_gap_discovery_status_cb);
		rk_bt_register_dev_found_callback(btmg_dev_found_cb);
		if(rk_bt_init(NULL) < 0) {
			pr_info("%s: rk_bt_init error\n", __func__);
			return -1;
		}

		rk_bt_sink_register_track_callback(btmg_sink_track_change_callback);
		rk_bt_sink_register_position_callback(btmg_sink_position_change_callback);
		rk_bt_sink_register_callback(btmg_sink_callback);
		rk_bt_sink_register_underurn_callback(btmg_sink_audio_underrun_cb);
		ret = rk_bt_sink_open();
	} else {
		rk_bt_deinit();
		ret = 0;
	}

	g_btmg_enable = enable;
	return ret;
}

/*return BT state, is enabled or not*/
bool bt_manager_is_enabled(void)
{
	return g_btmg_enable;
}

/*GAP APIs*/
/*start discovery, will return immediately*/
int bt_manager_start_discovery(unsigned int mseconds)
{
	return rk_bt_start_discovery(mseconds, SCAN_TYPE_AUTO);
}

/*cancel discovery, will return immediately*/
int bt_manager_cancel_discovery(void)
{
	return rk_bt_cancel_discovery();
}

/*judge the discovery is in process or not*/
bool bt_manager_is_discovering()
{
	return rk_bt_is_discovering();
}

/*set BT discovery mode*/
int bt_manager_set_discovery_mode(btmg_discovery_mode_t mode)
{
	int ret = -1;
	switch (mode) {
		case BTMG_SCAN_MODE_NONE:
			ret = rk_bt_set_visibility(0, 0);
			break;
		case BTMG_SCAN_MODE_CONNECTABLE:
			ret = rk_bt_set_visibility(0, 1);
			break;
		case BTMG_SCAN_MODE_CONNECTABLE_DISCOVERABLE:
			ret = rk_bt_set_visibility(1, 1);
			break;
	}

	return ret;
}

/*pair*/
int bt_manager_pair(char *addr)
{
	return rk_bt_pair_by_addr(addr);
}

/*unpair*/
int bt_manager_unpair(char *addr)
{
	return rk_bt_unpair_by_addr(addr);
}

/*get bt state*/
btmg_state_t bt_manager_get_state()
{
	return g_bt_state;
}

/*get BT name*/
int bt_manager_get_name(char *name, int size)
{
	return rk_bt_get_device_name(name, size);
}

/*set BT name*/
int bt_manager_set_name(const char *name)
{
	return rk_bt_set_device_name((char *)name);
}

/*get local device address*/
int bt_manager_get_address(char *addr, int size)
{
	return rk_bt_get_device_addr(addr, size);
}

/*a2dp sink APIs*/
/*request a2dp_sink connection*/
int bt_manager_a2dp_sink_connect(char *addr)
{
	return rk_bt_sink_connect_by_addr(addr);
}

/*request a2dp_sink disconnection*/
int bt_manager_a2dp_sink_disconnect(char *addr)
{
	return rk_bt_sink_disconnect_by_addr(addr);
}

/*used to send avrcp command, refer to the struct btmg_avrcp_command_t for the supported commands*/
int bt_manager_avrcp_command(char *addr, btmg_avrcp_command_t command)
{
	int ret = -1;
	char bd_addr[18];

	memset(bd_addr, 0, 18);
	rk_bt_sink_get_default_dev_addr(bd_addr, 18);
	if(strncmp(bd_addr, addr, 17)) {
		pr_info("%s: Invalid address(%s)\n", __func__, addr);
		return -1;
	}

	switch(command) {
		case BTMG_AVRCP_PLAY:
			ret = rk_bt_sink_play();
			break;
		case BTMG_AVRCP_STOP:
			ret = rk_bt_sink_stop();
			break;
		case BTMG_AVRCP_PAUSE:
			ret = rk_bt_sink_pause();
			break;
		case BTMG_AVRCP_FORWARD:
			ret = rk_bt_sink_next();
			break;
		case BTMG_AVRCP_BACKWARD:
			ret = rk_bt_sink_prev();
			break;
	}

	return ret;
}

bt_paried_device *bt_create_one_paired_dev(RkBtScanedDevice *scan_dev)
{
	bt_paried_device *new_device = (bt_paried_device*)malloc(sizeof(bt_paried_device));
	if(!new_device) {
		printf("%s: malloc one paierded device failed\n", __func__);
		return NULL;
	}

	new_device->remote_address = (char *)malloc(strlen(scan_dev->remote_address) + 1);
	strncpy(new_device->remote_address, scan_dev->remote_address, strlen(scan_dev->remote_address));
	new_device->remote_address[strlen(scan_dev->remote_address)] = '\0';

	new_device->remote_name = (char *)malloc(strlen(scan_dev->remote_name) + 1);
	strncpy(new_device->remote_name, scan_dev->remote_name, strlen(scan_dev->remote_name));
	new_device->remote_name[strlen(scan_dev->remote_name)] = '\0';

	new_device->is_connected = scan_dev->is_connected;
	new_device->next = NULL;

	return new_device;
}

/* Get the paired device,need to call <bt_manager_free_paired_devices> to free data*/
int bt_manager_get_paired_devices(bt_paried_device **dev_list,int *count)
{
	int i;
	RkBtScanedDevice *dev_tmp = NULL;
	RkBtScanedDevice *scan_dev_list;

	if(dev_list == NULL) {
		pr_info("%s: invalid dev_list\n", __func__);
		return -1;
	}

	if(rk_bt_get_paired_devices(&scan_dev_list, count) < 0)
		return -1;

	//printf("%s: current paired devices count: %d\n", __func__, *count);
	dev_tmp = scan_dev_list;
	for(i = 0; i < *count; i++) {
		//printf("device %d\n", i);
		//printf("	remote_address: %s\n", dev_tmp->remote_address);
		//printf("	remote_name: %s\n", dev_tmp->remote_name);
		//printf("	is_connected: %d\n", dev_tmp->is_connected);
		//printf("	cod: %d\n", dev_tmp->cod);

		if(*dev_list == NULL) {
			*dev_list = bt_create_one_paired_dev(dev_tmp);
			if(*dev_list == NULL)
				return -1;
		} else {
			bt_paried_device *cur_dev = *dev_list;
			while(cur_dev->next != NULL)
				cur_dev = cur_dev->next;

			bt_paried_device *new_dev = bt_create_one_paired_dev(dev_tmp);
			if(!new_dev)
				return -1;

			cur_dev->next = new_dev;
		}

		dev_tmp = dev_tmp->next;
	}

	rk_bt_free_paired_devices(scan_dev_list);
	return 0;
}

/* free paird device data resource*/
int bt_manager_free_paired_devices(bt_paried_device *dev_list)
{
	bt_paried_device *dev_tmp = NULL;

	if(dev_list == NULL) {
		printf("%s: dev_list is nill, don't need to clear\n", __func__);
		return -1;
	}

	while(dev_list->next != NULL) {
		printf("%s: free dev: %s\n", __func__, dev_list->remote_address);
		dev_tmp = dev_list->next;
		free(dev_list->remote_address);
		free(dev_list->remote_name);
		free(dev_list);
		dev_list = dev_tmp;
	}

	if(dev_list != NULL) {
		printf("%s: last free dev: %s\n", __func__, dev_list->remote_address);
		free(dev_list->remote_address);
		free(dev_list->remote_name);
		free(dev_list);
		dev_list = NULL;
	}

	return 0;
}

int bt_manager_disconnect(char *addr)
{
	return 0;
}

/*send GetPlayStatus cmd*/
int bt_manager_send_get_play_status()
{
	return rk_bt_sink_get_play_status();
}

/*if support avrcp EVENT_PLAYBACK_POS_CHANGED,*/
bool bt_manager_is_support_pos_changed()
{
	return rk_bt_sink_get_poschange();
}

int bt_manager_switch_throughput(bool sw_to_wlan)
{
	if (sw_to_wlan)
		return exec_command_system("hcitool cmd 0x3f 0xa7 0x01");
	else
		return exec_command_system("hcitool cmd 0x3f 0xa7 0x00");
}