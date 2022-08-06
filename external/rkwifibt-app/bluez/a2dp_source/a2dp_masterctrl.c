#include <stdio.h>
#include <errno.h>
#include <glib.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <stdbool.h>
#include <wordexp.h>
#include <sys/un.h>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <linux/input.h>
#include <dirent.h>
#include <fcntl.h>
#include <pthread.h>

#include "DeviceIo.h"
//#include "Rk_shell.h"

#include "a2dp_masterctrl.h"
#include "shell.h"
#include "util.h"
#include "agent.h"
#include "gatt.h"
#include "advertising.h"
#include "../bluez_ctrl.h"
#include "../gatt_config.h"
#include "../gatt_client.h"
#include "../bluez_ctrl.h"
#include "utility.h"
#include "slog.h"

/* operands in passthrough commands */
#define AVC_VOLUME_UP        0x41
#define AVC_VOLUME_DOWN      0x42
#define AVC_PLAY             0x44
#define AVC_STOP             0x45
#define AVC_PAUSE            0x46
#define AVC_FORWARD          0x4b
#define AVC_BACKWARD         0x4c

/* String display constants */
#define COLORED_NEW COLOR_GREEN "NEW" COLOR_OFF
#define COLORED_CHG COLOR_YELLOW "CHG" COLOR_OFF
#define COLORED_DEL COLOR_RED "DEL" COLOR_OFF

#define PROMPT_ON   COLOR_BLUE "[bluetooth]" COLOR_OFF "# "
#define PROMPT_OFF  "Waiting to connect to bluetoothd..."

#define DISTANCE_VAL_INVALID    0x7FFF

DBusConnection *dbus_conn = NULL;
static GDBusProxy *agent_manager;
static char *auto_register_agent = NULL;
static RkBtContent *g_bt_content = NULL;

struct remote_sink_device {
	int state;
	char addr[64];
	char name[64];
};
struct remote_sink_device rsd;

struct adapter {
	GDBusProxy *proxy;
	GDBusProxy *ad_proxy;
	GList *devices;
};

typedef struct {
	RK_BT_STATE_CALLBACK bt_state_cb;
	RK_BT_BOND_CALLBACK bt_bond_state_cb;
	RK_BT_DISCOVERY_CALLBACK bt_decovery_cb;
	RK_BT_DEV_FOUND_CALLBACK bt_dev_found_cb;
	RK_BT_SOURCE_CALLBACK bt_source_event_cb;
	RK_BLE_STATE_CALLBACK ble_state_cb;
	RK_BT_NAME_CHANGE_CALLBACK bt_name_change_cb;
	RK_BT_MTU_CALLBACK ble_mtu_cb;
} bt_callback_t;

typedef struct {
	bool is_scaning;
	bool scan_off_failed;
	pthread_t scan_thread;
	unsigned int scan_time;
	RK_BT_SCAN_TYPE scan_type;
} bt_scan_info_t;

typedef struct {
	bool is_connecting;
	bool is_reconnected;
	char connect_address[18];
	char reconnect_address[18];
} bt_source_info_t;

//workround unknow err for con/discon remote proxy
struct ConnectContext_t {
    GDBusProxy *proxy;
    char *address;
};

struct adapter *default_ctrl = NULL;
GDBusProxy *default_dev = NULL;
GDBusProxy *ble_dev = NULL;
GDBusProxy *default_attr = NULL;
GList *ctrl_list;

GDBusClient *btsrc_client = NULL;

/* For connect cmd */
#define BTSRC_CONNECT_IDLE   0
#define BTSRC_CONNECT_DOING  1
#define BTSRC_CONNECT_SUCESS 2
#define BTSRC_CONNECT_FAILED 3

static bool BT_OPENED = 0;
static bool BT_CLOSED = 0;

static void *g_btmaster_userdata = NULL;
static RK_BLE_STATE g_ble_state;
static RK_BT_SOURCE_EVENT g_device_state = BT_SOURCE_EVENT_DISCONNECTED;

static bt_source_info_t g_bt_source_info = {
	false, false,
};

static bt_callback_t g_bt_callback = {
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
};

static bt_scan_info_t g_bt_scan_info = {
	false, false, 0, 0, SCAN_TYPE_AUTO,
};

#ifdef __cplusplus
extern "C" {
#endif

extern void register_app(GDBusProxy *proxy);
extern void unregister_app(GDBusProxy *proxy);
int gatt_set_on_adv(void);
static void filter_clear_transport();
static int remove_device(GDBusProxy *proxy);

#ifdef __cplusplus
}
#endif

extern void a2dp_sink_proxy_removed(GDBusProxy *proxy, void *user_data);
extern void a2dp_sink_proxy_added(GDBusProxy *proxy, void *user_data);
extern void a2dp_sink_property_changed(GDBusProxy *proxy, const char *name, DBusMessageIter *iter, void *user_data);
extern void adapter_changed(GDBusProxy *proxy, DBusMessageIter *iter, void *user_data);
extern void device_changed(GDBusProxy *proxy, DBusMessageIter *iter, void *user_data);
extern int init_avrcp_ctrl(void);

static volatile int ble_service_cnt = 0;

extern void report_avrcp_event(enum BtEvent event, void *data, int len);

static const char *agent_arguments[] = {
	"on",
	"off",
	"DisplayOnly",
	"DisplayYesNo",
	"KeyboardDisplay",
	"KeyboardOnly",
	"NoInputNoOutput",
	NULL
};

static const char *ad_arguments[] = {
	"on",
	"off",
	"peripheral",
	"broadcast",
	NULL
};

#define BT_RECONNECT_CFG "/data/cfg/lib/bluetooth/reconnect_cfg"

int a2dp_master_save_status(char *address);
static int load_last_device(char *address);
static void save_last_device(GDBusProxy *proxy);

static void bt_bond_state_send(const char *bd_addr, const char *name, RK_BT_BOND_STATE state)
{
	if(g_bt_callback.bt_bond_state_cb)
		g_bt_callback.bt_bond_state_cb(bd_addr, name, state);
}

void bt_register_bond_callback(RK_BT_BOND_CALLBACK cb)
{
	g_bt_callback.bt_bond_state_cb = cb;
}

void bt_deregister_bond_callback()
{
	g_bt_callback.bt_bond_state_cb = NULL;
}

void bt_state_send(RK_BT_STATE state)
{
	if(g_bt_callback.bt_state_cb)
		g_bt_callback.bt_state_cb(state);
}

void bt_register_state_callback(RK_BT_STATE_CALLBACK cb)
{
	g_bt_callback.bt_state_cb = cb;
}

void bt_deregister_state_callback()
{
	g_bt_callback.bt_state_cb = NULL;
}

void ble_state_send(RK_BLE_STATE status)
{
	char addr[18], name[256];

	g_ble_state = status;

	if(!g_bt_callback.ble_state_cb) {
		pr_info("%s: ble_state_cb are not registered\n", __func__);
		return;
	}

	if(!ble_is_open()) {
		pr_info("%s: ble is close\n", __func__);
		return;
	}

	memset(addr, 0, 18);
	memset(name, 0, 256);
	bt_get_device_addr_by_proxy(ble_dev, addr, 18);
	bt_get_device_name_by_proxy(ble_dev, name, 256);
	g_bt_callback.ble_state_cb(addr, name, status);
}

void ble_get_state(RK_BLE_STATE *p_state)
{
	if (!p_state)
		return;

	*p_state = g_ble_state;
}

void ble_register_state_callback(RK_BLE_STATE_CALLBACK cb)
{
	g_bt_callback.ble_state_cb = cb;
}

void ble_deregister_state_callback()
{
	g_bt_callback.ble_state_cb = NULL;
}

static void bt_discovery_state_send(RK_BT_DISCOVERY_STATE state)
{
	if(g_bt_callback.bt_decovery_cb) {
		pr_info("%s: state = %d\n", __func__, state);
		g_bt_callback.bt_decovery_cb(state);
	}
}

void bt_register_discovery_callback(RK_BT_DISCOVERY_CALLBACK cb)
{
	g_bt_callback.bt_decovery_cb = cb;
}

void bt_deregister_discovery_callback()
{
	g_bt_callback.bt_decovery_cb = NULL;
}

void dev_found_send(GDBusProxy *proxy, RK_BT_DEV_FOUND_CALLBACK cb)
{
	DBusMessageIter iter;
	dbus_uint32_t bt_class = 0;
	const char *address, *name;
	short rssi = DISTANCE_VAL_INVALID;

	if(!cb) {
		pr_info("%s: cb == NULL\n", __func__);
		return;
	}

	if (g_dbus_proxy_get_property(proxy, "Address", &iter) == FALSE) {
		pr_info("%s: get Address failed\n", __func__);
		return;
	}

	dbus_message_iter_get_basic(&iter, &address);

	if (g_dbus_proxy_get_property(proxy, "Alias", &iter))
		dbus_message_iter_get_basic(&iter, &name);
	else
		name = "unknown";

	if(g_dbus_proxy_get_property(proxy, "Class", &iter))
		dbus_message_iter_get_basic(&iter, &bt_class);

	if (g_dbus_proxy_get_property(proxy, "RSSI", &iter))
		dbus_message_iter_get_basic(&iter, &rssi);

	cb(address, name, bt_class, rssi);
}

static void bt_dev_found_send(GDBusProxy *proxy)
{
	if(!bt_is_scaning() || !g_bt_callback.bt_dev_found_cb)
		return;

	dev_found_send(proxy, g_bt_callback.bt_dev_found_cb);
}

void bt_register_dev_found_callback(RK_BT_DEV_FOUND_CALLBACK cb)
{
	g_bt_callback.bt_dev_found_cb = cb;
}

void bt_deregister_dev_found_callback()
{
	g_bt_callback.bt_dev_found_cb = NULL;
}

static void bt_name_change_send(GDBusProxy *proxy)
{
	const char *address, *name;
	DBusMessageIter iter;

	if(!g_bt_callback.bt_name_change_cb)
		return;

	if (g_dbus_proxy_get_property(proxy, "Address", &iter) == FALSE) {
		pr_info("%s: get Address failed\n", __func__);
		return;
	}

	dbus_message_iter_get_basic(&iter, &address);

	if (g_dbus_proxy_get_property(proxy, "Alias", &iter))
		dbus_message_iter_get_basic(&iter, &name);
	else
		name = address;

	g_bt_callback.bt_name_change_cb(address, name);
}

static void ble_mtu_exchange_send(GDBusProxy *proxy)
{
	const char *address;
	dbus_uint16_t mtu;
	DBusMessageIter iter;

	if(!ble_is_open() && !ble_client_is_open())
		return;

	if(!g_bt_callback.ble_mtu_cb)
		return;

	if (!g_dbus_proxy_get_property(proxy, "Address", &iter)) {
		pr_info("%s: get Address failed\n", __func__);
		return;
	}
	dbus_message_iter_get_basic(&iter, &address);

	if (!g_dbus_proxy_get_property(proxy, "MTU", &iter)) {
		pr_info("%s: get MTU failed\n", __func__);
		return;
	}
	dbus_message_iter_get_basic(&iter, &mtu);

	if(mtu > BT_ATT_MAX_LE_MTU || mtu < BT_ATT_DEFAULT_LE_MTU)
		pr_err("%s: MTU exchange error(%d)\n", __func__, mtu);

	g_bt_callback.ble_mtu_cb(address, mtu);
}

void bt_register_name_change_callback(RK_BT_NAME_CHANGE_CALLBACK cb)
{
	g_bt_callback.bt_name_change_cb = cb;
}

void bt_deregister_name_change_callback()
{
	g_bt_callback.bt_name_change_cb = NULL;
}

void ble_register_mtu_callback(RK_BT_MTU_CALLBACK cb)
{
	g_bt_callback.ble_mtu_cb = cb;
}

void ble_deregister_mtu_callback()
{
	g_bt_callback.ble_mtu_cb = NULL;
}

static void proxy_leak(gpointer data)
{
	pr_info("Leaking proxy %p\n", data);
}

static void connect_handler(DBusConnection *connection, void *user_data)
{
	bt_shell_set_prompt(PROMPT_ON);
}

static void disconnect_handler(DBusConnection *connection, void *user_data)
{
	//bt_shell_detach();

	//bt_shell_set_prompt(PROMPT_OFF);

	g_list_free_full(ctrl_list, proxy_leak);
	ctrl_list = NULL;

	default_ctrl = NULL;
}

static void print_adapter(GDBusProxy *proxy, const char *description)
{
	DBusMessageIter iter;
	const char *address, *name;

	if (g_dbus_proxy_get_property(proxy, "Address", &iter) == FALSE)
		return;

	dbus_message_iter_get_basic(&iter, &address);

	if (g_dbus_proxy_get_property(proxy, "Alias", &iter) == TRUE)
		dbus_message_iter_get_basic(&iter, &name);
	else
		name = "<unknown>";

	pr_info("%s%s%sController %s %s %s\n",
				description ? "[" : "",
				description ? : "",
				description ? "] " : "",
				address, name,
				default_ctrl &&
				default_ctrl->proxy == proxy ?
				"[default]" : "");

}

static void btsrc_scan_save_device(GDBusProxy *proxy, BtScanParam *param)
{
	DBusMessageIter iter;
	const char *address, *name;
	BtDeviceInfo *device_info = NULL;
	size_t cplen = 0;

	if (g_dbus_proxy_get_property(proxy, "Address", &iter) == FALSE)
		return;

	dbus_message_iter_get_basic(&iter, &address);

	if (g_dbus_proxy_get_property(proxy, "Alias", &iter) == TRUE)
		dbus_message_iter_get_basic(&iter, &name);
	else
		name = "<unknown>";

	if (param && (param->item_cnt < BT_SOURCE_SCAN_DEVICES_CNT)) {
		device_info = &(param->devices[param->item_cnt]);
		memset(device_info, 0, sizeof(BtDeviceInfo));
		cplen = sizeof(device_info->name);
		cplen = (strlen(name) > cplen) ? cplen : strlen(name);
		memcpy(device_info->name, name, cplen);
		memcpy(device_info->address, address, sizeof(device_info->address));
		param->item_cnt++;
	}
}

static void print_device(GDBusProxy *proxy, const char *description)
{
	DBusMessageIter iter;
	const char *address, *name;

	if (g_dbus_proxy_get_property(proxy, "Address", &iter) == FALSE) {
		pr_info("%s: can't get Address\n", __func__);
		return;
	}

	dbus_message_iter_get_basic(&iter, &address);

	if (g_dbus_proxy_get_property(proxy, "Alias", &iter) == TRUE)
		dbus_message_iter_get_basic(&iter, &name);
	else
		name = "<unknown>";

	printf("%s%s%sDevice %s %s\n",
				description ? "[" : "",
				description ? : "",
				description ? "] " : "",
				address, name);
}

void print_fixed_iter(const char *label, const char *name,
						DBusMessageIter *iter)
{
	dbus_bool_t *valbool;
	dbus_uint32_t *valu32;
	dbus_uint16_t *valu16;
	dbus_int16_t *vals16;
	unsigned char *byte;
	int len;

	switch (dbus_message_iter_get_arg_type(iter)) {
	case DBUS_TYPE_BOOLEAN:
		dbus_message_iter_get_fixed_array(iter, &valbool, &len);

		if (len <= 0)
			return;

		pr_info("%s%s:\n", label, name);
		bt_shell_hexdump((void *)valbool, len * sizeof(*valbool));

		break;
	case DBUS_TYPE_UINT32:
		dbus_message_iter_get_fixed_array(iter, &valu32, &len);

		if (len <= 0)
			return;

		pr_info("%s%s:\n", label, name);
		bt_shell_hexdump((void *)valu32, len * sizeof(*valu32));

		break;
	case DBUS_TYPE_UINT16:
		dbus_message_iter_get_fixed_array(iter, &valu16, &len);

		if (len <= 0)
			return;

		pr_info("%s%s:\n", label, name);
		bt_shell_hexdump((void *)valu16, len * sizeof(*valu16));

		break;
	case DBUS_TYPE_INT16:
		dbus_message_iter_get_fixed_array(iter, &vals16, &len);

		if (len <= 0)
			return;

		pr_info("%s%s:\n", label, name);
		bt_shell_hexdump((void *)vals16, len * sizeof(*vals16));

		break;
	case DBUS_TYPE_BYTE:
		dbus_message_iter_get_fixed_array(iter, &byte, &len);

		if (len <= 0)
			return;

		pr_info("%s%s:\n", label, name);
		bt_shell_hexdump((void *)byte, len * sizeof(*byte));

		break;
	default:
		return;
	};
}

