#include <stdio.h>
#include <errno.h>
#include <glib.h>
#include <unistd.h>
#include <stdlib.h>

#include "a2dp_source/a2dp_masterctrl.h"
#include "a2dp_source/gatt.h"
#include "utility.h"
#include "slog.h"
#include "gatt_client.h"

extern struct adapter *default_ctrl;
extern GDBusProxy *ble_dev;
extern GDBusProxy *default_attr;

typedef struct {
	RK_BLE_CLIENT_STATE_CALLBACK state_cb;
	RK_BLE_CLIENT_RECV_CALLBACK recv_cb;
	RK_BLE_CLIENT_STATE state;
} gatt_client_control_t;

static gatt_client_control_t g_gatt_client_ctl = {
	NULL, NULL, RK_BLE_CLIENT_STATE_IDLE,
};

void gatt_client_register_state_callback(RK_BLE_CLIENT_STATE_CALLBACK cb)
{
	g_gatt_client_ctl.state_cb = cb;
}

void gatt_client_register_recv_callback(RK_BLE_CLIENT_RECV_CALLBACK cb)
{
	g_gatt_client_ctl.recv_cb = cb;
}

void gatt_client_state_send(RK_BLE_CLIENT_STATE state)
{
	char addr[18], name[256];

	g_gatt_client_ctl.state = state;

	if(!g_gatt_client_ctl.state_cb) {
		pr_info("%s: ble_client_state_cb are not registered\n", __func__);
		return;
	}

	memset(addr, 0, 18);
	memset(name, 0, 256);
	bt_get_device_addr_by_proxy(ble_dev, addr, 18);
	bt_get_device_name_by_proxy(ble_dev, name, 256);
	g_gatt_client_ctl.state_cb(addr, name, state);
}

void gatt_client_recv_data_send(GDBusProxy *proxy, DBusMessageIter *iter)
{
	DBusMessageIter array, uuid_iter;
	const char *uuid;
	uint8_t *value;
	int len;

	if (dbus_message_iter_get_arg_type(iter) != DBUS_TYPE_ARRAY) {
		pr_info("%s: Unable to get value\n", __func__);
		return;
	}

	dbus_message_iter_recurse(iter, &array);
	dbus_message_iter_get_fixed_array(&array, &value, &len);

	if (len < 0) {
		pr_info("%s: Unable to parse value\n", __func__);
		return;
	}

	if (g_dbus_proxy_get_property(proxy, "UUID", &uuid_iter) == FALSE)
		return;

	dbus_message_iter_get_basic(&uuid_iter, &uuid);

	if (g_gatt_client_ctl.recv_cb)
		g_gatt_client_ctl.recv_cb(uuid, value, len);
}

RK_BLE_CLIENT_STATE gatt_client_get_state()
{
	return g_gatt_client_ctl.state;
}

void gatt_client_open()
{
	ble_clean();
}

void gatt_client_close()
{
	pr_info("%s\n", __func__);
	ble_deregister_mtu_callback();
	g_gatt_client_ctl.state = RK_BLE_CLIENT_STATE_IDLE;
	g_gatt_client_ctl.recv_cb = NULL;
	g_gatt_client_ctl.state_cb = NULL;
}

int gatt_client_get_service_info(char *address, RK_BLE_CLIENT_SERVICE_INFO *info)
{
	GDBusProxy *proxy;

	if(!address || !info) {
		pr_err("%s: Invalid parameters\n", __func__);
		return -1;
	}

	proxy = find_device_by_address(address);
	if (!proxy) {
		pr_info("%s: can't find device(%s)\n", __func__, address);
		return -1;
	}

	memset(info, 0, sizeof(RK_BLE_CLIENT_SERVICE_INFO));
	gatt_get_list_attributes(g_dbus_proxy_get_path(proxy), info);
	return 0;
}

static int gatt_client_select_attribute(char *uuid)
{
	GDBusProxy *proxy;

	if (!ble_dev) {
		pr_info("%s: No ble client connected\n", __func__);
		return -1;
	}

	proxy = gatt_select_attribute(NULL, uuid);
	if (proxy) {
		set_default_attribute(proxy);
		return 0;
	}

	return -1;
}

int gatt_client_read(char *uuid, int offset)
{
	if(!uuid) {
		pr_err("%s: Invalid uuid\n", __func__);
		return -1;
	}

	if(gatt_client_select_attribute(uuid) < 0) {
		pr_err("%s: select attribute failed\n", __func__);
		return -1;
	}

	return gatt_read_attribute(default_attr, offset);
}

int gatt_client_write(char *uuid, char *data, int data_len, int offset)
{
	if(!uuid || !data) {
		pr_err("%s: Invalid uuid or data\n", __func__);
		return -1;
	}

	if(gatt_client_select_attribute(uuid) < 0) {
		pr_err("%s: select attribute failed\n", __func__);
		return -1;
	}

	return gatt_write_attribute(default_attr, data, data_len, offset);
}

bool gatt_client_is_notifying(const char *uuid)
{
	if(!uuid) {
		pr_err("%s: Invalid uuid\n", __func__);
		return -1;
	}

	if(gatt_client_select_attribute(uuid) < 0) {
		pr_err("%s: select attribute failed\n", __func__);
		return -1;
	}

	return gatt_get_notifying(default_attr);
}

int gatt_client_notify(const char *uuid, bool enable)
{
	if(!uuid) {
		pr_err("%s: Invalid uuid\n", __func__);
		return -1;
	}

	if(gatt_client_select_attribute(uuid) < 0) {
		pr_err("%s: select attribute failed\n", __func__);
		return -1;
	}

	return gatt_notify_attribute(default_attr, enable ? true : false);
}

int gatt_client_get_eir_data(char *address, char *eir_data, int eir_len)
{
	DBusMessageIter iter;
	DBusMessageIter array;
	unsigned char *data;
	int len, data_len = 0;
	struct GDBusProxy *proxy;

	if(!address || (strlen(address) < 17)) {
		pr_err("%s: invalid address\n", __func__);
		return -1;
	}

	if(!eir_data || eir_len <= 0) {
		pr_err("%s: invalid eir_data buf, len = %d\n", __func__, len);
		return -1;
	}

	proxy = find_device_by_address(address);
	if(proxy == NULL)
		return -1;

	if (g_dbus_proxy_get_property(proxy, "EirData", &iter) == FALSE) {
		pr_err("%s: get broadcast data failed\n", __func__);
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

	if(dbus_message_iter_get_arg_type(&array) != DBUS_TYPE_BYTE) {
		pr_err("%s: iter type != DBUS_TYPE_BYTE\n", __func__);
		return -1;
	}

	dbus_message_iter_get_fixed_array(&array, &data, &data_len);
	if (data_len <= 0) {
		pr_err("%s: get broadcast data failed, len = %d\n", __func__, data_len);
		return -1;
	}

	pr_err("%s: get broadcast data, data_len = %d\n", __func__, data_len);

	bt_shell_hexdump((void *)data, data_len * sizeof(*data));

	len = data_len > eir_len ? eir_len : data_len;
	memset(eir_data, 0, eir_len);
	memcpy(eir_data, data, len);
	return 0;
}