void print_iter(const char *label, const char *name,
						DBusMessageIter *iter)
{
	dbus_bool_t valbool;
	dbus_uint32_t valu32;
	dbus_uint16_t valu16;
	dbus_int16_t vals16;
	unsigned char byte;
	const char *valstr;
	DBusMessageIter subiter;
	char *entry;

	if (iter == NULL) {
		pr_info("%s%s is nil\n", label, name);
		return;
	}

	switch (dbus_message_iter_get_arg_type(iter)) {
	case DBUS_TYPE_INVALID:
		pr_info("%s%s is invalid\n", label, name);
		break;
	case DBUS_TYPE_STRING:
	case DBUS_TYPE_OBJECT_PATH:
		dbus_message_iter_get_basic(iter, &valstr);
		pr_info("%s%s: %s\n", label, name, valstr);
		if (!strncmp(name, "Status", 6)) {
			if (strstr(valstr, "playing"))
				report_avrcp_event(BT_EVENT_START_PLAY, NULL, 0);
			else if (strstr(valstr, "paused"))
				report_avrcp_event(BT_EVENT_PAUSE_PLAY, NULL, 0);
			else if (strstr(valstr, "stopped"))
				report_avrcp_event(BT_EVENT_STOP_PLAY, NULL, 0);
		}
		break;
	case DBUS_TYPE_BOOLEAN:
		dbus_message_iter_get_basic(iter, &valbool);
		pr_info("%s%s: %s\n", label, name,
					valbool == TRUE ? "yes" : "no");
		break;
	case DBUS_TYPE_UINT32:
		dbus_message_iter_get_basic(iter, &valu32);
		pr_info("%s%s: 0x%08x\n", label, name, valu32);
		break;
	case DBUS_TYPE_UINT16:
		dbus_message_iter_get_basic(iter, &valu16);
		pr_info("%s%s: 0x%04x\n", label, name, valu16);
		break;
	case DBUS_TYPE_INT16:
		dbus_message_iter_get_basic(iter, &vals16);
		pr_info("%s%s: %d\n", label, name, vals16);
		break;
	case DBUS_TYPE_BYTE:
		dbus_message_iter_get_basic(iter, &byte);
		pr_info("%s%s: 0x%02x\n", label, name, byte);
		break;
	case DBUS_TYPE_VARIANT:
		dbus_message_iter_recurse(iter, &subiter);
		print_iter(label, name, &subiter);
		break;
	case DBUS_TYPE_ARRAY:
		dbus_message_iter_recurse(iter, &subiter);

		if (dbus_type_is_fixed(
				dbus_message_iter_get_arg_type(&subiter))) {
			print_fixed_iter(label, name, &subiter);
			break;
		}

		while (dbus_message_iter_get_arg_type(&subiter) !=
							DBUS_TYPE_INVALID) {
			print_iter(label, name, &subiter);
			dbus_message_iter_next(&subiter);
		}
		break;
	case DBUS_TYPE_DICT_ENTRY:
		dbus_message_iter_recurse(iter, &subiter);
		entry = g_strconcat(name, " Key", NULL);
		print_iter(label, entry, &subiter);
		g_free(entry);

		entry = g_strconcat(name, " Value", NULL);
		dbus_message_iter_next(&subiter);
		print_iter(label, entry, &subiter);
		g_free(entry);
		break;
	default:
		pr_info("%s%s has unsupported type\n", label, name);
		break;
	}
}

void print_property(GDBusProxy *proxy, const char *name)
{
	DBusMessageIter iter;

	if (g_dbus_proxy_get_property(proxy, name, &iter) == FALSE)
		return;

	print_iter("\t", name, &iter);
}

static void print_uuid(const char *uuid)
{
	const char *text;

	text = bt_uuidstr_to_str(uuid);
	if (text) {
		char str[26];
		unsigned int n;

		str[sizeof(str) - 1] = '\0';

		n = snprintf(str, sizeof(str), "%s", text);
		if (n > sizeof(str) - 1) {
			str[sizeof(str) - 2] = '.';
			str[sizeof(str) - 3] = '.';
			if (str[sizeof(str) - 4] == ' ')
				str[sizeof(str) - 4] = '.';

			n = sizeof(str) - 1;
		}

		pr_info("\tUUID: %s%*c(%s)\n", str, 26 - n, ' ', uuid);
	} else
		pr_info("\tUUID: %*c(%s)\n", 26, ' ', uuid);
}

static void print_uuids(GDBusProxy *proxy)
{
	DBusMessageIter iter, value;

	if (g_dbus_proxy_get_property(proxy, "UUIDs", &iter) == FALSE)
		return;

	dbus_message_iter_recurse(&iter, &value);

	while (dbus_message_iter_get_arg_type(&value) == DBUS_TYPE_STRING) {
		const char *uuid;

		dbus_message_iter_get_basic(&value, &uuid);

		print_uuid(uuid);

		dbus_message_iter_next(&value);
	}
}

static gboolean device_is_child(GDBusProxy *device, GDBusProxy *master)
{
	DBusMessageIter iter;
	const char *adapter, *path;

	if (!master)
		return FALSE;

	if (g_dbus_proxy_get_property(device, "Adapter", &iter) == FALSE)
		return FALSE;

	dbus_message_iter_get_basic(&iter, &adapter);
	path = g_dbus_proxy_get_path(master);

	if (!strcmp(path, adapter))
		return TRUE;

	return FALSE;
}

static GDBusProxy * service_is_child(GDBusProxy *service)
{
	DBusMessageIter iter;
	const char *device;

	if (g_dbus_proxy_get_property(service, "Device", &iter) == FALSE)
		return NULL;

	dbus_message_iter_get_basic(&iter, &device);

	if (!default_ctrl)
		return NULL;

	return g_dbus_proxy_lookup(default_ctrl->devices, NULL, device,
					"org.bluez.Device1");
}

static struct adapter *find_parent(GDBusProxy *device)
{
	GList *list;

	for (list = g_list_first(ctrl_list); list; list = g_list_next(list)) {
		struct adapter *adapter = (struct adapter *)list->data;

		if (device_is_child(device, adapter->proxy) == TRUE)
			return adapter;
	}
	return NULL;
}

#define SERVER_CLASS_TELEPHONY	(1U << (22))
#define SERVER_CLASS_AUDIO		(1U << (21))
#define DEVICE_CLASS_SHIFT		8
#define DEVICE_CLASS_MASK		0x3f
#define DEVICE_CLASS_PHONE		2
#define DEVICE_CLASS_AUDIO		4

enum BT_Device_Class dist_dev_class(GDBusProxy *proxy)
{
	DBusMessageIter iter;
	const char *address_type = NULL;
	const char *address = NULL;
	const char *alias = NULL;
	dbus_uint32_t cod;

	if (g_dbus_proxy_get_property(proxy, "AddressType", &iter)) {
		dbus_message_iter_get_basic(&iter, &address_type);
		pr_info("%s addressType:%s\n", __func__, address_type);

		if (g_dbus_proxy_get_property(proxy, "Alias", &iter)) {
			dbus_message_iter_get_basic(&iter, &alias);
			pr_info("%s Alias: %s\n", __func__, alias);
		}

		if (g_dbus_proxy_get_property(proxy, "Address", &iter)) {
			dbus_message_iter_get_basic(&iter, &address);
			pr_info("%s address: %s\n", __func__, address);
		}

		if (!strcmp(address_type, "random")) {
			pr_info("%s The device is ble(random)\n", __func__);
			return BT_BLE_DEVICE;
		}

		if (!strcmp(address_type, "public")) {
			if (g_dbus_proxy_get_property(proxy, "Class", &iter) == FALSE) {
				pr_info("%s The device is ble(public)\n", __func__);
				return BT_BLE_DEVICE;
			} else {
				bool is_phone, is_audio;

				dbus_message_iter_get_basic(&iter, &cod);
				pr_info("%s class: 0x%x\n", __func__, cod);

				is_phone = ((cod >> DEVICE_CLASS_SHIFT) & DEVICE_CLASS_MASK) == DEVICE_CLASS_PHONE ? true : false;
				is_audio = ((cod >> DEVICE_CLASS_SHIFT) & DEVICE_CLASS_MASK) == DEVICE_CLASS_AUDIO ? true : false;

				if ((cod & SERVER_CLASS_TELEPHONY) && is_phone) {
					pr_info("%s The device is source\n", __func__);
					return BT_SOURCE_DEVICE;
				}

				if ((cod & SERVER_CLASS_AUDIO) && is_audio) {
					pr_info("%s The device is sink\n", __func__);
					return BT_SINK_DEVICE;
				}

				if (is_phone || is_audio) {
					DBusMessageIter value;
					const char *text;
					char str[26];
					unsigned int n;
					const char *uuid;
					enum BT_Device_Class ret = BT_IDLE;

					if (g_dbus_proxy_get_property(proxy, "UUIDs", &iter) == FALSE)
						return BT_IDLE;

					dbus_message_iter_recurse(&iter, &value);

					while (dbus_message_iter_get_arg_type(&value) == DBUS_TYPE_STRING) {
						dbus_message_iter_get_basic(&value, &uuid);

						text = bt_uuidstr_to_str(uuid);
						if (text) {
							str[sizeof(str) - 1] = '\0';

							n = snprintf(str, sizeof(str), "%s", text);
							if (n > sizeof(str) - 1) {
								  str[sizeof(str) - 2] = '.';
								  str[sizeof(str) - 3] = '.';
								  if (str[sizeof(str) - 4] == ' ')
										  str[sizeof(str) - 4] = '.';

								  n = sizeof(str) - 1;
							}

							if (strstr(str, "Audio Sink")) {
								ret = BT_SINK_DEVICE;
								pr_info("%s The device is sink\n", __func__);
								break;
							} else if (strstr(str, "Audio Source")) {
								ret = BT_SOURCE_DEVICE;
								pr_info("%s The device is source\n", __func__);
								break;
							}
						}

						dbus_message_iter_next(&value);
					}

					return ret;
				}
			}
		}
	}

	pr_info("%s The device is unknow\n", __func__);
	return BT_IDLE;
}

static void set_default_device(GDBusProxy *proxy, const char *attribute)
{
	char *desc = NULL;
	DBusMessageIter iter;
	const char *path;

	pr_info("%s: proxy is %s\n", __func__, proxy ? "non-null" : "null");
	pr_info("%s: default_dev %p, proxy: %p\n", __func__, default_dev, proxy);

	if ((proxy != NULL) && (default_dev != NULL) && (default_dev != proxy)) {
		pr_info("check proxy: ref: %d:%d\n", *((unsigned int *)proxy), *((unsigned int *)default_dev));
		return;
	}

	default_dev = proxy;

	if (proxy == NULL) {
		default_attr = NULL;
		goto done;
	}

	save_last_device(proxy);

	if (!g_dbus_proxy_get_property(proxy, "Alias", &iter)) {
		if (!g_dbus_proxy_get_property(proxy, "Address", &iter))
			goto done;
	}

	path = g_dbus_proxy_get_path(proxy);

	dbus_message_iter_get_basic(&iter, &desc);
	desc = g_strdup_printf(COLOR_BLUE "[%s%s%s]" COLOR_OFF "# ", desc,
				attribute ? ":" : "",
				attribute ? attribute + strlen(path) : "");
done:
	bt_shell_set_prompt(desc ? desc : PROMPT_ON);
	g_free(desc);
}

static void ble_connected_handler(GDBusProxy *proxy)
{
	if(!ble_is_open()) {
		pr_info("%s: ble is close\n", __func__);
		return;
	}

	if(ble_dev) {
		pr_info("%s: ble connection already exists\n", __func__);
		return;
	}

	ble_dev = proxy;
	pr_info("%s: ble_dev = %p\n", __func__, ble_dev);
	ble_state_send(RK_BLE_STATE_CONNECT);
}

static void ble_disconnect_handler()
{
	if(!ble_is_open()) {
		pr_info("%s: ble is close\n", __func__);
		return;
	}

	if(!ble_dev) {
		pr_info("%s: ble_dev is NULL\n", __func__);
		return;
	}

	ble_state_send(RK_BLE_STATE_DISCONNECT);

	ble_dev = NULL;
	ble_service_cnt = 0;
	pr_info("%s: ble disconneced\n", __func__);
	gatt_set_on_adv();
}

static void device_added(GDBusProxy *proxy)
{
	DBusMessageIter iter;
	struct adapter *adapter = find_parent(proxy);
	char dev_addr[18], dev_name[256];
	dbus_bool_t paired = FALSE;
	dbus_bool_t connected = FALSE;
	enum BT_Device_Class bdc;

	if (!adapter) {
		/* TODO: Error */
		return;
	}

	adapter->devices = g_list_append(adapter->devices, proxy);
	//print_device(proxy, COLORED_NEW);
	bt_shell_set_env(g_dbus_proxy_get_path(proxy), proxy);
	pr_info("%s: path: %s\n", __func__, g_dbus_proxy_get_path(proxy));

	bt_dev_found_send(proxy);

	if (g_dbus_proxy_get_property(proxy, "Connected", &iter))
		dbus_message_iter_get_basic(&iter, &connected);

	bdc = dist_dev_class(proxy);
	if(bdc == BT_SINK_DEVICE || bdc == BT_SOURCE_DEVICE) {
		if (g_dbus_proxy_get_property(proxy, "Paired", &iter) == TRUE) {
			dbus_message_iter_get_basic(&iter, &paired);
			if (!paired && connected) {
				bt_get_device_addr_by_proxy(proxy, dev_addr, 18);
				bt_get_device_name_by_proxy(proxy, dev_name, 256);
				bt_bond_state_send(dev_addr, dev_name, RK_BT_BOND_STATE_BONDING);
			}
		}

		if (default_dev)
			return;

		if (connected)
			set_default_device(proxy, NULL);
	} else {
		if(connected)
			ble_connected_handler(proxy);
	}
}

static struct adapter *find_ctrl(GList *source, const char *path);

static struct adapter *adapter_new(GDBusProxy *proxy)
{
	struct adapter *adapter = (struct adapter *)g_malloc0(sizeof(struct adapter));
	pr_info("=== %s ===\n", __func__);

	ctrl_list = g_list_append(ctrl_list, adapter);

	if (!default_ctrl)
		default_ctrl = adapter;

	return adapter;
}

static void adapter_added(GDBusProxy *proxy)
{
	struct adapter *adapter;
	char hostname_buf[HOSTNAME_MAX_LEN];

	pr_info("=== %s ===\n", __func__);

	adapter = find_ctrl(ctrl_list, g_dbus_proxy_get_path(proxy));
	if (!adapter)
		adapter = adapter_new(proxy);

	adapter->proxy = proxy;
	print_adapter(proxy, COLORED_NEW);

check_open:
	if(!bt_is_open()) {
		usleep(20 * 1000);
		goto check_open;
	}

	if (g_bt_content && g_bt_content->bt_name) {
		pr_info("%s: bt_name: %s\n", __func__, g_bt_content->bt_name);
		rk_bt_set_device_name(g_bt_content->bt_name);
	} else {
		bt_gethostname(hostname_buf, sizeof(hostname_buf));
		pr_info("%s: bt_name: %s\n", __func__, hostname_buf);
		rk_bt_set_device_name(hostname_buf);
	}

	msleep(50);
	exec_command_system("hciconfig hci0 piscan");

	//bt_state_send(RK_BT_STATE_ON);

	pr_info("%s: bt check open ok\n", __func__);
}

static void ad_manager_added(GDBusProxy *proxy)
{
	struct adapter *adapter;
	adapter = find_ctrl(ctrl_list, g_dbus_proxy_get_path(proxy));
	if (!adapter)
		adapter = adapter_new(proxy);

	adapter->ad_proxy = proxy;
}

static void le_proxy_added(GDBusProxy *proxy)
{
	enum BT_Device_Class bdc;

	if(!ble_is_open())
		return;

	bdc = dist_dev_class(proxy);
	//if(bdc == BT_SINK_DEVICE || bdc == BT_SOURCE_DEVICE) {
	//	pr_info("%s: bdc(%d) != ble\n", __func__, bdc);
	//	return;
	//}

	pr_info("%s: ble_service_cnt = %d\n", __func__, ble_service_cnt);
	if (ble_service_cnt == 0) {
		if(!ble_dev) {
			ble_dev = proxy;
			ble_state_send(RK_BLE_STATE_CONNECT);
			pr_info("%s: ble conneced, ble_dev = %p\n", __func__, ble_dev);
		}
	}

	ble_service_cnt++;
}

static void proxy_added(GDBusProxy *proxy, void *user_data)
{
	const char *interface;

	interface = g_dbus_proxy_get_interface(proxy);
	pr_info("BT Enter: proxy_added: %s\n", interface);

	if (!strcmp(interface, "org.bluez.Device1")) {
		device_added(proxy);
	} else if (!strcmp(interface, "org.bluez.Adapter1")) {
		adapter_added(proxy);
	} else if (!strcmp(interface, "org.bluez.AgentManager1")) {
		if (!agent_manager) {
			agent_manager = proxy;

			if (auto_register_agent &&
					!bt_shell_get_env("NON_INTERACTIVE")) {
				agent_register(dbus_conn, agent_manager,
							auto_register_agent);
				}
		}
	} else if (!strcmp(interface, "org.bluez.GattService1")) {
		GDBusProxy *le_proxy = service_is_child(proxy);
		if (le_proxy != NULL) {
			gatt_add_service(proxy);
			le_proxy_added(le_proxy);
		}
	} else if (!strcmp(interface, "org.bluez.GattCharacteristic1")) {
		gatt_add_characteristic(proxy);
	} else if (!strcmp(interface, "org.bluez.GattDescriptor1")) {
		gatt_add_descriptor(proxy);
	} else if (!strcmp(interface, "org.bluez.GattManager1")) {
		//gatt_add_manager(proxy);
		register_app(proxy);
	} else if (!strcmp(interface, "org.bluez.LEAdvertisingManager1")) {
		ad_manager_added(proxy);
	} else if (!strcmp(interface, "org.bluez.MediaTransport1")) {
		a2dp_master_event_send(BT_SOURCE_EVENT_CONNECTED, rsd.addr, rsd.name);
		a2dp_master_save_status(rsd.addr);
	}

	if (bt_sink_is_open()) {
		if ((!strcmp(interface, "org.bluez.MediaPlayer1")) ||
			(!strcmp(interface, "org.bluez.MediaFolder1")) ||
			(!strcmp(interface, "org.bluez.MediaItem1")))
			a2dp_sink_proxy_added(proxy, user_data);

		if (!strcmp(interface, "org.bluez.Device1")) {
			if ((dist_dev_class(proxy) == BT_SOURCE_DEVICE))
				a2dp_sink_proxy_added(proxy, user_data);
		}

		if (!strcmp(interface, "org.bluez.Adapter1")) {
			a2dp_sink_proxy_added(proxy, user_data);
		}
	}

	pr_info("BT Exit: proxy_added: %s\n", interface);
}

void set_default_attribute(GDBusProxy *proxy)
{
	//const char *path;

	default_attr = proxy;

	//path = g_dbus_proxy_get_path(proxy);

	//set_default_device(default_dev, path);
}

static void device_removed(GDBusProxy *proxy)
{
	char dev_addr[18], dev_name[256];
	dbus_bool_t paired;
	DBusMessageIter iter;

	pr_info("%s: path: %s\n", __func__, g_dbus_proxy_get_path(proxy));
	struct adapter *adapter = (struct adapter *)find_parent(proxy);
	if (!adapter) {
		pr_info("%s: adapter is NULL\n", __func__);
		/* TODO: Error */
		return;
	}

	if (g_dbus_proxy_get_property(proxy, "Paired", &iter) == TRUE) {
		dbus_message_iter_get_basic(&iter, &paired);
		if (paired) {
			bt_get_device_addr_by_proxy(proxy, dev_addr, 18);
			bt_get_device_name_by_proxy(proxy, dev_name, 256);
			pr_info("%s: addr: %s, name: %s [%d]\n", __func__, dev_addr, dev_name, paired);
			bt_bond_state_send(dev_addr, dev_name, RK_BT_BOND_STATE_NONE);
		}
	}

	adapter->devices = g_list_remove(adapter->devices, proxy);

	//print_device(proxy, COLORED_DEL);
	bt_shell_set_env(g_dbus_proxy_get_path(proxy), NULL);

	if (default_dev == proxy)
		set_default_device(NULL, NULL);
	else if(ble_dev == proxy)
		ble_disconnect_handler();
}

static void adapter_removed(GDBusProxy *proxy)
{
	GList *ll;

	for (ll = g_list_first(ctrl_list); ll; ll = g_list_next(ll)) {
		struct adapter *adapter = (struct adapter *)ll->data;

		if (adapter->proxy == proxy) {
			print_adapter(proxy, COLORED_DEL);
			bt_shell_set_env(g_dbus_proxy_get_path(proxy), NULL);

			if (default_ctrl && default_ctrl->proxy == proxy) {
				default_ctrl = NULL;
				set_default_device(NULL, NULL);
			}

			ctrl_list = g_list_remove_link(ctrl_list, ll);
			g_list_free(adapter->devices);
			g_free(adapter);
			g_list_free(ll);
			return;
		}
	}
}

static void le_proxy_removed(GDBusProxy *proxy)
{
	char *proxy_path, *ble_dev_path;

	if(!ble_is_open())
		return;

	if(!ble_dev) {
		pr_info("%s: ble_dev == NULL\n", __func__);
		return;
	}

	proxy_path = g_dbus_proxy_get_path(proxy);
	ble_dev_path = g_dbus_proxy_get_path(ble_dev);
	if(!g_str_has_prefix(proxy_path, ble_dev_path)) {
		pr_info("%s: proxy_path = %s, ble_dev_path = %s\n", __func__, proxy_path, ble_dev_path);
		return;
	}

	ble_service_cnt--;
	pr_info("%s: ble_service_cnt = %d\n", __func__, ble_service_cnt);
	if (ble_service_cnt == 0) {
		ble_disconnect_handler();
		ble_dev = NULL;
	}
}

static void proxy_removed(GDBusProxy *proxy, void *user_data)
{
	const char *interface;

	interface = g_dbus_proxy_get_interface(proxy);
	pr_info("BT Enter: proxy_removed: %s\n", interface);
	pr_info("%s: path: %s\n", __func__, g_dbus_proxy_get_path(proxy));

	if (!strcmp(interface, "org.bluez.Device1")) {
		device_removed(proxy);
	} else if (!strcmp(interface, "org.bluez.Adapter1")) {
		adapter_removed(proxy);
	} else if (!strcmp(interface, "org.bluez.AgentManager1")) {
		if (agent_manager == proxy) {
			agent_manager = NULL;
			if (auto_register_agent) {
				agent_unregister(dbus_conn, NULL);
			}
		}
	} else if (!strcmp(interface, "org.bluez.GattService1")) {
		gatt_remove_service(proxy);

		if (default_attr == proxy)
			set_default_attribute(NULL);

		le_proxy_removed(proxy);
	} else if (!strcmp(interface, "org.bluez.GattCharacteristic1")) {
		gatt_remove_characteristic(proxy);

		if (default_attr == proxy)
			set_default_attribute(NULL);
	} else if (!strcmp(interface, "org.bluez.GattDescriptor1")) {
		gatt_remove_descriptor(proxy);

		if (default_attr == proxy)
			set_default_attribute(NULL);
	} else if (!strcmp(interface, "org.bluez.GattManager1")) {
		//gatt_remove_manager(proxy);
		unregister_app(proxy);
	} else if (!strcmp(interface, "org.bluez.LEAdvertisingManager1")) {
		ad_unregister(dbus_conn, NULL);
	} else if (!strcmp(interface, "org.bluez.MediaTransport1")) {
		//a2dp_master_save_status(NULL);
		//a2dp_master_event_send(BT_SOURCE_EVENT_DISCONNECTED, NULL, NULL);
	}

	if (bt_sink_is_open())
		a2dp_sink_proxy_removed(proxy, user_data);
	pr_info("BT Exit: proxy_removed: %s\n", interface);
}

static struct adapter *find_ctrl(GList *source, const char *path)
{
	GList *list;

	for (list = g_list_first(source); list; list = g_list_next(list)) {
		struct adapter *adapter = (struct adapter *)list->data;

		if (!strcasecmp(g_dbus_proxy_get_path(adapter->proxy), path))
			return adapter;
	}

	return NULL;
}

static void device_paired_process(GDBusProxy *proxy,
					DBusMessageIter *iter, char *dev_addr)
{
	dbus_bool_t valbool = FALSE;
	char dev_name[256];

	bt_get_device_name_by_proxy(proxy, dev_name, 256);

	dbus_message_iter_get_basic(iter, &valbool);
	if(valbool)
		bt_bond_state_send(dev_addr, dev_name, RK_BT_BOND_STATE_BONDED);
	else
		bt_bond_state_send(dev_addr, dev_name, RK_BT_BOND_STATE_NONE);
}

static void source_connected_handler(GDBusProxy *proxy, enum BT_Device_Class bdc, dbus_bool_t connected)
{
	DBusMessageIter iter;
	DBusMessageIter addr_iter;
	const char *address = NULL;
	const char *name = NULL;

	if (!bt_source_is_open() || bdc != BT_SINK_DEVICE)
		return;

	if (g_dbus_proxy_get_property(proxy, "Alias", &iter) == TRUE)
		dbus_message_iter_get_basic(&iter, &name);
	else
		pr_info("%s: can't get remote device name\n", __func__);

	if (g_dbus_proxy_get_property(proxy, "Address", &addr_iter) == TRUE)
		dbus_message_iter_get_basic(&addr_iter, &address);
	else
		pr_info("%s: can't get remote device address\n", __func__);

	pr_info("%s thread tid = %lu\n", __func__, pthread_self());
	memset(&rsd, 0, sizeof(struct remote_sink_device));
	strcpy(rsd.addr, address);
	strcpy(rsd.name, name);

	pr_info("%s: connected: %d, rsd addr: %s, name: %s.\n", __func__, connected, rsd.addr, rsd.name);

	if ((connected == FALSE) && (g_device_state == BT_SOURCE_EVENT_CONNECTED)) {
		a2dp_master_save_status(NULL);
		a2dp_master_event_send(BT_SOURCE_EVENT_DISCONNECTED, address, name);
	}
}

static void ble_client_connected_handler(GDBusProxy *proxy, dbus_bool_t connected)
{
	if(!ble_client_is_open())
		return;

	if(connected) {
		if(ble_dev) {
			pr_info("%s: ble connection already exists\n", __func__);
			return;
		}

		ble_dev = proxy;
		gatt_client_state_send(RK_BLE_CLIENT_STATE_CONNECT);
		pr_info("%s: ble client conneced, ble_dev = %p\n", __func__, ble_dev);
	} else {
		if(!ble_dev) {
			pr_info("%s: ble_dev is NULL\n", __func__);
			return;
		}

		if(ble_dev != proxy) {
			pr_info("%s: ble_dev(%p) != proxy(%p)\n", __func__, ble_dev, proxy);
			return;
		}

		gatt_client_state_send(RK_BLE_CLIENT_STATE_DISCONNECT);
		ble_dev = NULL;
		pr_info("%s: ble client disconneced\n", __func__);
	}
}

static void device_connected_handler(GDBusProxy *proxy,
					DBusMessageIter *iter, void *user_data)
{
	dbus_bool_t connected = false;
	enum BT_Device_Class bdc;

	dbus_message_iter_get_basic(iter, &connected);

	bdc = dist_dev_class(proxy);
	if(bdc == BT_SOURCE_DEVICE || bdc == BT_SINK_DEVICE) {
		if (connected && default_dev == NULL)
			set_default_device(proxy, NULL);
		else if (!connected && default_dev == proxy)
			set_default_device(NULL, NULL);

		source_connected_handler(proxy, bdc, connected);

		//bt_sink
		if (bt_sink_is_open() && bdc == BT_SOURCE_DEVICE)
			device_changed(proxy, iter, user_data);
	} else if(bdc == BT_BLE_DEVICE) {
		if(ble_is_open()) {
			if(connected) {
				ble_connected_handler(proxy);
				return;
			}

			if(ble_dev != proxy)
				pr_info("%s: ble_dev(%p) != proxy(%p)\n", __func__, ble_dev, proxy);

			if(!connected && ble_dev == proxy) {
				const char *type = NULL;

				if (g_dbus_proxy_get_property(proxy, "AddressType", iter)) {
					dbus_message_iter_get_basic(iter, &type);
					pr_info("%s: AddressType = %s\n", __func__, type);
					if(!strcmp(type, "public"))
						remove_device(proxy);
				}
			}
		} else {
			ble_client_connected_handler(proxy, connected);
		}
	}
}

static void source_reconnect_handler(GDBusProxy *proxy, char *dev_addr)
{
	pthread_t tid;
	enum BT_Device_Class cod;

	if(!g_bt_source_info.is_reconnected)
		return;

	if(!strcmp(g_bt_source_info.reconnect_address, dev_addr)) {
		cod = dist_dev_class(proxy);
		if(cod != BT_SINK_DEVICE) {
			pr_info("%s: find reconnect device(%s), but cod(%d) != sink\n", __func__, dev_addr, cod);
			return;
		}

		source_set_reconnect_tag(false);
		pr_info("%s: reconnect device = %s\n", __func__, dev_addr);
		a2dp_master_event_send(BT_SOURCE_EVENT_AUTO_RECONNECTING, dev_addr, dev_addr);
		a2dp_master_connect(dev_addr);
	}
}

static void source_avrcp_keycode_handler(GDBusProxy *proxy, DBusMessageIter *iter)
{
	dbus_uint32_t keycode;

	dbus_message_iter_get_basic(iter, &keycode);
	switch(keycode) {
		case AVC_PLAY:
			a2dp_master_event_send(BT_SOURCE_EVENT_RC_PLAY, NULL, NULL);
			break;
		case AVC_PAUSE:
			a2dp_master_event_send(BT_SOURCE_EVENT_RC_PAUSE, NULL, NULL);
			break;
		case AVC_STOP:
			a2dp_master_event_send(BT_SOURCE_EVENT_RC_STOP, NULL, NULL);
			break;
		case AVC_VOLUME_UP:
			a2dp_master_event_send(BT_SOURCE_EVENT_RC_VOL_UP, NULL, NULL);
			break;
		case AVC_VOLUME_DOWN:
			a2dp_master_event_send(BT_SOURCE_EVENT_RC_VOL_DOWN, NULL, NULL);
			break;
		case AVC_FORWARD:
			a2dp_master_event_send(BT_SOURCE_EVENT_RC_FORWARD, NULL, NULL);
			break;
		case AVC_BACKWARD:
			a2dp_master_event_send(BT_SOURCE_EVENT_RC_BACKWARD, NULL, NULL);
			break;
		default:
			break;
	}
}

static void property_changed(GDBusProxy *proxy, const char *name,
					DBusMessageIter *iter, void *user_data)
{
	const char *interface;
	struct adapter *ctrl;

	interface = g_dbus_proxy_get_interface(proxy);
	pr_info("BT Enter: property_changed: %s\n", interface);

	if (!strcmp(interface, "org.bluez.Device1")) {
		if (default_ctrl && device_is_child(proxy,
					default_ctrl->proxy) == TRUE) {
			DBusMessageIter addr_iter;
			char *str;
			char dev_addr[18];

			bt_get_device_addr_by_proxy(proxy, dev_addr, 18);

			if (strcmp(name, "Paired") == 0)
				device_paired_process(proxy, iter, dev_addr);

			if (strcmp(name, "Connected") == 0)
				device_connected_handler(proxy, iter, user_data);

			if(strcmp(name, "RSSI") == 0)
				source_reconnect_handler(proxy, dev_addr);

			if(strcmp(name, "Alias") == 0)
				bt_name_change_send(proxy);

			if(strcmp(name, "MTU") == 0)
				ble_mtu_exchange_send(proxy);
		}
	} else if (!strcmp(interface, "org.bluez.Adapter1")) {
		DBusMessageIter addr_iter;
		char *str;

		if (g_dbus_proxy_get_property(proxy, "Address",
						&addr_iter) == TRUE) {
			const char *address;

			dbus_message_iter_get_basic(&addr_iter, &address);
			str = g_strdup_printf("[" COLORED_CHG
						"] Controller %s ", address);
		} else
			str = g_strdup("");

		if (!strcmp(name, "Powered"))
			adapter_changed(proxy, iter, user_data);

		if (!strcmp(name, "Discovering")) {
			dbus_bool_t val;
			dbus_message_iter_get_basic(iter, &val);
			pr_info("Adapter Discovering changed to %s", val ? "TRUE" : "FALSE");
			if (!val) {
				g_bt_scan_info.is_scaning = false;
				filter_clear_transport();
				bt_discovery_state_send(RK_BT_DISC_STOPPED_BY_USER);
			}
		}

		print_iter(str, name, iter);
		g_free(str);
	} else if (!strcmp(interface, "org.bluez.LEAdvertisingManager1")) {
		DBusMessageIter addr_iter;
		char *str;

		ctrl = find_ctrl(ctrl_list, g_dbus_proxy_get_path(proxy));
		if (!ctrl)
			return;

		if (g_dbus_proxy_get_property(ctrl->proxy, "Address",
						&addr_iter) == TRUE) {
			const char *address;

			dbus_message_iter_get_basic(&addr_iter, &address);
			str = g_strdup_printf("[" COLORED_CHG
						"] Controller %s ",
						address);
		} else
			str = g_strdup("");

		print_iter(str, name, iter);
		g_free(str);
	}else if (!strcmp(interface, "org.bluez.GattCharacteristic1")
		|| !strcmp(interface, "org.bluez.GattDescriptor1")) {
		char *str;

		str = g_strdup_printf("[" COLORED_CHG "] Attribute %s ",
						g_dbus_proxy_get_path(proxy));

		print_iter(str, name, iter);
		g_free(str);

		if(!strcmp(name, "Value"))
			gatt_client_recv_data_send(proxy, iter);
	} else if (!strcmp(interface, "org.bluez.MediaPlayer1")) {
		if (!strcmp(name, "KeyCode"))
			source_avrcp_keycode_handler(proxy, iter);
	}

	if (bt_sink_is_open())
		a2dp_sink_property_changed(proxy, name, iter, user_data);

	pr_info("BT Exit: property_changed: %s\n", interface);
}

static void message_handler(DBusConnection *connection,
					DBusMessage *message, void *user_data)
{
	pr_info("[SIGNAL] %s.%s\n", dbus_message_get_interface(message),
					dbus_message_get_member(message));
}

static struct adapter *find_ctrl_by_address(GList *source, const char *address)
{
	GList *list;

	for (list = g_list_first(source); list; list = g_list_next(list)) {
		struct adapter *adapter = (struct adapter *)list->data;
		DBusMessageIter iter;
		const char *str;

		if (g_dbus_proxy_get_property(adapter->proxy,
					"Address", &iter) == FALSE)
			continue;

		dbus_message_iter_get_basic(&iter, &str);

		if (!strcasecmp(str, address))
			return adapter;
	}

	return NULL;
}

static GDBusProxy *find_proxy_by_address(GList *source, const char *address)
{
	GList *list;

	for (list = g_list_first(source); list; list = g_list_next(list)) {
		GDBusProxy *proxy = (GDBusProxy *)list->data;
		DBusMessageIter iter;
		const char *str;

		if (g_dbus_proxy_get_property(proxy, "Address", &iter) == FALSE)
			continue;

		dbus_message_iter_get_basic(&iter, &str);
		if (!strcasecmp(str, address))
			return proxy;
	}

	return NULL;
}

static gboolean check_default_ctrl(void)
{
	if (!default_ctrl) {
		pr_info("%s: No default controller available\n", __func__);
		return FALSE;
	}

	return TRUE;
}

static gboolean parse_argument(int argc, char *argv[], const char **arg_table,
					const char *msg, dbus_bool_t *value,
					const char **option)
{
	const char **opt;

	if (!strcmp(argv[1], "on") || !strcmp(argv[1], "yes")) {
		*value = TRUE;
		if (option)
			*option = "";
		return TRUE;
	}

	if (!strcmp(argv[1], "off") || !strcmp(argv[1], "no")) {
		*value = FALSE;
		return TRUE;
	}

	for (opt = arg_table; opt && *opt; opt++) {
		if (strcmp(argv[1], *opt) == 0) {
			*value = TRUE;
			*option = *opt;
			return TRUE;
		}
	}

	pr_info("Invalid argument %s\n", argv[1]);
	return FALSE;
}

static void cmd_list(int argc, char *argv[])
{
	GList *list;

	for (list = g_list_first(ctrl_list); list; list = g_list_next(list)) {
		struct adapter *adapter = (struct adapter *)list->data;
		print_adapter(adapter->proxy, NULL);
	}
}

static void cmd_show(int argc, char *argv[])
{
	struct adapter *adapter;
	GDBusProxy *proxy;
	DBusMessageIter iter;
	const char *address;

	if (argc < 2 || !strlen(argv[1])) {
		if (check_default_ctrl() == FALSE)
			return bt_shell_noninteractive_quit(EXIT_FAILURE);

		proxy = default_ctrl->proxy;
	} else {
		adapter = find_ctrl_by_address(ctrl_list, argv[1]);
		if (!adapter) {
			pr_info("Controller %s not available\n",
								argv[1]);
			return bt_shell_noninteractive_quit(EXIT_FAILURE);
		}
		proxy = adapter->proxy;
	}

	if (g_dbus_proxy_get_property(proxy, "Address", &iter) == FALSE)
		return bt_shell_noninteractive_quit(EXIT_FAILURE);

	dbus_message_iter_get_basic(&iter, &address);

	if (g_dbus_proxy_get_property(proxy, "AddressType", &iter) == TRUE) {
		const char *type;

		dbus_message_iter_get_basic(&iter, &type);

		pr_info("Controller %s (%s)\n", address, type);
	} else {
		pr_info("Controller %s\n", address);
	}

	print_property(proxy, "Name");
	print_property(proxy, "Alias");
	print_property(proxy, "Class");
	print_property(proxy, "Powered");
	print_property(proxy, "Discoverable");
	print_property(proxy, "Pairable");
	print_uuids(proxy);
	print_property(proxy, "Modalias");
	print_property(proxy, "Discovering");

	return bt_shell_noninteractive_quit(EXIT_SUCCESS);
}

static void cmd_select(int argc, char *argv[])
{
	struct adapter *adapter;

	adapter = find_ctrl_by_address(ctrl_list, argv[1]);
	if (!adapter) {
		pr_info("Controller %s not available\n", argv[1]);
		return bt_shell_noninteractive_quit(EXIT_FAILURE);
	}

	if (default_ctrl && default_ctrl->proxy == adapter->proxy)
		return bt_shell_noninteractive_quit(EXIT_SUCCESS);

	default_ctrl = adapter;
	print_adapter(adapter->proxy, NULL);

	return bt_shell_noninteractive_quit(EXIT_SUCCESS);
}

static void cmd_devices(BtScanParam *param)
{
	GList *ll;

	if (check_default_ctrl() == FALSE)
		return bt_shell_noninteractive_quit(EXIT_SUCCESS);

	for (ll = g_list_first(default_ctrl->devices); ll; ll = g_list_next(ll)) {
		GDBusProxy *proxy = (GDBusProxy *)ll->data;

		if(param)
			btsrc_scan_save_device(proxy, param);
		else
			print_device(proxy, NULL);
	}

	return bt_shell_noninteractive_quit(EXIT_SUCCESS);
}

static void cmd_paired_devices()
{
	GList *ll;

	if (check_default_ctrl() == FALSE)
		return bt_shell_noninteractive_quit(EXIT_SUCCESS);

	for (ll = g_list_first(default_ctrl->devices);
			ll; ll = g_list_next(ll)) {
		GDBusProxy *proxy = (GDBusProxy *)ll->data;
		DBusMessageIter iter;
		dbus_bool_t paired;

		if (g_dbus_proxy_get_property(proxy, "Paired", &iter) == FALSE)
			continue;

		dbus_message_iter_get_basic(&iter, &paired);
		if (!paired)
			continue;

		print_device(proxy, NULL);
	}

	return bt_shell_noninteractive_quit(EXIT_SUCCESS);
}

static void generic_callback(const DBusError *error, void *user_data)
{
	if (dbus_error_is_set(error)) {
		pr_info("Set failed: %s\n", error->name);
		return bt_shell_noninteractive_quit(EXIT_FAILURE);
	} else {
		pr_info("Changing succeeded\n");
		return bt_shell_noninteractive_quit(EXIT_SUCCESS);
	}
}

static void cmd_system_alias(int argc, char *argv[])
{
	char *name;

	if (check_default_ctrl() == FALSE)
		return bt_shell_noninteractive_quit(EXIT_FAILURE);

	name = g_strdup(argv[1]);

	if (g_dbus_proxy_set_property_basic(default_ctrl->proxy, "Alias",
					DBUS_TYPE_STRING, &name,
					generic_callback, name, g_free) == TRUE)
		return;

	g_free(name);
}

static void cmd_reset_alias(int argc, char *argv[])
{
	char *name;

	if (check_default_ctrl() == FALSE)
		return bt_shell_noninteractive_quit(EXIT_FAILURE);

	name = g_strdup("");

	if (g_dbus_proxy_set_property_basic(default_ctrl->proxy, "Alias",
					DBUS_TYPE_STRING, &name,
					generic_callback, name, g_free) == TRUE)
		return;

	g_free(name);
}

static void cmd_power(int argc, char *argv[])
{
	dbus_bool_t powered;
	char *str;

	if (!parse_argument(argc, argv, NULL, NULL, &powered, NULL))
		return bt_shell_noninteractive_quit(EXIT_FAILURE);

	if (check_default_ctrl() == FALSE)
		return bt_shell_noninteractive_quit(EXIT_FAILURE);

	str = g_strdup_printf("power %s", powered == TRUE ? "on" : "off");

	if (g_dbus_proxy_set_property_basic(default_ctrl->proxy, "Powered",
					DBUS_TYPE_BOOLEAN, &powered,
					generic_callback, str, g_free) == TRUE)
		return;

	g_free(str);
}

static void cmd_pairable(int argc, char *argv[])
{
	dbus_bool_t pairable;
	char *str;

	if (!parse_argument(argc, argv, NULL, NULL, &pairable, NULL))
		return bt_shell_noninteractive_quit(EXIT_FAILURE);

	if (check_default_ctrl() == FALSE)
		return bt_shell_noninteractive_quit(EXIT_FAILURE);

	str = g_strdup_printf("pairable %s", pairable == TRUE ? "on" : "off");

	if (g_dbus_proxy_set_property_basic(default_ctrl->proxy, "Pairable",
					DBUS_TYPE_BOOLEAN, &pairable,
					generic_callback, str, g_free) == TRUE)
		return;

	g_free(str);

	return bt_shell_noninteractive_quit(EXIT_FAILURE);
}

static void cmd_discoverable(int argc, char *argv[])
{
	dbus_bool_t discoverable;
	char *str;

	if (!parse_argument(argc, argv, NULL, NULL, &discoverable, NULL))
		return bt_shell_noninteractive_quit(EXIT_FAILURE);

	if (check_default_ctrl() == FALSE)
		return bt_shell_noninteractive_quit(EXIT_FAILURE);

	str = g_strdup_printf("discoverable %s",
				discoverable == TRUE ? "on" : "off");

	if (g_dbus_proxy_set_property_basic(default_ctrl->proxy, "Discoverable",
					DBUS_TYPE_BOOLEAN, &discoverable,
					generic_callback, str, g_free) == TRUE)
		return;

	g_free(str);

	return bt_shell_noninteractive_quit(EXIT_FAILURE);
}

static void cmd_agent(int argc, char *argv[])
{
	dbus_bool_t enable;
	const char *capability;

	if (!parse_argument(argc, argv, agent_arguments, "capability",
						&enable, &capability))
		return bt_shell_noninteractive_quit(EXIT_FAILURE);

	if (enable == TRUE) {
		g_free(auto_register_agent);
		auto_register_agent = g_strdup(capability);

		if (agent_manager)
			agent_register(dbus_conn, agent_manager,
						auto_register_agent);
		else
			pr_info("Agent registration enabled\n");
	} else {
		g_free(auto_register_agent);
		auto_register_agent = NULL;

		if (agent_manager)
			agent_unregister(dbus_conn, agent_manager);
		else
			pr_info("Agent registration disabled\n");
	}

	return bt_shell_noninteractive_quit(EXIT_SUCCESS);
}

static void cmd_default_agent(int argc, char *argv[])
{
	agent_default(dbus_conn, agent_manager);
}

static void discovery_reply(DBusMessage *message, void *user_data)
{
	dbus_bool_t enable = GPOINTER_TO_UINT(user_data);
	DBusError error;

	dbus_error_init(&error);

	if (dbus_set_error_from_message(&error, message) == TRUE) {
		pr_info("%s: Failed to %s(%lu) discovery: %s\n", __func__,
				enable == TRUE ? "start" : "stop", pthread_self(), error.name);
		if(enable)
			bt_discovery_state_send(RK_BT_DISC_START_FAILED);
		else
			g_bt_scan_info.scan_off_failed = true;

		dbus_error_free(&error);
		return;
	}

	if(enable)
		bt_discovery_state_send(RK_BT_DISC_STARTED);

	pr_info("%s: Discovery %s(%lu)\n", __func__, enable ? "started" : "stopped", pthread_self());
	/* Leave the discovery running even on noninteractive mode */
}

static struct set_discovery_filter_args {
	char *transport;
	dbus_uint16_t rssi;
	dbus_int16_t pathloss;
	char **uuids;
	size_t uuids_len;
	dbus_bool_t duplicate;
	bool set;
} filter = {
	NULL,
	DISTANCE_VAL_INVALID,
	DISTANCE_VAL_INVALID,
	NULL,
	0,
	false,
	true,
};

static void scan_filter_transport()
{
	const char *type = NULL;

	switch(g_bt_scan_info.scan_type) {
		case SCAN_TYPE_AUTO:
			type = "auto";
			break;

		case SCAN_TYPE_BREDR:
			type = "bredr";
			break;

		case SCAN_TYPE_LE:
			type = "le";
			break;
	}

	if(type) {
		filter_clear_transport();
		filter.transport = g_strdup(type);
		filter.set = false;
	}
}

static void set_discovery_filter_setup(DBusMessageIter *iter, void *user_data)
{
	struct set_discovery_filter_args *args = (struct set_discovery_filter_args *)user_data;
	DBusMessageIter dict;

	dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY,
				DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
				DBUS_TYPE_STRING_AS_STRING
				DBUS_TYPE_VARIANT_AS_STRING
				DBUS_DICT_ENTRY_END_CHAR_AS_STRING, &dict);

	if(args->uuids_len > 0)
		g_dbus_dict_append_array(&dict, "UUIDs", DBUS_TYPE_STRING,
								&args->uuids,
								args->uuids_len);

	if (args->pathloss != DISTANCE_VAL_INVALID)
		g_dbus_dict_append_entry(&dict, "Pathloss", DBUS_TYPE_UINT16,
						&args->pathloss);

	if (args->rssi != DISTANCE_VAL_INVALID)
		g_dbus_dict_append_entry(&dict, "RSSI", DBUS_TYPE_INT16,
						&args->rssi);

	if (args->transport != NULL) {
		pr_info("%s: scan transport: %s\n", __func__, args->transport);
		g_dbus_dict_append_entry(&dict, "Transport", DBUS_TYPE_STRING,
						&args->transport);
	}

	if (args->duplicate)
		g_dbus_dict_append_entry(&dict, "DuplicateData",
						DBUS_TYPE_BOOLEAN,
						&args->duplicate);

	dbus_message_iter_close_container(iter, &dict);
}


static void set_discovery_filter_reply(DBusMessage *message, void *user_data)
{
	DBusError error;

	dbus_error_init(&error);
	if (dbus_set_error_from_message(&error, message) == TRUE) {
		pr_info("SetDiscoveryFilter failed: %s\n", error.name);
		dbus_error_free(&error);
		return bt_shell_noninteractive_quit(EXIT_FAILURE);
	}

	filter.set = true;

	pr_info("SetDiscoveryFilter success\n");

	return bt_shell_noninteractive_quit(EXIT_SUCCESS);
}

static void set_discovery_filter(void)
{
	if (check_default_ctrl() == FALSE || filter.set)
		return;

	if (g_dbus_proxy_method_call(default_ctrl->proxy, "SetDiscoveryFilter",
		set_discovery_filter_setup, set_discovery_filter_reply,
		&filter, NULL) == FALSE) {
		pr_info("Failed to set discovery filter\n");
		return bt_shell_noninteractive_quit(EXIT_FAILURE);
	}

	filter.set = true;
}

static int cmd_scan(const char *cmd)
{
	dbus_bool_t enable;
	const char *method;

	if (strcmp(cmd, "on") == 0) {
		enable = TRUE;
	} else if (strcmp(cmd, "off") == 0){
		enable = FALSE;
	} else {
		pr_info("ERROR: %s cmd(%s) is invalid!\n", __func__, cmd);
		return -1;
	}

	if (check_default_ctrl() == FALSE)
		return -1;

	if (enable == TRUE) {
		scan_filter_transport();
		set_discovery_filter();
		method = "StartDiscovery";
	} else
		method = "StopDiscovery";

	pr_info("%s method = %s\n", __func__, method);

	if (g_dbus_proxy_method_call(default_ctrl->proxy, method,
				NULL, discovery_reply,
				GUINT_TO_POINTER(enable), NULL) == FALSE) {
		pr_info("Failed to %s discovery\n",
					enable == TRUE ? "start" : "stop");
		return -1;
	}

	return 0;
}

static void cmd_scan_filter_uuids(int argc, char *argv[])
{
	if (argc < 2 || !strlen(argv[1])) {
		char **uuid;

		for (uuid = filter.uuids; uuid && *uuid; uuid++)
			print_uuid(*uuid);

		return bt_shell_noninteractive_quit(EXIT_SUCCESS);
	}

	g_strfreev(filter.uuids);
	filter.uuids = NULL;
	filter.uuids_len = 0;

	if (!strcmp(argv[1], "all"))
		goto commit;

	filter.uuids = g_strdupv(&argv[1]);
	if (!filter.uuids) {
		pr_info("Failed to parse input\n");
		return bt_shell_noninteractive_quit(EXIT_FAILURE);
	}

	filter.uuids_len = g_strv_length(filter.uuids);

commit:
	filter.set = false;
}

static void cmd_scan_filter_rssi(int argc, char *argv[])
{
	if (argc < 2 || !strlen(argv[1])) {
		if (filter.rssi != DISTANCE_VAL_INVALID)
			pr_info("RSSI: %d\n", filter.rssi);
		return bt_shell_noninteractive_quit(EXIT_SUCCESS);
	}

	filter.pathloss = DISTANCE_VAL_INVALID;
	filter.rssi = atoi(argv[1]);

	filter.set = false;
}

static void cmd_scan_filter_pathloss(int argc, char *argv[])
{
	if (argc < 2 || !strlen(argv[1])) {
		if (filter.pathloss != DISTANCE_VAL_INVALID)
			pr_info("Pathloss: %d\n",
						filter.pathloss);
		return bt_shell_noninteractive_quit(EXIT_SUCCESS);
	}

	filter.rssi = DISTANCE_VAL_INVALID;
	filter.pathloss = atoi(argv[1]);

	filter.set = false;
}

static void cmd_scan_filter_duplicate_data(int argc, char *argv[])
{
	if (argc < 2 || !strlen(argv[1])) {
		pr_info("DuplicateData: %s\n",
				filter.duplicate ? "on" : "off");
		return bt_shell_noninteractive_quit(EXIT_SUCCESS);
	}

	if (!strcmp(argv[1], "on"))
		filter.duplicate = true;
	else if (!strcmp(argv[1], "off"))
		filter.duplicate = false;
	else {
		pr_info("Invalid option: %s\n", argv[1]);
		return bt_shell_noninteractive_quit(EXIT_FAILURE);
	}

	filter.set = false;
}

static void filter_clear_uuids(void)
{
	g_strfreev(filter.uuids);
	filter.uuids = NULL;
	filter.uuids_len = 0;
}

static void filter_clear_rssi(void)
{
	filter.rssi = DISTANCE_VAL_INVALID;
}

static void filter_clear_pathloss(void)
{
	filter.pathloss = DISTANCE_VAL_INVALID;
}

static void filter_clear_transport()
{
	if(filter.transport) {
		pr_info("%s\n", __func__);
		g_free(filter.transport);
		filter.transport = NULL;
	}
}

static void filter_clear_duplicate(void)
{
	filter.duplicate = false;
}

struct clear_entry {
	const char *name;
	void (*clear) (void);
};

static const struct clear_entry filter_clear[] = {
	{ "uuids", filter_clear_uuids },
	{ "rssi", filter_clear_rssi },
	{ "pathloss", filter_clear_pathloss },
	{ "transport", filter_clear_transport },
	{ "duplicate-data", filter_clear_duplicate },
	{}
};

static char *filter_clear_generator(const char *text, int state)
{
	static int index, len;
	const char *arg;

	if (!state) {
		index = 0;
		len = strlen(text);
	}

	while ((arg = filter_clear[index].name)) {
		index++;

		if (!strncmp(arg, text, len))
			return strdup(arg);
	}

	return NULL;
}

static gboolean data_clear(const struct clear_entry *entry_table,
							const char *name)
{
	const struct clear_entry *entry;
	bool all = false;

	if (!name || !strlen(name) || !strcmp("all", name))
		all = true;

	for (entry = entry_table; entry && entry->name; entry++) {
		if (all || !strcmp(entry->name, name)) {
			entry->clear();
			if (!all)
				goto done;
		}
	}

	if (!all) {
		pr_info("Invalid argument %s\n", name);
		return FALSE;
	}

done:
	return TRUE;
}

static void cmd_scan_filter_clear(int argc, char *argv[])
{
	bool all = false;

	if (argc < 2 || !strlen(argv[1]))
		all = true;

	if (!data_clear(filter_clear, all ? "all" : argv[1]))
		return bt_shell_noninteractive_quit(EXIT_FAILURE);

	filter.set = false;

	if (check_default_ctrl() == FALSE)
		return bt_shell_noninteractive_quit(EXIT_FAILURE);

	set_discovery_filter();
}

static struct GDBusProxy *find_device(int argc, char *argv[])
{
	GDBusProxy *proxy;

	if (argc < 2 || !strlen(argv[1])) {
		if (default_dev)
			return default_dev;
		pr_info("Missing device address argument\n");
		return NULL;
	}

	if (check_default_ctrl() == FALSE)
		return NULL;

	proxy = find_proxy_by_address(default_ctrl->devices, argv[1]);
	if (!proxy) {
		pr_info("Device %s not available\n", argv[1]);
		return NULL;
	}

	return proxy;
}

struct GDBusProxy *find_device_by_address(char *address)
{
	GDBusProxy *proxy;

	if (!address || strlen(address) < 17) {
		if (default_dev)
			return default_dev;
		pr_info("Missing device address argument\n");
		return NULL;
	}

	if (check_default_ctrl() == FALSE)
		return NULL;

	proxy = find_proxy_by_address(default_ctrl->devices, address);
	if (!proxy) {
		pr_info("%s: Device %s not available\n", __func__, address);
		return NULL;
	}

	return proxy;
}

static void cmd_info(int argc, char *argv[])
{
	GDBusProxy *proxy;
	DBusMessageIter iter;
	const char *address;

	proxy = find_device(argc, argv);
	if (!proxy)
		return bt_shell_noninteractive_quit(EXIT_FAILURE);

	if (g_dbus_proxy_get_property(proxy, "Address", &iter) == FALSE)
		return bt_shell_noninteractive_quit(EXIT_FAILURE);

	dbus_message_iter_get_basic(&iter, &address);

	if (g_dbus_proxy_get_property(proxy, "AddressType", &iter) == TRUE) {
		const char *type;

		dbus_message_iter_get_basic(&iter, &type);

		pr_info("Device %s (%s)\n", address, type);
	} else {
		pr_info("Device %s\n", address);
	}

	print_property(proxy, "Name");
	print_property(proxy, "Alias");
	print_property(proxy, "Class");
	print_property(proxy, "Appearance");
	print_property(proxy, "Icon");
	print_property(proxy, "Paired");
	print_property(proxy, "Trusted");
	print_property(proxy, "Blocked");
	print_property(proxy, "Connected");
	print_property(proxy, "LegacyPairing");
	print_property(proxy, "Modalias");
	print_property(proxy, "ManufacturerData");
	print_property(proxy, "ServiceData");
	print_property(proxy, "RSSI");
	print_property(proxy, "TxPower");
	print_property(proxy, "AdvertisingFlags");
	print_property(proxy, "AdvertisingData");
	print_uuids(proxy);

	return bt_shell_noninteractive_quit(EXIT_SUCCESS);
}

static void pair_reply(DBusMessage *message, void *user_data)
{
	DBusError error;

	dbus_error_init(&error);

	if (dbus_set_error_from_message(&error, message) == TRUE) {
		pr_info("Failed to pair: %s\n", error.name);
		dbus_error_free(&error);
		return bt_shell_noninteractive_quit(EXIT_FAILURE);
	}

	pr_info("Pairing successful\n");

	return bt_shell_noninteractive_quit(EXIT_SUCCESS);
}

static const char *proxy_address(GDBusProxy *proxy)
{
	DBusMessageIter iter;
	const char *addr;

	if (!g_dbus_proxy_get_property(proxy, "Address", &iter))
		return NULL;

	dbus_message_iter_get_basic(&iter, &addr);

	return addr;
}

static int cmd_pair(GDBusProxy *proxy)
{
	if (!proxy)
		return -1;

	if (g_dbus_proxy_method_call(proxy, "Pair", NULL, pair_reply,
							NULL, NULL) == FALSE) {
		pr_info("%s: Failed to pair\n", __func__);
		return -1;
	}

	pr_info("%s: Attempting to pair with %s\n", __func__, proxy_address(proxy));
	return 0;
}

static void cmd_trust(int argc, char *argv[])
{
	GDBusProxy *proxy;
	dbus_bool_t trusted;
	char *str;

	proxy = find_device(argc, argv);
	if (!proxy)
		return bt_shell_noninteractive_quit(EXIT_FAILURE);

	trusted = TRUE;

	str = g_strdup_printf("%s trust", proxy_address(proxy));

	if (g_dbus_proxy_set_property_basic(proxy, "Trusted",
					DBUS_TYPE_BOOLEAN, &trusted,
					generic_callback, str, g_free) == TRUE)
		return;

	g_free(str);

	return bt_shell_noninteractive_quit(EXIT_FAILURE);
}

static void cmd_untrust(int argc, char *argv[])
{
	GDBusProxy *proxy;
	dbus_bool_t trusted;
	char *str;

	proxy = find_device(argc, argv);
	if (!proxy)
		return bt_shell_noninteractive_quit(EXIT_FAILURE);

	trusted = FALSE;

	str = g_strdup_printf("%s untrust", proxy_address(proxy));

	if (g_dbus_proxy_set_property_basic(proxy, "Trusted",
					DBUS_TYPE_BOOLEAN, &trusted,
					generic_callback, str, g_free) == TRUE)
		return;

	g_free(str);

	return bt_shell_noninteractive_quit(EXIT_FAILURE);
}

static void cmd_block(int argc, char *argv[])
{
	GDBusProxy *proxy;
	dbus_bool_t blocked;
	char *str;

	proxy = find_device(argc, argv);
	if (!proxy)
		return bt_shell_noninteractive_quit(EXIT_FAILURE);

	blocked = TRUE;

	str = g_strdup_printf("%s block", proxy_address(proxy));

	if (g_dbus_proxy_set_property_basic(proxy, "Blocked",
					DBUS_TYPE_BOOLEAN, &blocked,
					generic_callback, str, g_free) == TRUE)
		return;

	g_free(str);

	return bt_shell_noninteractive_quit(EXIT_FAILURE);
}

static void cmd_unblock(int argc, char *argv[])
{
	GDBusProxy *proxy;
	dbus_bool_t blocked;
	char *str;

	proxy = find_device(argc, argv);
	if (!proxy)
		return bt_shell_noninteractive_quit(EXIT_FAILURE);

	blocked = FALSE;

	str = g_strdup_printf("%s unblock", proxy_address(proxy));

	if (g_dbus_proxy_set_property_basic(proxy, "Blocked",
					DBUS_TYPE_BOOLEAN, &blocked,
					generic_callback, str, g_free) == TRUE)
		return;

	g_free(str);

	return bt_shell_noninteractive_quit(EXIT_FAILURE);
}

static void remove_device_reply(DBusMessage *message, void *user_data)
{
	DBusError error;

	dbus_error_init(&error);

	if (dbus_set_error_from_message(&error, message) == TRUE) {
		pr_info("Failed to remove device: %s\n", error.name);
		//a2dp_master_event_send(BT_SOURCE_EVENT_REMOVE_FAILED, "", "");
		dbus_error_free(&error);
		return bt_shell_noninteractive_quit(EXIT_FAILURE);
	}

	pr_info("%s: Device has been removed\n", __func__);
	//a2dp_master_event_send(BT_SOURCE_EVENT_REMOVED, "", "");
	return bt_shell_noninteractive_quit(EXIT_SUCCESS);
}

static void remove_device_setup(DBusMessageIter *iter, void *user_data)
{
	char *path = (char *)user_data;

	dbus_message_iter_append_basic(iter, DBUS_TYPE_OBJECT_PATH, &path);
}

static int remove_device(GDBusProxy *proxy)
{
	char *path;

	path = g_strdup(g_dbus_proxy_get_path(proxy));

	if (check_default_ctrl() == FALSE)
		return -1;

	pr_info("%s: Attempting to remove device with %s\n", __func__, proxy_address(proxy));
	if (g_dbus_proxy_method_call(default_ctrl->proxy, "RemoveDevice",
						remove_device_setup,
						remove_device_reply,
						path, g_free) == FALSE) {
		pr_info("%s: Failed to remove device\n", __func__);
		g_free(path);
		return -1;
	}

	return 0;
}

static void cmd_remove(int argc, char *argv[])
{
	GDBusProxy *proxy;

	if (check_default_ctrl() == FALSE)
		return bt_shell_noninteractive_quit(EXIT_FAILURE);

	pr_info("%s\n", __func__);
	if (strcmp(argv[1], "*") == 0) {
		GList *list;

		for (list = default_ctrl->devices; list;
						list = g_list_next(list)) {
			GDBusProxy *proxy = (GDBusProxy *)list->data;

			remove_device(proxy);
		}
		return;
	}

	proxy = find_proxy_by_address(default_ctrl->devices, argv[1]);
	if (!proxy) {
		pr_info("Device %s not available\n", argv[1]);
		return bt_shell_noninteractive_quit(EXIT_FAILURE);
	}

	remove_device(proxy);
}

static void cmd_list_attributes(int argc, char *argv[])
{
	GDBusProxy *proxy;

	proxy = find_device(argc, argv);
	if (!proxy)
		return bt_shell_noninteractive_quit(EXIT_FAILURE);

	gatt_list_attributes(g_dbus_proxy_get_path(proxy));

	return bt_shell_noninteractive_quit(EXIT_SUCCESS);
}

static void cmd_set_alias(int argc, char *argv[])
{
	char *name;

	if (!default_dev) {
		pr_info("No device connected\n");
		return bt_shell_noninteractive_quit(EXIT_FAILURE);
	}

	name = g_strdup(argv[1]);

	if (g_dbus_proxy_set_property_basic(default_dev, "Alias",
					DBUS_TYPE_STRING, &name,
					generic_callback, name, g_free) == TRUE)
		return;

	g_free(name);

	return bt_shell_noninteractive_quit(EXIT_FAILURE);
}

static struct GDBusProxy *find_attribute(int argc, char *argv[])
{
	GDBusProxy *proxy;

	if (argc < 2 || !strlen(argv[1])) {
		if (default_attr)
			return default_attr;
		pr_info("Missing attribute argument\n");
		return NULL;
	}

	proxy = gatt_select_attribute(default_attr, argv[1]);
	if (!proxy) {
		pr_info("Attribute %s not available\n", argv[1]);
		return NULL;
	}

	return proxy;
}

static void cmd_attribute_info(int argc, char *argv[])
{
	GDBusProxy *proxy;
	DBusMessageIter iter;
	const char *iface, *uuid, *text;

	proxy = find_attribute(argc, argv);
	if (!proxy)
		return bt_shell_noninteractive_quit(EXIT_FAILURE);

	if (g_dbus_proxy_get_property(proxy, "UUID", &iter) == FALSE)
		return bt_shell_noninteractive_quit(EXIT_FAILURE);

	dbus_message_iter_get_basic(&iter, &uuid);

	text = bt_uuidstr_to_str(uuid);
	if (!text)
		text = g_dbus_proxy_get_path(proxy);

	iface = g_dbus_proxy_get_interface(proxy);
	if (!strcmp(iface, "org.bluez.GattService1")) {
		pr_info("Service - %s\n", text);

		print_property(proxy, "UUID");
		print_property(proxy, "Primary");
		print_property(proxy, "Characteristics");
		print_property(proxy, "Includes");
	} else if (!strcmp(iface, "org.bluez.GattCharacteristic1")) {
		pr_info("Characteristic - %s\n", text);

		print_property(proxy, "UUID");
		print_property(proxy, "Service");
		print_property(proxy, "Value");
		print_property(proxy, "Notifying");
		print_property(proxy, "Flags");
		print_property(proxy, "Descriptors");
	} else if (!strcmp(iface, "org.bluez.GattDescriptor1")) {
		pr_info("Descriptor - %s\n", text);

		print_property(proxy, "UUID");
		print_property(proxy, "Characteristic");
		print_property(proxy, "Value");
	}

	return bt_shell_noninteractive_quit(EXIT_SUCCESS);
}

static void cmd_acquire_write(int argc, char *argv[])
{
	if (!default_attr) {
		pr_info("No attribute selected\n");
		return bt_shell_noninteractive_quit(EXIT_FAILURE);
	}

	gatt_acquire_write(default_attr, argv[1]);
}

static void cmd_release_write(int argc, char *argv[])
{
	if (!default_attr) {
		pr_info("No attribute selected\n");
		return bt_shell_noninteractive_quit(EXIT_FAILURE);
	}

	gatt_release_write(default_attr, argv[1]);
}

static void cmd_acquire_notify(int argc, char *argv[])
{
	if (!default_attr) {
		pr_info("No attribute selected\n");
		return bt_shell_noninteractive_quit(EXIT_FAILURE);
	}

	gatt_acquire_notify(default_attr, argv[1]);
}

static void cmd_release_notify(int argc, char *argv[])
{
	if (!default_attr) {
		pr_info("No attribute selected\n");
		return bt_shell_noninteractive_quit(EXIT_FAILURE);
	}

	gatt_release_notify(default_attr, argv[1]);
}

static void cmd_notify(int argc, char *argv[])
{
	dbus_bool_t enable;

	if (!parse_argument(argc, argv, NULL, NULL, &enable, NULL))
		return bt_shell_noninteractive_quit(EXIT_FAILURE);

	if (!default_attr) {
		pr_info("No attribute selected\n");
		return bt_shell_noninteractive_quit(EXIT_FAILURE);
	}

	gatt_notify_attribute(default_attr, enable ? true : false);
}

static void cmd_register_app(int argc, char *argv[])
{
	if (check_default_ctrl() == FALSE)
		return bt_shell_noninteractive_quit(EXIT_FAILURE);

	gatt_register_app(dbus_conn, default_ctrl->proxy, argc, argv);
}

static void cmd_unregister_app(int argc, char *argv[])
{
	if (check_default_ctrl() == FALSE)
		return bt_shell_noninteractive_quit(EXIT_FAILURE);

	gatt_unregister_app(dbus_conn, default_ctrl->proxy);
}

static void cmd_register_service(int argc, char *argv[])
{
	if (check_default_ctrl() == FALSE)
		return bt_shell_noninteractive_quit(EXIT_FAILURE);

	gatt_register_service(dbus_conn, default_ctrl->proxy, argc, argv);
}

static void cmd_register_includes(int argc, char *argv[])
{
	if (check_default_ctrl() == FALSE)
		return bt_shell_noninteractive_quit(EXIT_FAILURE);

	gatt_register_include(dbus_conn, default_ctrl->proxy, argc, argv);
}

static void cmd_unregister_includes(int argc, char *argv[])
{
	if (check_default_ctrl() == FALSE)
		return bt_shell_noninteractive_quit(EXIT_FAILURE);

	gatt_unregister_include(dbus_conn, default_ctrl->proxy, argc, argv);
}

static void cmd_unregister_service(int argc, char *argv[])
{
	if (check_default_ctrl() == FALSE)
		return bt_shell_noninteractive_quit(EXIT_FAILURE);

	gatt_unregister_service(dbus_conn, default_ctrl->proxy, argc, argv);
}

static void cmd_register_characteristic(int argc, char *argv[])
{
	if (check_default_ctrl() == FALSE)
		return bt_shell_noninteractive_quit(EXIT_FAILURE);

	gatt_register_chrc(dbus_conn, default_ctrl->proxy, argc, argv);
}

static void cmd_unregister_characteristic(int argc, char *argv[])
{
	if (check_default_ctrl() == FALSE)
		return bt_shell_noninteractive_quit(EXIT_FAILURE);

	gatt_unregister_chrc(dbus_conn, default_ctrl->proxy, argc, argv);
}

static void cmd_register_descriptor(int argc, char *argv[])
{
	if (check_default_ctrl() == FALSE)
		return bt_shell_noninteractive_quit(EXIT_FAILURE);

	gatt_register_desc(dbus_conn, default_ctrl->proxy, argc, argv);
}

static void cmd_unregister_descriptor(int argc, char *argv[])
{
	if (check_default_ctrl() == FALSE)
		return bt_shell_noninteractive_quit(EXIT_FAILURE);

	gatt_unregister_desc(dbus_conn, default_ctrl->proxy, argc, argv);
}

static char *generic_generator(const char *text, int state,
					GList *source, const char *property)
{
	static int index, len;
	GList *list;

	if (!state) {
		index = 0;
		len = strlen(text);
	}

	for (list = g_list_nth(source, index); list;
						list = g_list_next(list)) {
		GDBusProxy *proxy = (GDBusProxy *)list->data;
		DBusMessageIter iter;
		const char *str;

		index++;

		if (g_dbus_proxy_get_property(proxy, property, &iter) == FALSE)
			continue;

		dbus_message_iter_get_basic(&iter, &str);

		if (!strncasecmp(str, text, len))
			return strdup(str);
	}

	return NULL;
}

static char *ctrl_generator(const char *text, int state)
{
	static int index = 0;
	static int len = 0;
	GList *list;

	if (!state) {
		index = 0;
		len = strlen(text);
	}

	for (list = g_list_nth(ctrl_list, index); list;
						list = g_list_next(list)) {
		struct adapter *adapter = (struct adapter *)list->data;
		DBusMessageIter iter;
		const char *str;

		index++;

		if (g_dbus_proxy_get_property(adapter->proxy,
					"Address", &iter) == FALSE)
			continue;

		dbus_message_iter_get_basic(&iter, &str);

		if (!strncasecmp(str, text, len))
			return strdup(str);
	}

	return NULL;
}

static char *dev_generator(const char *text, int state)
{
	return generic_generator(text, state,
			default_ctrl ? default_ctrl->devices : NULL, "Address");
}

static char *attribute_generator(const char *text, int state)
{
	return gatt_attribute_generator(text, state);
}

static char *argument_generator(const char *text, int state,
					const char *args_list[])
{
	static int index, len;
	const char *arg;

	if (!state) {
		index = 0;
		len = strlen(text);
	}

	while ((arg = args_list[index])) {
		index++;

		if (!strncmp(arg, text, len))
			return strdup(arg);
	}

	return NULL;
}

static char *capability_generator(const char *text, int state)
{
	return argument_generator(text, state, agent_arguments);
}

static void cmd_advertise(int argc, char *argv[])
{
	dbus_bool_t enable;
	const char *type;

	if (!parse_argument(argc, argv, ad_arguments, "type",
					&enable, &type))
		return bt_shell_noninteractive_quit(EXIT_FAILURE);

	if (!default_ctrl || !default_ctrl->ad_proxy) {
		pr_info("LEAdvertisingManager not found\n");
		bt_shell_noninteractive_quit(EXIT_FAILURE);
	}

	if (enable == TRUE)
		ad_register(dbus_conn, default_ctrl->ad_proxy, type);
	else
		ad_unregister(dbus_conn, default_ctrl->ad_proxy);
}

static char *ad_generator(const char *text, int state)
{
	return argument_generator(text, state, ad_arguments);
}

static void cmd_advertise_uuids(int argc, char *argv[])
{
	ad_advertise_uuids(dbus_conn, argc, argv);
}

static void cmd_advertise_service(int argc, char *argv[])
{
	ad_advertise_service(dbus_conn, argc, argv);
}

static void cmd_advertise_manufacturer(int argc, char *argv[])
{
	ad_advertise_manufacturer(dbus_conn, argc, argv);
}

static void cmd_advertise_data(int argc, char *argv[])
{
	ad_advertise_data(dbus_conn, argc, argv);
}

static void cmd_advertise_discoverable(int argc, char *argv[])
{
	dbus_bool_t discoverable;

	if (argc < 2) {
		ad_advertise_discoverable(dbus_conn, NULL);
		return;
	}

	if (!parse_argument(argc, argv, NULL, NULL, &discoverable, NULL))
		return bt_shell_noninteractive_quit(EXIT_FAILURE);

	ad_advertise_discoverable(dbus_conn, &discoverable);
}

static void cmd_advertise_discoverable_timeout(int argc, char *argv[])
{
	long int value;
	char *endptr = NULL;

	if (argc < 2) {
		ad_advertise_discoverable_timeout(dbus_conn, NULL);
		return;
	}

	value = strtol(argv[1], &endptr, 0);
	if (!endptr || *endptr != '\0' || value > UINT16_MAX) {
		pr_info("Invalid argument\n");
		return bt_shell_noninteractive_quit(EXIT_FAILURE);
	}

	ad_advertise_discoverable_timeout(dbus_conn, &value);
}

static void cmd_advertise_tx_power(int argc, char *argv[])
{
	dbus_bool_t powered;

	if (argc < 2) {
		ad_advertise_tx_power(dbus_conn, NULL);
		return;
	}

	if (!parse_argument(argc, argv, NULL, NULL, &powered, NULL))
		return bt_shell_noninteractive_quit(EXIT_FAILURE);

	ad_advertise_tx_power(dbus_conn, &powered);
}

static void cmd_advertise_name(int argc, char *argv[])
{
	if (argc < 2) {
		ad_advertise_local_name(dbus_conn, NULL);
		return;
	}

	if (strcmp(argv[1], "on") == 0 || strcmp(argv[1], "yes") == 0) {
		ad_advertise_name(dbus_conn, true);
		return;
	}

	if (strcmp(argv[1], "off") == 0 || strcmp(argv[1], "no") == 0) {
		ad_advertise_name(dbus_conn, false);
		return;
	}

	ad_advertise_local_name(dbus_conn, argv[1]);
}

static void cmd_advertise_appearance(int argc, char *argv[])
{
	long int value;
	char *endptr = NULL;

	if (argc < 2) {
		ad_advertise_local_appearance(dbus_conn, NULL);
		return;
	}

	if (strcmp(argv[1], "on") == 0 || strcmp(argv[1], "yes") == 0) {
		ad_advertise_appearance(dbus_conn, true);
		return;
	}

	if (strcmp(argv[1], "off") == 0 || strcmp(argv[1], "no") == 0) {
		ad_advertise_appearance(dbus_conn, false);
		return;
	}

	value = strtol(argv[1], &endptr, 0);
	if (!endptr || *endptr != '\0' || value > UINT16_MAX) {
		pr_info("Invalid argument\n");
		return bt_shell_noninteractive_quit(EXIT_FAILURE);
	}

	ad_advertise_local_appearance(dbus_conn, &value);
}

static void cmd_advertise_duration(int argc, char *argv[])
{
	long int value;
	char *endptr = NULL;

	if (argc < 2) {
		ad_advertise_duration(dbus_conn, NULL);
		return;
	}

	value = strtol(argv[1], &endptr, 0);
	if (!endptr || *endptr != '\0' || value > UINT16_MAX) {
		pr_info("Invalid argument\n");
		return bt_shell_noninteractive_quit(EXIT_FAILURE);
	}

	ad_advertise_duration(dbus_conn, &value);
}

static void cmd_advertise_timeout(int argc, char *argv[])
{
	long int value;
	char *endptr = NULL;

	if (argc < 2) {
		ad_advertise_timeout(dbus_conn, NULL);
		return;
	}

	value = strtol(argv[1], &endptr, 0);
	if (!endptr || *endptr != '\0' || value > UINT16_MAX) {
		pr_info("Invalid argument\n");
		return bt_shell_noninteractive_quit(EXIT_FAILURE);
	}

	ad_advertise_timeout(dbus_conn, &value);
}

static void ad_clear_uuids(void)
{
	ad_disable_uuids(dbus_conn);
}

static void ad_clear_service(void)
{
	ad_disable_service(dbus_conn);
}

static void ad_clear_manufacturer(void)
{
	ad_disable_manufacturer(dbus_conn);
}

static void ad_clear_data(void)
{
	ad_disable_data(dbus_conn);
}

static void ad_clear_tx_power(void)
{
	dbus_bool_t powered = false;

	ad_advertise_tx_power(dbus_conn, &powered);
}

static void ad_clear_name(void)
{
	ad_advertise_name(dbus_conn, false);
}

static void ad_clear_appearance(void)
{
	ad_advertise_appearance(dbus_conn, false);
}

static void ad_clear_duration(void)
{
	long int value = 0;

	ad_advertise_duration(dbus_conn, &value);
}

static void ad_clear_timeout(void)
{
	long int value = 0;

	ad_advertise_timeout(dbus_conn, &value);
}

static const struct clear_entry ad_clear[] = {
	{ "uuids",      ad_clear_uuids },
	{ "service",        ad_clear_service },
	{ "manufacturer",   ad_clear_manufacturer },
	{ "data",       ad_clear_data },
	{ "tx-power",       ad_clear_tx_power },
	{ "name",       ad_clear_name },
	{ "appearance",     ad_clear_appearance },
	{ "duration",       ad_clear_duration },
	{ "timeout",        ad_clear_timeout },
	{}
};

static void cmd_ad_clear(int argc, char *argv[])
{
	bool all = false;

	if (argc < 2 || !strlen(argv[1]))
		all = true;

	if(!data_clear(ad_clear, all ? "all" : argv[1]))
		return bt_shell_noninteractive_quit(EXIT_FAILURE);
}

static const struct option options[] = {
	{ "agent",  required_argument, 0, 'a' },
	{ 0, 0, 0, 0 }
};

static const char *agent_option;

static const char **optargs[] = {
	&agent_option
};

static const char *help[] = {
	"Register agent handler: <capability>"
};

static const struct bt_shell_opt opt = {
	.options = options,
	.optno = sizeof(options) / sizeof(struct option),
	.optstr = "a:",
	.optarg = optargs,
	.help = help,
};

static void client_ready(GDBusClient *client, void *user_data)
{
	return;
}

static guint reconnect_timer;
static void connect_reply(DBusMessage *message, void *user_data)
{
	struct ConnectContext_t *ctx_ptr = (struct ConnectContext_t *)user_data;
	if (!ctx_ptr) {
		pr_err("%s: invaild connect context!\n", __func__);
		return;
	}
	GDBusProxy *proxy = ctx_ptr->proxy;
	DBusError error;
	static int conn_count = 0;
	DBusMessageIter iter;
	const char *address = ctx_ptr->address;
	char addr[18], name[256];

	dbus_error_init(&error);

	g_bt_source_info.is_connecting = false;
	if (dbus_set_error_from_message(&error, message) == TRUE) {
		pr_info("%s: Failed to connect: %s\n", __func__, error.name);
		dbus_error_free(&error);

		conn_count--;
		if (conn_count > 0) {
			if(g_dbus_proxy_get_property(proxy, "Address", &iter) == FALSE) {
				pr_err("%s: can't get address!\n", __func__);
				return;
			}

			dbus_message_iter_get_basic(&iter, &address);
			if (reconnect_timer) {
				g_source_remove(reconnect_timer);
				reconnect_timer = 0;
			}
			reconnect_timer = g_timeout_add_seconds(3,
						a2dp_master_connect, address);
			return;
		}

		if(bt_source_is_open()) {
			pr_err("%s: ---------------------- at here\n", __func__);
			memset(addr, 0, 18);
			memset(name, 0, 256);
			//bt_get_device_addr_by_proxy(proxy, addr, 18);
			//bt_get_device_name_by_proxy(proxy, name, 256);//fail maybe no name
			a2dp_master_event_send(BT_SOURCE_EVENT_CONNECT_FAILED, address, address);
		}
		pr_err("%s: ------------------ ending\n", __func__);
		conn_count = 0;
		return bt_shell_noninteractive_quit(EXIT_FAILURE);
	}

	pr_info("%s: Connection successful\n", __func__);
	//set_default_device(proxy, NULL);

	return bt_shell_noninteractive_quit(EXIT_SUCCESS);
}

static void disconn_reply(DBusMessage *message, void *user_data)
{
	GDBusProxy *proxy = (GDBusProxy *)user_data;
	DBusError error;

	dbus_error_init(&error);

	if (dbus_set_error_from_message(&error, message) == TRUE) {
		char addr[18], name[256];

		pr_info("Failed to disconnect: %s\n", error.name);
		dbus_error_free(&error);

		if(bt_source_is_open()) {
			memset(addr, 0, 18);
			memset(name, 0, 256);
			bt_get_device_addr_by_proxy(proxy, addr, 18);
			bt_get_device_name_by_proxy(proxy, name, 256);
			a2dp_master_event_send(BT_SOURCE_EVENT_DISCONNECT_FAILED, addr, name);
		}

		return bt_shell_noninteractive_quit(EXIT_FAILURE);
	}

	pr_info("%s: Successful disconnected\n", __func__);

	//check disconnect
	if(bt_is_connected())
		pr_info("\n\n%s: The ACL link still exists!\n\n\n", __func__);

	if (proxy == default_dev)
		set_default_device(NULL, NULL);

	return bt_shell_noninteractive_quit(EXIT_SUCCESS);
}

static void a2dp_source_clean(void)
{
	dbus_conn = NULL;
	agent_manager = NULL;
	auto_register_agent = NULL;

	default_ctrl = NULL;
	ctrl_list = NULL;
	default_dev = NULL;
	default_attr = NULL;

	btsrc_client = NULL;
}

static void bluetooth_open(RkBtContent *bt_content)
{
	pr_info("%s thread tid = %lu\n", __func__, pthread_self());
	a2dp_source_clean();

	if (agent_option)
		auto_register_agent = g_strdup(agent_option);
	else
		auto_register_agent = g_strdup("");

	dbus_conn = g_dbus_setup_bus(DBUS_BUS_SYSTEM, NULL, NULL);
	g_dbus_attach_object_manager(dbus_conn);

	btsrc_client = g_dbus_client_new(dbus_conn, "org.bluez", "/org/bluez");
	if (NULL == btsrc_client) {
		pr_info("%s: btsrc_client init fail\n", __func__);
		g_free(auto_register_agent);
		dbus_connection_unref(dbus_conn);
		return;
	}

	init_avrcp_ctrl();

	if(bt_content) {
		g_bt_content = bt_content;
		gatt_init(bt_content);
	} else {
		g_bt_content = NULL;
	}

	g_dbus_client_set_connect_watch(btsrc_client, connect_handler, NULL);
	g_dbus_client_set_disconnect_watch(btsrc_client, disconnect_handler, NULL);
	g_dbus_client_set_signal_watch(btsrc_client, message_handler, NULL);
	g_dbus_client_set_proxy_handlers(btsrc_client, proxy_added, proxy_removed,
						  property_changed, NULL);

	BT_OPENED = 1;
	pr_info("%s: server start...\n", __func__);
	return;
}

static gboolean _bluetooth_open(void *user_data)
{
	RkBtContent *bt_content = user_data;

	bluetooth_open(bt_content);
	return FALSE;
}

extern GMainContext *Bluez_Context;
int bt_open(RkBtContent *bt_content)
{
	int confirm_cnt = 100;

	BT_OPENED = 0;
#ifdef DefGContext
	//_bluetooth_open(bt_content);
	g_idle_add(_bluetooth_open, bt_content);
#else
	GSource *source;
	source = g_idle_source_new();
	g_source_set_priority (source, G_PRIORITY_DEFAULT_IDLE);
	g_source_set_callback (source, _bluetooth_open, bt_content, NULL);
	g_source_attach (source, Bluez_Context);
	g_source_unref (source);
#endif

	while (confirm_cnt--) {
		if (BT_OPENED)
			return 0;

		usleep(50 * 1000);
	}

	return -1;
}

static gboolean _bluetooth_close(void *user_data)
{
	memset(&g_bt_scan_info, 0, sizeof(bt_scan_info_t));

	g_dbus_client_unref(btsrc_client);
	btsrc_client = NULL;

	gatt_cleanup();

	dbus_connection_unref(dbus_conn);
	dbus_conn = NULL;

	g_list_free_full(ctrl_list, proxy_leak);
	g_free(auto_register_agent);
	auto_register_agent = NULL;
	//a2dp_source_clean();

	BT_CLOSED = 1;
	pr_info("%s: server exit!\n", __func__);
	return FALSE;
}

void bt_close()
{
	int confirm_cnt = 100;

	BT_CLOSED = 0;
#ifdef DefGContext
	//_bluetooth_close(NULL);
	g_idle_add(_bluetooth_close, NULL);
#else
	GSource *source;

	source = g_idle_source_new();
	g_source_set_priority (source, G_PRIORITY_DEFAULT_IDLE);
	g_source_set_callback (source, _bluetooth_close, NULL, NULL);
	g_source_attach (source, Bluez_Context);
	g_source_unref (source);
#endif
}

static int a2dp_master_get_rssi(GDBusProxy *proxy)
{
	int retry_cnt = 5;
	DBusMessageIter iter;
	short rssi = DISTANCE_VAL_INVALID;

	while (retry_cnt--) {
		if (g_dbus_proxy_get_property(proxy, "RSSI", &iter) == FALSE) {
			usleep(10000); //10ms
			continue;
		}
		break;
	}

	if (retry_cnt >= 0)
		dbus_message_iter_get_basic(&iter, &rssi);

	return rssi;
}

static RK_BT_PLAYROLE_TYPE a2dp_master_get_playrole(GDBusProxy *proxy)
{
	int ret = PLAYROLE_TYPE_UNKNOWN;
	enum BT_Device_Class device_class;

	device_class = dist_dev_class(proxy);
	if (device_class == BT_SINK_DEVICE)
		ret = PLAYROLE_TYPE_SINK;
	else if (device_class == BT_SOURCE_DEVICE)
		ret = PLAYROLE_TYPE_SOURCE;

	return ret;
}

int a2dp_master_scan(void *arg, int len, RK_BT_SCAN_TYPE scan_type)
{
	BtScanParam *param = NULL;
	BtDeviceInfo *start = NULL;
	GDBusProxy *proxy;
	int ret = 0;
	int i;

	if (check_default_ctrl() == FALSE)
		return -1;

	param = (BtScanParam *)arg;
	if (len < sizeof(BtScanParam)) {
		pr_info("%s parameter error. BtScanParam setting is incorrect\n", __func__);
		return -1;
	}

	if(g_bt_scan_info.is_scaning) {
		pr_info("%s: devices discovering\n", __func__);
		return -1;
	}
	g_bt_scan_info.is_scaning = true;
	g_bt_scan_info.scan_type = scan_type;

	pr_info("=== scan on ===\n");
	cmd_scan("on");
	if (param->mseconds > 100) {
		pr_info("Waiting for Scan(%d ms)...\n", param->mseconds);
		usleep(param->mseconds * 1000);
	} else {
		pr_info("warning:%dms is too short, scan time is changed to 2s.\n",
			param->mseconds);
		usleep(2000 * 1000);
	}

	pr_info("=== scan off ===\n");
	cmd_scan("off");

	cmd_devices(param);
	pr_info("=== parse scan device (cnt:%d) ===\n", param->item_cnt);
	for (i = 0; i < param->item_cnt; i++) {
		start = &param->devices[i];
		proxy = find_device_by_address(start->address);
		if (!proxy) {
			pr_info("%s find_device_by_address failed!\n", __func__);
			continue;
		}
		/* Get bluetooth rssi */
		ret = a2dp_master_get_rssi(proxy);
		if (ret != DISTANCE_VAL_INVALID) {
			start->rssi = ret;
			start->rssi_valid = TRUE;
		}
		/* Get bluetooth AudioProfile */
		ret = a2dp_master_get_playrole(proxy);
		if (ret == PLAYROLE_TYPE_SINK)
			memcpy(start->playrole, "Audio Sink", strlen("Audio Sink"));
		else if (ret == PLAYROLE_TYPE_SOURCE)
			memcpy(start->playrole, "Audio Source", strlen("Audio Source"));
		else
			memcpy(start->playrole, "Unknow", strlen("Unknow"));
	}

	g_bt_scan_info.scan_type = SCAN_TYPE_AUTO;
	filter_clear_transport();
	g_bt_scan_info.is_scaning = false;
	return 0;
}

static void *a2dp_master_connect_thread(void *arg)
{
	GDBusProxy *proxy;
	char *address = (char *)arg;
	static struct ConnectContext_t ctx;
	ctx.proxy = NULL;
	ctx.address = NULL;
	pr_info("%s thread tid = %lu\n", __func__, pthread_self());

	if(bt_is_scaning())
		bt_cancel_discovery(RK_BT_DISC_STOPPED_BY_USER);

	if(!bt_source_is_open()) {
		pr_err("%s: bt source is not open\n", __func__);
		a2dp_master_event_send(BT_SOURCE_EVENT_CONNECT_FAILED, address, address);
		return NULL;
	}

	proxy = find_proxy_by_address(default_ctrl->devices, address);
	if (!proxy) {
		pr_info("%s: Device %s not available\n", __func__, address);
		a2dp_master_event_send(BT_SOURCE_EVENT_CONNECT_FAILED, address, address);
		return NULL;
	}

	g_device_state = BT_SOURCE_EVENT_DISCONNECTED;
	a2dp_master_event_send(BT_SOURCE_EVENT_CONNECTTING, address, NULL);
	ctx.proxy = proxy;
	ctx.address = address;
	if (g_dbus_proxy_method_call(proxy, "Connect", NULL, connect_reply,
							&ctx, NULL) == FALSE) {
		//char name[256];
		//memset(name, 0, 256);
		//bt_get_device_name_by_proxy(proxy, name, 256);

		pr_info("%s: Failed to connect\n", __func__);
		a2dp_master_event_send(BT_SOURCE_EVENT_CONNECT_FAILED, address, address);
		return NULL;
	}

	g_bt_source_info.is_connecting = true;
	pr_info("%s: Attempting to connect to %s [proxy: %p]\n", __func__, address, proxy);
	return NULL;
}

int a2dp_master_connect(char *t_address)
{
	pthread_t tid;
	int addr_len;

	if (!t_address || (strlen(t_address) < 17)) {
		pr_err("%s: Invalid address\n", __func__);
		a2dp_master_event_send(BT_SOURCE_EVENT_CONNECT_FAILED, t_address, t_address);
		return -1;
	}

	if (check_default_ctrl() == FALSE) {
		a2dp_master_event_send(BT_SOURCE_EVENT_CONNECT_FAILED, t_address, t_address);
		return -1;
	}

	memset(g_bt_source_info.connect_address, 0, sizeof(g_bt_source_info.connect_address));
	addr_len = sizeof(g_bt_source_info.connect_address) > strlen(t_address) ?
		strlen(t_address) : sizeof(g_bt_source_info.connect_address);
	memcpy(g_bt_source_info.connect_address, t_address, addr_len);

	if (pthread_create(&tid, NULL, a2dp_master_connect_thread, (void *)g_bt_source_info.connect_address)) {
		pr_err("%s: source connect thread create failed!\n", __func__);
		a2dp_master_event_send(BT_SOURCE_EVENT_CONNECT_FAILED, t_address, t_address);
		return -1;
	}

	pthread_detach(tid);
	return 0;
}

void ble_disconn_reply(DBusMessage *message, void *user_data)
{
	GDBusProxy *proxy = (GDBusProxy *)user_data;
	DBusError error;

	dbus_error_init(&error);

	if (dbus_set_error_from_message(&error, message) == TRUE) {
		pr_info("Failed to disconnect: %s\n", error.name);
		dbus_error_free(&error);
		return;
	}

	if (proxy == ble_dev) {
		pr_info("%s: Successful disconnected ble\n", __func__);
	} else {
		pr_info("%s: Failed disconnected ble\n", __func__);
	}
}

int ble_disconnect()
{
	if (!ble_dev) {
		pr_info("%s: ble no connect\n", __func__);
		return -1;
	}

	if(ble_is_open()) {
		if(!remove_ble_device())
			return;
	}

	if (g_dbus_proxy_method_call(ble_dev, "Disconnect", NULL, ble_disconn_reply,
							ble_dev, NULL) == FALSE) {
		pr_info("%s: Failed to disconnect\n", __func__);
		return -1;
	}

	pr_info("%s: Attempting to disconnect ble from %s\n", __func__, proxy_address(ble_dev));
	return 0;
}

int remove_ble_device()
{
	DBusMessageIter iter;
	const char *type = NULL;

	if (!ble_dev) {
		pr_info("%s: ble no connect\n", __func__);
		return -1;
	}

	if (g_dbus_proxy_get_property(ble_dev, "AddressType", &iter)) {
		dbus_message_iter_get_basic(&iter, &type);
		pr_info("%s: AddressType = %s\n", __func__, type);
		if (!strcmp(type, "public")) {
			return remove_device(ble_dev);
		}
	}

	return -1;
}

void ble_clean()
{
	ble_service_cnt = 0;
	ble_dev = NULL;
}

static int disconnect_by_proxy(GDBusProxy *proxy)
{
	if (!proxy) {
		pr_info("%s: Invalid proxy\n", __func__);
		return -1;
	}

	if (g_dbus_proxy_method_call(proxy, "Disconnect", NULL, disconn_reply,
							proxy, NULL) == FALSE) {
		pr_info("Failed to disconnect\n");
		return -1;
	}

	pr_info("%s: Attempting to disconnect from %s\n", __func__, proxy_address(proxy));
	return 0;
}

/*
 * Get the Bluetooth connection status.
 * Input parameters:
 *     Addr_buff -> if not empty, the interface will resolve the address
 *     of the current connection and store it in addr_buf.
 * return value:
 *    0-> not connected;
 *    1-> is connected;
 */
int a2dp_master_status(char *addr_buf, int addr_len, char *name_buf, int name_len)
{
	DBusMessageIter iter;
	const char *address;
	const char *name;

	if (!default_dev) {
		pr_info("no source connect\n");
		return 0;
	}

	if (addr_buf) {
		if (g_dbus_proxy_get_property(default_dev, "Address", &iter) == FALSE) {
			pr_info("WARING: Bluetooth connected, but can't get address!\n");
			return 0;
		}
		dbus_message_iter_get_basic(&iter, &address);
		memset(addr_buf, 0, addr_len);
		memcpy(addr_buf, address, (strlen(address) > addr_len) ? addr_len : strlen(address));
	}

	if (name_buf) {
		if (g_dbus_proxy_get_property(default_dev, "Alias", &iter) == FALSE) {
			pr_info("WARING: Bluetooth connected, but can't get device name!\n");
			return 0;
		}

		dbus_message_iter_get_basic(&iter, &name);
		memset(name_buf, 0, name_len);
		memcpy(name_buf, name, (strlen(name) > name_len) ? name_len : strlen(name));
	}

	return 1;
}

int remove_by_address(char *t_address)
{
	GDBusProxy *proxy;

	if(t_address == NULL) {
		pr_err("%s: Invalid address\n", __func__);
		return -1;
	}

	if (check_default_ctrl() == FALSE)
		return -1;

	if (strcmp(t_address, "*") == 0) {
		GList *list;

		for (list = default_ctrl->devices; list; list = g_list_next(list)) {
			proxy = (GDBusProxy *)list->data;
			remove_device(proxy);
		}
		return 0;
	} else if ((strlen(t_address) < 17)) {
		pr_err("%s: %s address error!\n", __func__, t_address);
		return -1;
	}

	proxy = find_proxy_by_address(default_ctrl->devices, t_address);
	if (!proxy) {
		pr_info("Device %s not available\n", t_address);
		return -1;
	}

	return remove_device(proxy);
}

int a2dp_master_save_status(char *address)
{
	char buff[100] = {0};
	struct sockaddr_un serverAddr;
	int snd_cnt = 3;
	int sockfd;
	int send_len = 0;

	sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		pr_info("FUNC:%s create sockfd failed!\n", __func__);
		return 0;
	}

	serverAddr.sun_family = AF_UNIX;
	strcpy(serverAddr.sun_path, "/tmp/a2dp_master_status");

	memset(buff, 0, sizeof(buff));
	if (address)
		sprintf(buff, "status:connect;address:%s;", address);
	else
		sprintf(buff, "status:disconnect;");

	while(snd_cnt--) {
		send_len = sendto(sockfd, buff, strlen(buff), MSG_DONTWAIT, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
		if(send_len == strlen(buff)) {
			pr_info("%s: send: %s(%d)\n", __func__, buff, send_len);
			break;
		}

		usleep(1000);
	}

	close(sockfd);
	return 0;
}

void a2dp_master_event_send(RK_BT_SOURCE_EVENT event, char *dev_addr, char *dev_name)
{
	if(!g_bt_callback.bt_source_event_cb || !bt_source_is_open())
		return;

	if ((event == BT_SOURCE_EVENT_CONNECTED) || (event == BT_SOURCE_EVENT_DISCONNECTED))
		g_device_state = event;

	pr_info("%s: state: %d, addr: %s, name: %s.\n", __func__, event, dev_addr, dev_name);

	if(!dev_addr && !dev_name) {
		char addr[18], name[256];

		memset(addr, 0, 18);
		memset(name, 0, 256);
		bt_get_device_addr_by_proxy(default_dev, addr, 18);
		bt_get_device_name_by_proxy(default_dev, name, 256);

		g_bt_callback.bt_source_event_cb(g_btmaster_userdata, addr, name, event);
	} else {
		g_bt_callback.bt_source_event_cb(g_btmaster_userdata, dev_addr, dev_name, event);
	}
}

void a2dp_master_register_cb(void *userdata, RK_BT_SOURCE_CALLBACK cb)
{
	g_bt_callback.bt_source_event_cb = cb;
	g_btmaster_userdata = userdata;
	return;
}

void a2dp_master_deregister_cb()
{
	g_bt_callback.bt_source_event_cb = NULL;
	g_btmaster_userdata = NULL;
	return;
}

/**********************************************
 *      bt source avrcp
 **********************************************/
static void save_last_device(GDBusProxy *proxy)
{
	int fd;
	const char *object_path, *address;
	DBusMessageIter iter, class_iter;
	dbus_uint32_t valu32;
	char buff[512] = {0};

	pr_info("%s\n", __func__);

	if (g_dbus_proxy_get_property(proxy, "Address", &iter) == FALSE) {
		pr_info("Get adapter address error");
		return;
	}

	dbus_message_iter_get_basic(&iter, &address);
	pr_info("Connected device address: %s\n", address);

	if (g_dbus_proxy_get_property(proxy, "Class", &class_iter) == FALSE) {
		pr_info("Get adapter Class error\n");
		return;
	}

	dbus_message_iter_get_basic(&class_iter, &valu32);
	pr_info("Connected device class: 0x%x\n", valu32);

	object_path = g_dbus_proxy_get_path(proxy);
	pr_info("Connected device object path: %s\n", object_path);

	fd = open(BT_RECONNECT_CFG, O_RDWR | O_CREAT | O_TRUNC, 0644);
	if (fd == -1) {
		pr_info("Open %s error: %s\n", BT_RECONNECT_CFG, strerror(errno));
		return;
	}

	sprintf(buff, "ADDRESS:%s;CLASS:%x;PATH:%s;", address, valu32, object_path);
	write(fd, buff, strlen(buff));
	fsync(fd);
	close(fd);
}

static int load_last_device(char *address)
{
	int fd, ret, i;
	char buff[512] = {0};
	char *start = NULL, *end = NULL;

	pr_info("Load path %s\n", BT_RECONNECT_CFG);

	ret = access(BT_RECONNECT_CFG, F_OK);
	if (ret == -1) {
		pr_info("%s does not exist\n", BT_RECONNECT_CFG);
		return -1;
	}

	fd = open(BT_RECONNECT_CFG, O_RDONLY);
	if (fd == -1) {
		pr_info("Open %s error: %s\n", BT_RECONNECT_CFG, strerror(errno));
		return -1;
	}

	ret = read(fd, buff, sizeof(buff));
	if (ret < 0) {
		pr_info("read %s error: %s\n", BT_RECONNECT_CFG, strerror(errno));
		close(fd);
		return -1;
	}

	start = strstr(buff, "ADDRESS:");
	end = strstr(buff, ";");
	if (!start || !end || (end < start)) {
		pr_info("file %s content invalid(address): %s\n", BT_RECONNECT_CFG, buff);
		close(fd);
		return -1;
	}
	start += strlen("ADDRESS:");
	if (address)
		memcpy(address, start, end - start);

	close(fd);
	return 0;
}

static void reconn_last_device_reply(DBusMessage * message, void *user_data)
{
	DBusError err;

	dbus_error_init(&err);
	if (dbus_set_error_from_message(&err, message) == TRUE) {
		pr_info("Reconnect failed!\n");
		dbus_error_free(&err);
	}
}

int reconn_last_devices(BtDeviceType type)
{
	GDBusProxy *proxy;
	DBusMessageIter addr_iter, addrType_iter;
	int fd, ret, reconnect = 1;
	char buff[100] = {0};
	char address[48] = {0};
	enum BT_Device_Class device_class;

	fd = open("/userdata/cfg/bt_reconnect", O_RDONLY);
	if (fd > 0) {
		ret = read(fd, buff, sizeof(buff));
		if (ret > 0) {
			if (strstr(buff, "bluez-reconnect:disable"))
				reconnect = 0;
		}
		close(fd);
	}

	if (reconnect == 0) {
		pr_info("%s: automatic reconnection is disabled!\n", __func__);
		return 0;
	}

	if (bt_is_connected()) {
		pr_info("%s: The device is connected and does not need to be reconnected!\n", __func__);
		return 0;
	}

	ret = load_last_device(address);
	if (ret < 0)
		return ret;

	proxy = find_device_by_address(address);
	if (!proxy) {
		pr_info("Invalid proxy, stop reconnecting\n");
		return -1;
	}

	device_class = dist_dev_class(proxy);
	if (device_class == BT_IDLE) {
		pr_info("Invalid device_class, stop reconnecting\n");
		return -1;
	}

	if(type == BT_DEVICES_A2DP_SINK) {
		pr_info("%s: source reconnected(%s)\n", __func__, address);
		memset(g_bt_source_info.reconnect_address, 0, sizeof(g_bt_source_info.reconnect_address));
		memcpy(g_bt_source_info.reconnect_address, address, sizeof(g_bt_source_info.reconnect_address));
		source_set_reconnect_tag(true);
		return 0;
	}

	switch(type) {
		case BT_DEVICES_A2DP_SINK:
			if (device_class != BT_SINK_DEVICE)
				reconnect = 0;
			break;
		case BT_DEVICES_A2DP_SOURCE:
			if (device_class != BT_SOURCE_DEVICE)
				reconnect = 0;
			break;
		case BT_DEVICES_BLE:
			if (device_class != BT_BLE_DEVICE)
				reconnect = 0;
			break;
		case BT_DEVICES_HFP:
			if (device_class != BT_SOURCE_DEVICE)
				reconnect = 0;
			break;
		case BT_DEVICES_SPP:
			break;
		default:
			reconnect = 0;
	}

	if (reconnect == 0) {
		pr_info("Unable to find a suitable reconnect device!\n");
		return -1;
	}

	if(type == BT_DEVICES_A2DP_SINK) {
		a2dp_master_connect(address);
	} else {
		if(bt_is_scaning())
			bt_cancel_discovery(RK_BT_DISC_STOPPED_BY_USER);

		if (g_dbus_proxy_method_call(proxy, "Connect", NULL,
			reconn_last_device_reply, NULL, NULL) == FALSE) {
			pr_info("Failed to call org.bluez.Device1.Connect\n");
			return -1;
		}
	}

	return 0;
}

int disconnect_current_devices()
{
	if (!default_dev) {
		pr_info("%s: No connected device\n", __func__);
		return -1;
	}

	return disconnect_by_proxy(default_dev);
}

int get_dev_platform(char *address)
{
	int vendor = -1, platform = DEV_PLATFORM_UNKNOWN;
	char *str;
	const char *valstr;
	GDBusProxy *proxy;
	DBusMessageIter iter;

	if(!address) {
		pr_info("%s: Invalid address\n", __func__);
		return DEV_PLATFORM_UNKNOWN;
	}

	proxy = find_device_by_address(address);
	if (!proxy) {
		pr_info("%s: Invalid proxy\n", __func__);
		return DEV_PLATFORM_UNKNOWN;
	}

	if (g_dbus_proxy_get_property(proxy, "Modalias", &iter) == FALSE) {
		pr_info("%s: WARING: can't get Modalias!\n", __func__);
		return DEV_PLATFORM_UNKNOWN;
	}

	dbus_message_iter_get_basic(&iter, &valstr);
	pr_info("%s: Modalias valstr = %s\n", __func__, valstr);

	str = strstr(valstr, "v");
	if(str) {
		if(!strncasecmp(str + 1, "004c", 4))
			vendor = IOS_VENDOR_SOURCE_BT;
		else if(!strncasecmp(str + 1, "05ac", 4))
			vendor = IOS_VENDOR_SOURCE_USB;
	}

	if(vendor == IOS_VENDOR_SOURCE_BT || vendor == IOS_VENDOR_SOURCE_USB)
		platform = DEV_PLATFORM_IOS;

	pr_info("%s: %s is %s\n", __func__, address,
		platform == DEV_PLATFORM_UNKNOWN ? "Unknown Platform" : "Apple IOS");

	return platform;
}

int get_current_dev_platform()
{
	if (!default_dev) {
		pr_info("%s: No connected device\n", __func__);
		return DEV_PLATFORM_UNKNOWN;
	}

	return get_dev_platform(proxy_address(default_dev));
}

static void connect_by_address_reply(DBusMessage *message, void *user_data)
{
	GDBusProxy *proxy = (GDBusProxy*)user_data;
	DBusError error;

	dbus_error_init(&error);

	if (dbus_set_error_from_message(&error, message) == TRUE) {
		pr_info("%s: Failed to connect: %s\n", __func__, error.name);
		dbus_error_free(&error);
		//set_default_device(NULL, NULL);
		return bt_shell_noninteractive_quit(EXIT_FAILURE);
	}

	pr_info("%s: Connection successful\n", __func__);
	//set_default_device(proxy, NULL);

	return bt_shell_noninteractive_quit(EXIT_SUCCESS);
}

int connect_by_address(char *addr)
{
	GDBusProxy *proxy;

	if (!addr || (strlen(addr) < 17)) {
		pr_err("%s: Invalid address\n", __func__);
		return -1;
	}

	proxy = find_device_by_address(addr);
	if (!proxy) {
		pr_info("%s: Invalid proxy\n", __func__);
		return -1;
	}

	if(bt_is_scaning())
		bt_cancel_discovery(RK_BT_DISC_STOPPED_BY_USER);

	if (g_dbus_proxy_method_call(proxy, "Connect", NULL,
		connect_by_address_reply, proxy, NULL) == FALSE) {
		pr_info("%s: Failed to call org.bluez.Device1.Connect\n", __func__);
		return -1;
	}

	pr_info("%s: Attempting to connect to %s\n", __func__, addr);
	return 0;
}

int disconnect_by_address(char *addr)
{
	GDBusProxy *proxy;

	if (!addr || (strlen(addr) < 17)) {
		pr_err("%s: Invalid address\n", __func__);
		return -1;
	}

	proxy = find_device_by_address(addr);
	if (!proxy) {
		pr_info("%s: Invalid proxy\n", __func__);
		return -1;
	}

	return disconnect_by_proxy(proxy);
}

void bt_display_devices()
{
	cmd_devices(NULL);
}

void bt_display_paired_devices()
{
	cmd_paired_devices();
}

RkBtScanedDevice *bt_create_one_scaned_dev(GDBusProxy *proxy)
{
	DBusMessageIter iter;
	const char *address, *name;
	dbus_int16_t rssi = -100;
	dbus_bool_t is_connected = FALSE;
	dbus_uint32_t cod = 0;

	RkBtScanedDevice *new_device = (RkBtScanedDevice*)malloc(sizeof(RkBtScanedDevice));
	if(!new_device) {
		pr_err("%s: malloc one scaned device failed\n", __func__);
		return NULL;
	}

	if (g_dbus_proxy_get_property(proxy, "Address", &iter))
		dbus_message_iter_get_basic(&iter, &address);
	else
		address = "<unknown>";
	pr_info("	addr: %s\n", address);

	if (g_dbus_proxy_get_property(proxy, "Alias", &iter))
		dbus_message_iter_get_basic(&iter, &name);
	else
		name = "<unknown>";
	pr_info("	name: %s\n", name);

	if (g_dbus_proxy_get_property(proxy, "Connected", &iter))
		dbus_message_iter_get_basic(&iter, &is_connected);
	else
		pr_info("%s: Can't get connected status\n", __func__);
	pr_info("	Connected: %d\n", is_connected);

	if(g_dbus_proxy_get_property(proxy, "Class", &iter))
		dbus_message_iter_get_basic(&iter, &cod);
	else
		pr_info("%s: Can't get class of device\n", __func__);
	pr_info("	Class: 0x%x\n", cod);

	new_device->remote_address = (char *)malloc(strlen(address) + 1);
	if(!new_device->remote_address) {
		pr_err("%s: malloc remote_address failed\n", __func__);
		return NULL;
	}
	strncpy(new_device->remote_address, address, strlen(address));
	new_device->remote_address[strlen(address)] = '\0';

	new_device->remote_name = (char *)malloc(strlen(name) + 1);
	if(!new_device->remote_name) {
		pr_err("%s: malloc remote_name failed\n", __func__);
		return NULL;
	}
	strncpy(new_device->remote_name, name, strlen(name));
	new_device->remote_name[strlen(name)] = '\0';

	new_device->is_connected = is_connected;
	new_device->cod = cod;
	new_device->next = NULL;

	return new_device;
}

static int list_scaned_dev_push_back(RkBtScanedDevice **dev_list, GDBusProxy *proxy)
{
	if(dev_list == NULL) {
		pr_info("%s: invalid dev_list\n", __func__);
		return -1;
	}

	if(*dev_list == NULL) {
		*dev_list = bt_create_one_scaned_dev(proxy);
		if(*dev_list == NULL)
			return -1;
	} else {
		RkBtScanedDevice *cur_dev = *dev_list;
		while(cur_dev->next != NULL)
			cur_dev = cur_dev->next;

		RkBtScanedDevice *new_dev = bt_create_one_scaned_dev(proxy);
		if(!new_dev)
			return -1;

		cur_dev->next = new_dev;
	}

	return 0;
}

int bt_get_scaned_devices(RkBtScanedDevice **dev_list, int *count, bool paired)
{
	GList *ll;

	*count = 0;
	if (check_default_ctrl() == FALSE)
		return -1;

	for (ll = g_list_first(default_ctrl->devices);
			ll; ll = g_list_next(ll)) {
		GDBusProxy *proxy = (GDBusProxy *)ll->data;

		if(paired) {
			DBusMessageIter iter;
			dbus_bool_t paired;

			if (g_dbus_proxy_get_property(proxy, "Paired", &iter) == FALSE)
				continue;

			dbus_message_iter_get_basic(&iter, &paired);
			if (!paired)
				continue;
		}
		if(!list_scaned_dev_push_back(dev_list, proxy))
			(*count)++;
	}

	pr_info("%s: paired = %d, count = %d\n", __func__, paired, *count);
	return 0;
}

int bt_free_scaned_devices(RkBtScanedDevice *dev_list)
{
	RkBtScanedDevice *dev_tmp = NULL;

	if(dev_list == NULL) {
		pr_info("%s: dev_list is null, don't need to clear\n", __func__);
		return -1;
	}

	while(dev_list->next != NULL) {
		pr_info("%s: free dev: %s\n", __func__, dev_list->remote_address);
		dev_tmp = dev_list->next;
		free(dev_list->remote_address);
		free(dev_list->remote_name);
		free(dev_list);
		dev_list = dev_tmp;
	}

	if(dev_list != NULL) {
		pr_info("%s: last free dev: %s\n", __func__, dev_list->remote_address);
		free(dev_list->remote_address);
		free(dev_list->remote_name);
		free(dev_list);
		dev_list = NULL;
	}

	return 0;
}

int pair_by_addr(char *addr)
{
	GDBusProxy *proxy;
	char dev_name[256];

	if (!addr || (strlen(addr) < 17)) {
		pr_err("%s: Invalid address\n", __func__);
		return -1;
	}

	proxy = find_device_by_address(addr);
	if (!proxy) {
		pr_info("%s: Invalid proxy\n", __func__);
		return -1;
	}

	bt_get_device_name_by_proxy(proxy, dev_name, 256);
	bt_bond_state_send(addr, dev_name, RK_BT_BOND_STATE_BONDING);

	return cmd_pair(proxy);
}

int unpair_by_addr(char *addr)
{
	GDBusProxy *proxy;

	if (!addr || (strlen(addr) < 17)) {
		pr_err("%s: Invalid address\n", __func__);
		return -1;
	}

	pr_info("%s\n", __func__);
	proxy = find_device_by_address(addr);
	if (!proxy) {
		pr_info("%s: Invalid proxy\n", __func__);
		return -1;
	}

	/* There is no direct unpair method, removing device will clear pairing information */
	return remove_device(proxy);
}

int bt_set_device_name(char *name)
{
	if (!name) {
		pr_info("%s: Invalid bt name: %s\n", __func__, name);
		return -1;
	}

	if (check_default_ctrl() == FALSE)
		return -1;

	if (!default_ctrl->proxy) {
		pr_info("%s: Invalid proxy\n", __func__);
		return -1;
	}

	if (g_dbus_proxy_set_property_basic(default_ctrl->proxy, "Alias",
					DBUS_TYPE_STRING, &name,
					generic_callback, name, NULL) == FALSE) {
		pr_info("%s: set Alias property error\n", __func__);
		return -1;
	}

	pr_info("%s: Attempting to set device name %s\n", __func__, name);
	return 0;
}

int bt_get_device_name_by_proxy(GDBusProxy *proxy,
			char *name_buf, int name_len)
{
	DBusMessageIter iter;
	const char *name;

	if (!proxy) {
		pr_info("%s: Invalid proxy\n", __func__);
		return -1;
	}

	if (!name_buf || name_len <= 0) {
		pr_info("%s: Invalid name buffer, name_len: %d\n", __func__, name_len);
		return -1;
	}

	memset(name_buf, 0, name_len);
	if (g_dbus_proxy_get_property(proxy, "Alias", &iter) == FALSE) {
		pr_info("WARING: Bluetooth connected, but can't get device name!\n");
		return -1;
	}

	dbus_message_iter_get_basic(&iter, &name);
	memcpy(name_buf, name, (strlen(name) > name_len) ? name_len : strlen(name));

	return 0;
}

int bt_get_device_name(char *name_buf, int name_len)
{
	if (check_default_ctrl() == FALSE)
		return -1;

	if (!default_ctrl->proxy) {
		pr_info("%s: Invalid proxy\n", __func__);
		return -1;
	}

	return bt_get_device_name_by_proxy(default_ctrl->proxy, name_buf, name_len);
}

int bt_get_device_addr_by_proxy(GDBusProxy *proxy,
			char *addr_buf, int addr_len)
{
	DBusMessageIter iter;
	const char *address;

	if (!proxy) {
		pr_info("%s: Invalid proxy\n", __func__);
		return -1;
	}

	if (!addr_buf || addr_len < 17) {
		pr_info("%s: Invalid address buffer, addr_len: %d\n", __func__, addr_len);
		return -1;
	}

	memset(addr_buf, 0, addr_len);
	if (g_dbus_proxy_get_property(proxy, "Address", &iter) == FALSE) {
		pr_info("WARING: Bluetooth connected, but can't get address!\n");
		return -1;
	}

	dbus_message_iter_get_basic(&iter, &address);
	memcpy(addr_buf, address, (strlen(address) > addr_len) ? addr_len : strlen(address));

	return 0;
}

int bt_get_device_addr(char *addr_buf, int addr_len)
{
	if (check_default_ctrl() == FALSE)
		return -1;

	if (!default_ctrl->proxy) {
		pr_info("%s: Invalid proxy\n", __func__);
		return -1;
	}

	return bt_get_device_addr_by_proxy(default_ctrl->proxy, addr_buf, addr_len);
}


int bt_get_default_dev_addr(char *addr_buf, int addr_len)
{
	if (!default_dev) {
		pr_info("%s: no connected device\n", __func__);
		return -1;
	}

	return bt_get_device_addr_by_proxy(default_dev, addr_buf, addr_len);
}

static void reomve_unpaired_device ()
{
	GDBusProxy *proxy;
	DBusMessageIter iter;
	GList *list;

	if (check_default_ctrl() == FALSE)
		return;

	for (list = default_ctrl->devices; list;
					list = g_list_next(list)) {
		dbus_bool_t paired = FALSE;
		dbus_bool_t connected = FALSE;

		GDBusProxy *proxy = (GDBusProxy *)list->data;

		if (g_dbus_proxy_get_property(proxy, "Paired", &iter))
			dbus_message_iter_get_basic(&iter, &paired);

		if (g_dbus_proxy_get_property(proxy, "Connected", &iter))
			dbus_message_iter_get_basic(&iter, &connected);

		if (paired || connected) {
			const char *address;
			if (g_dbus_proxy_get_property(proxy, "Address", &iter)) {
				dbus_message_iter_get_basic(&iter, &address);
				pr_info("%s: address(%s) is paired(%d) or connected(%d)\n", __func__, address, paired, connected);
			}
			continue;
		}

		remove_device(proxy);
	}

	return;
}

int bt_get_eir_data(char *address, char *eir_data, int eir_len)
{
	DBusMessageIter iter;
	DBusMessageIter array;
	unsigned char *data;
	int len, data_len = 0;
	struct GDBusProxy *proxy;

	if (!address || (strlen(address) < 17)) {
		pr_err("%s: invalid address\n", __func__);
		return -1;
	}

	if (!eir_data) {
		pr_err("%s: invalid eir_data buf\n", __func__);
		return -1;
	}

	proxy = find_device_by_address(address);
	if (proxy == NULL)
		return -1;

	if (g_dbus_proxy_get_property(proxy, "EirData", &iter) == FALSE) {
		pr_err("%s: get EirData data failed\n", __func__);
		return- 1;
	}

	if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_ARRAY) {
		pr_err("%s: iter type != DBUS_TYPE_ARRAY\n", __func__);
		return -1;
	}

	dbus_message_iter_recurse(&iter, &array);

	if (!dbus_type_is_fixed(dbus_message_iter_get_arg_type(&array))) {
		pr_err("%s: dbus type isn't fixed\n", __func__);
		return -1;
	}

	if (dbus_message_iter_get_arg_type(&array) != DBUS_TYPE_BYTE) {
		pr_err("%s: iter type != DBUS_TYPE_BYTE\n", __func__);
		return -1;
	}

	dbus_message_iter_get_fixed_array(&array, &data, &data_len);
	if (data_len <= 0) {
		pr_err("%s: get EirData data failed, len = %d\n", __func__, data_len);
		return -1;
	}

	pr_err("%s: get EirData data, data_len = %d\n", __func__, data_len);

	bt_shell_hexdump((void *)data, data_len * sizeof(*data));

	len = data_len > eir_len ? eir_len : data_len;
	memset(eir_data, 0, eir_len);
	memcpy(eir_data, data, len);

	return len;
}

int bt_start_discovery(unsigned int mseconds, RK_BT_SCAN_TYPE scan_type)
{
	int ret;

	if (bt_is_scaning()) {
		pr_info("%s: devices discovering\n", __func__);
		return -1;
	}

	g_bt_scan_info.is_scaning = true;
	g_bt_scan_info.scan_type = scan_type;

	reomve_unpaired_device();

	pr_info("=== scan on ===\n");
	//exec_command_system("hciconfig hci0 noscan");
	if(cmd_scan("on") < 0) {
		bt_discovery_state_send(RK_BT_DISC_START_FAILED);
	}

	return 0;
}

int bt_cancel_discovery(RK_BT_DISCOVERY_STATE state)
{
	int wait_cnt = 100;

	if (!g_bt_scan_info.is_scaning) {
		pr_info("%s: discovery canceling or canceled\n", __func__);
		return 0;
	}

	pr_info("%s thread tid = %lu\n", __func__, pthread_self());

	if (bt_is_discovering()) {
		pr_info("=== %s scan off again===\n", __func__);
		cmd_scan("off");
	}

	return 0;
}

bool bt_is_scaning()
{
	return bt_is_discovering() || g_bt_scan_info.is_scaning;
}

bool bt_is_discovering()
{
	DBusMessageIter iter;
	dbus_bool_t valbool;

	if (check_default_ctrl() == FALSE)
		return false;

	if (!default_ctrl->proxy) {
		pr_info("%s: Invalid proxy\n", __func__);
		return false;
	}

	if (g_dbus_proxy_get_property(default_ctrl->proxy, "Discovering", &iter) == FALSE) {
		pr_info("WARING: Bluetooth connected, but can't get Discovering!\n");
		return false;
	}

	dbus_message_iter_get_basic(&iter, &valbool);

	return valbool;
}

/*
 * / # hcitool con
 * Connections:
 *      > ACL 64:A2:F9:68:1E:7E handle 1 state 1 lm SLAVE AUTH ENCRYPT
 *      > LE 60:9C:59:31:7F:B9 handle 16 state 1 lm SLAVE
 */
bool bt_is_connected()
{
	bool ret = false;
	char buf[1024];

	memset(buf, 0, 1024);
	exec_command("hcitool con", buf, 1024);
	usleep(300000);

	if (strstr(buf, "ACL") || strstr(buf, "LE"))
		ret = true;

	return ret;
}

RK_BT_PLAYROLE_TYPE bt_get_playrole_by_addr(char *addr)
{
	GDBusProxy *proxy;

	if (!addr || (strlen(addr) < 17)) {
		pr_err("%s: Invalid address\n", __func__);
		return -1;
	}

	proxy = find_device_by_address(addr);
	if (!proxy) {
		pr_err("%s: Invalid proxy\n", __func__);
		return -1;
	}

	return a2dp_master_get_playrole(proxy);
}

void source_set_reconnect_tag(bool reconnect)
{
	g_bt_source_info.is_reconnected = reconnect;
}

void source_stop_connecting()
{
	if(g_bt_source_info.is_connecting) {
		pr_info("%s\n", __func__);
		if(!disconnect_by_address(g_bt_source_info.connect_address))
			sleep(3);
	}
}

bool get_device_connected_properties(char *addr)
{
	GDBusProxy *proxy;
	DBusMessageIter iter;
	dbus_bool_t is_connected = FALSE;

	if (!addr || (strlen(addr) < 17)) {
		pr_err("%s: Invalid address\n", __func__);
		return false;
	}

	proxy = find_device_by_address(addr);
	if (!proxy) {
		pr_info("%s: Invalid proxy\n", __func__);
		return false;
	}

	if (g_dbus_proxy_get_property(proxy, "Connected", &iter))
		dbus_message_iter_get_basic(&iter, &is_connected);
	else
		pr_info("%s: Can't get connected status\n", __func__);

	return is_connected;
}
