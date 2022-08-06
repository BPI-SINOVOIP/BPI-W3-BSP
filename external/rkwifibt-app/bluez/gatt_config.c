/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2014  Instituto Nokia de Tecnologia - INdT
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <arpa/inet.h>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <time.h>
#include <pthread.h>

#include <glib.h>
#include <dbus/dbus.h>
#include <RkBtBase.h>

#include "error.h"
#include "gdbus/gdbus.h"
#include "gatt_config.h"
#include "slog.h"

#define GATT_MGR_IFACE				"org.bluez.GattManager1"
#define GATT_SERVICE_IFACE			"org.bluez.GattService1"
#define GATT_CHR_IFACE				"org.bluez.GattCharacteristic1"
#define GATT_DESCRIPTOR_IFACE		"org.bluez.GattDescriptor1"

static char reconnect_path[66];
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
static volatile bool g_dis_adv_close_ble = false;
#define min(x, y) ((x) < (y) ? (x) : (y))

#define AD_FLAGS						0x1
#define AD_COMPLETE_128_SERVICE_UUID	0x7
#define AD_COMPLETE_LOCAL_NAME			0x9

typedef struct {
	uint8_t data[16];
} uuid128_t;

typedef struct {
	uint8_t data[6];
} mac_t;

struct AdvDataContent {
	uint8_t adv_length;
	uint8_t flag_length;
	uint8_t flag;
	uint8_t flag_value;
	uint8_t service_uuid_length;
	uint8_t service_uuid_flag;
	uuid128_t service_uuid_value;
};

struct AdvRespDataContent {
	uint8_t adv_resp_length;
	uint8_t local_name_length;
	uint8_t local_name_flag;
	uint8_t local_name_value[29];
};

#define GATT_MAX_CHR 10
#define MAX_UUID_LEN 38
typedef struct BLE_CONTENT_T
{
	uint8_t advData[MXA_ADV_DATA_LEN];
	uint8_t advDataLen;
	uint8_t respData[MXA_ADV_DATA_LEN];
	uint8_t respDataLen;
	uint8_t server_uuid[MAX_UUID_LEN];
	uint8_t char_uuid[GATT_MAX_CHR][MAX_UUID_LEN];
	uint8_t char_cnt;
	void (*cb_ble_recv_fun)(const char *uuid, char *data, int len);
	void (*cb_ble_request_data)(const char *uuid);
} ble_content_t;

ble_content_t *ble_content_internal = NULL;
ble_content_t ble_content_internal_bak;
static int gid = 0;
static int gdesc_id = 0;
static int characteristic_id = 1;
static int service_id = 1;
static bool gatt_is_stopping = false;

static char g_cmd_ra[256];
static char g_cmd_para[256];

#define CMD_RA           "hcitool -i hci0 cmd 0x08 0x0005"

//first A0 00(0xA0): min interval, 0xA0 * 0.625ms, second A0 00(0xA0): max interval, 0xA0 * 0.625ms
#define CMD_PARA         "hcitool -i hci0 cmd 0x08 0x0006 A0 00 A0 00 00 01 00 00 00 00 00 00 00 07 00"
//#define CMD_PARA         "hcitool -i hci0 cmd 0x08 0x0006 A0 00 A0 00 00 02 00 00 00 00 00 00 00 07 00"

#define SERVICES_UUID    "23 20 56 7c 05 cf 6e b4 c3 41 77 28 51 82 7e 1b"
#define CMD_EN           "hcitool -i hci0 cmd 0x08 0x000a 1"
#define CMD_DISEN        "hcitool -i hci0 cmd 0x08 0x000a 0"

static GDBusProxy *ble_proxy = NULL;

struct adapter {
	GDBusProxy *proxy;
	GDBusProxy *ad_proxy;
	GList *devices;
};

extern GDBusProxy *ble_dev;
extern struct adapter *default_ctrl;
extern DBusConnection *dbus_conn;

struct characteristic {
	char *service;
	char *uuid;
	char *path;
	uint8_t *value;
	int vlen;
	const char **props;
};

struct characteristic *gchr[GATT_MAX_CHR];
struct descriptor *gdesc[GATT_MAX_CHR];
char *gservice_path = NULL;

struct descriptor {
	struct characteristic *chr;
	char *uuid;
	char *path;
	uint8_t *value;
	int vlen;
	const char **props;
};

int gatt_set_on_adv(void);

/*
 * Supported properties are defined at doc/gatt-api.txt. See "Flags"
 * property of the GattCharacteristic1.
 */
static const char *ias_alert_level_props[] = { "read", "write", NULL };
static const char *chr_props[] = { "read", "write", "notify", "indicate", "write-without-response", NULL };
static const char *desc_props[] = { "read", "write", NULL };

static void chr_write(struct characteristic *chr, const uint8_t *value, int len);
static void chr_iface_destroy(gpointer user_data);

static gboolean desc_get_uuid(const GDBusPropertyTable *property,
					DBusMessageIter *iter, void *user_data)
{
	struct descriptor *desc = user_data;

	dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &desc->uuid);

	return TRUE;
}

static gboolean desc_get_characteristic(const GDBusPropertyTable *property,
					DBusMessageIter *iter, void *user_data)
{
	struct descriptor *desc = user_data;

	dbus_message_iter_append_basic(iter, DBUS_TYPE_OBJECT_PATH,
						&desc->chr->path);

	return TRUE;
}

static bool desc_read(struct descriptor *desc, DBusMessageIter *iter)
{
	DBusMessageIter array;

	dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY,
					DBUS_TYPE_BYTE_AS_STRING, &array);

	if (desc->vlen && desc->value)
		dbus_message_iter_append_fixed_array(&array, DBUS_TYPE_BYTE,
						&desc->value, desc->vlen);

	dbus_message_iter_close_container(iter, &array);

	return true;
}

static gboolean desc_get_value(const GDBusPropertyTable *property,
					DBusMessageIter *iter, void *user_data)
{
	struct descriptor *desc = user_data;

	pr_info("Descriptor(%s): Get(\"Value\")\n", desc->uuid);

	return desc_read(desc, iter);
}

static void desc_write(struct descriptor *desc, const uint8_t *value, int len)
{
	g_free(desc->value);
	desc->value = g_memdup(value, len);
	desc->vlen = len;

	g_dbus_emit_property_changed(dbus_conn, desc->path,
					GATT_DESCRIPTOR_IFACE, "Value");
}

static int parse_value(DBusMessageIter *iter, const uint8_t **value, int *len)
{
	DBusMessageIter array;

	if (dbus_message_iter_get_arg_type(iter) != DBUS_TYPE_ARRAY)
		return -EINVAL;

	dbus_message_iter_recurse(iter, &array);
	dbus_message_iter_get_fixed_array(&array, value, len);

	return 0;
}

static void desc_set_value(const GDBusPropertyTable *property,
				DBusMessageIter *iter,
				GDBusPendingPropertySet id, void *user_data)
{
	struct descriptor *desc = user_data;
	const uint8_t *value;
	int len;

	pr_info("Descriptor(%s): Set(\"Value\", ...)\n", desc->uuid);

	if (parse_value(iter, &value, &len)) {
		pr_info("Invalid value for Set('Value'...)\n");
		g_dbus_pending_property_error(id,
					ERROR_INTERFACE ".InvalidArguments",
					"Invalid arguments in method call");
		return;
	}

	desc_write(desc, value, len);

	g_dbus_pending_property_success(id);
}

static gboolean desc_get_props(const GDBusPropertyTable *property,
					DBusMessageIter *iter, void *data)
{
	struct descriptor *desc = data;
	DBusMessageIter array;
	int i;

	dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY,
					DBUS_TYPE_STRING_AS_STRING, &array);

	for (i = 0; desc->props[i]; i++)
		dbus_message_iter_append_basic(&array,
					DBUS_TYPE_STRING, &desc->props[i]);

	dbus_message_iter_close_container(iter, &array);

	return TRUE;
}

static const GDBusPropertyTable desc_properties[] = {
	{ "UUID",		"s", desc_get_uuid },
	{ "Characteristic",	"o", desc_get_characteristic },
	{ "Value",		"ay", desc_get_value, desc_set_value, NULL },
	{ "Flags",		"as", desc_get_props, NULL, NULL },
	{ }
};

static gboolean chr_get_uuid(const GDBusPropertyTable *property,
					DBusMessageIter *iter, void *user_data)
{
	struct characteristic *chr = user_data;

	dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &chr->uuid);

	return TRUE;
}

static gboolean chr_get_service(const GDBusPropertyTable *property,
					DBusMessageIter *iter, void *user_data)
{
	struct characteristic *chr = user_data;

	dbus_message_iter_append_basic(iter, DBUS_TYPE_OBJECT_PATH,
							&chr->service);

	return TRUE;
}

static bool chr_read(struct characteristic *chr, DBusMessageIter *iter)
{
	DBusMessageIter array;

	dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY,
					DBUS_TYPE_BYTE_AS_STRING, &array);

	dbus_message_iter_append_fixed_array(&array, DBUS_TYPE_BYTE,
						&chr->value, chr->vlen);

	dbus_message_iter_close_container(iter, &array);

	return true;
}

static gboolean chr_get_value(const GDBusPropertyTable *property,
					DBusMessageIter *iter, void *user_data)
{
	struct characteristic *chr = user_data;

	pr_info("Characteristic(%s): Get(\"Value\")\n", chr->uuid);

	return chr_read(chr, iter);
}

static gboolean chr_get_props(const GDBusPropertyTable *property,
					DBusMessageIter *iter, void *data)
{
	struct characteristic *chr = data;
	DBusMessageIter array;
	int i;

	dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY,
					DBUS_TYPE_STRING_AS_STRING, &array);

	for (i = 0; chr->props[i]; i++)
		dbus_message_iter_append_basic(&array,
					DBUS_TYPE_STRING, &chr->props[i]);

	dbus_message_iter_close_container(iter, &array);

	return TRUE;
}

static void chr_write(struct characteristic *chr, const uint8_t *value, int len)
{
	g_free(chr->value);
	chr->value = g_memdup(value, len);
	chr->vlen = len;

	g_dbus_emit_property_changed(dbus_conn, chr->path, GATT_CHR_IFACE,
								"Value");
}

static void chr_set_value(const GDBusPropertyTable *property,
				DBusMessageIter *iter,
				GDBusPendingPropertySet id, void *user_data)
{
	struct characteristic *chr = user_data;
	const uint8_t *value;
	int len;

	pr_info("Characteristic(%s): Set('Value', ...)\n", chr->uuid);

	if (!parse_value(iter, &value, &len)) {
		pr_info("Invalid value for Set('Value'...)\n");
		g_dbus_pending_property_error(id,
					ERROR_INTERFACE ".InvalidArguments",
					"Invalid arguments in method call");
		return;
	}

	chr_write(chr, value, len);

	g_dbus_pending_property_success(id);
}

static const GDBusPropertyTable chr_properties[] = {
	{ "UUID",	"s", chr_get_uuid },
	{ "Service",	"o", chr_get_service },
	{ "Value",	"ay", chr_get_value, chr_set_value, NULL },
	{ "Flags",	"as", chr_get_props, NULL, NULL },
	{ }
};

static gboolean service_get_primary(const GDBusPropertyTable *property,
					DBusMessageIter *iter, void *user_data)
{
	dbus_bool_t primary = TRUE;

	pr_info("Get Primary: %s\n", primary ? "True" : "False");

	dbus_message_iter_append_basic(iter, DBUS_TYPE_BOOLEAN, &primary);

	return TRUE;
}

static gboolean service_get_uuid(const GDBusPropertyTable *property,
					DBusMessageIter *iter, void *user_data)
{
	const char *uuid = user_data;

	pr_info("Get UUID: %s\n", uuid);

	dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &uuid);

	return TRUE;
}

static const GDBusPropertyTable service_properties[] = {
	{ "Primary", "b", service_get_primary },
	{ "UUID", "s", service_get_uuid },
	{ }
};

static void chr_iface_destroy(gpointer user_data)
{
	struct characteristic *chr = user_data;

	pr_info("== %s ==\n", __func__);
	g_free(chr->uuid);
	g_free(chr->service);
	g_free(chr->value);
	g_free(chr->path);
	g_free(chr);
}

static void desc_iface_destroy(gpointer user_data)
{
	struct descriptor *desc = user_data;

	pr_info("== %s ==\n", __func__);
	g_free(desc->uuid);
	g_free(desc->value);
	g_free(desc->path);
	g_free(desc);
}

static int parse_options(DBusMessageIter *iter, const char **device)
{
	DBusMessageIter dict;

	if (dbus_message_iter_get_arg_type(iter) != DBUS_TYPE_ARRAY)
		return -EINVAL;

	dbus_message_iter_recurse(iter, &dict);

	while (dbus_message_iter_get_arg_type(&dict) == DBUS_TYPE_DICT_ENTRY) {
		const char *key;
		DBusMessageIter value, entry;
		int var;

		dbus_message_iter_recurse(&dict, &entry);
		dbus_message_iter_get_basic(&entry, &key);

		dbus_message_iter_next(&entry);
		dbus_message_iter_recurse(&entry, &value);

		var = dbus_message_iter_get_arg_type(&value);
		if (strcasecmp(key, "device") == 0) {
			if (var != DBUS_TYPE_OBJECT_PATH)
				return -EINVAL;
			dbus_message_iter_get_basic(&value, device);
			pr_info("Device: %s\n", *device);
		}

		dbus_message_iter_next(&dict);
	}

	return 0;
}

static void execute(const char cmdline[], char recv_buff[], int len)
{
	//pr_info("[GATT_CONFIG] execute: %s\n", cmdline);

	FILE *stream = NULL;
	char *tmp_buff = recv_buff;

	memset(recv_buff, 0, len);

	if ((stream = popen(cmdline, "r")) != NULL) {
		while (fgets(tmp_buff, len, stream)) {
			//pr_info("tmp_buf[%d]: %s\n", strlen(tmp_buff), tmp_buff);
			tmp_buff += strlen(tmp_buff);
			len -= strlen(tmp_buff);
			if (len <= 1)
				break;
		}
		//pr_info("[GATT_CONFIG] execute_r: %s \n", recv_buff);
		pclose(stream);
	}
}

static DBusMessage *chr_read_value(DBusConnection *conn, DBusMessage *msg,
							void *user_data)
{
	struct characteristic *chr = user_data;
	DBusMessage *reply;
	DBusMessageIter iter;
	const char *device;
	pr_info("=== chr_read_value enter ===\n");

	if (!dbus_message_iter_init(msg, &iter))
		return g_dbus_create_error(msg, DBUS_ERROR_INVALID_ARGS,
							"Invalid arguments");

	if (parse_options(&iter, &device))
		return g_dbus_create_error(msg, DBUS_ERROR_INVALID_ARGS,
							"Invalid arguments");

	reply = dbus_message_new_method_return(msg);
	if (!reply)
		return g_dbus_create_error(msg, DBUS_ERROR_NO_MEMORY,
							"No Memory");

	dbus_message_iter_init_append(reply, &iter);

	//an empty response
	chr->vlen = 0;
	chr_read(chr, &iter);

	if(ble_content_internal->cb_ble_request_data)
		ble_content_internal->cb_ble_request_data(chr->uuid);

	pr_info("=== chr_read_value exit ===\n");
	return reply;
}

static DBusMessage *chr_write_value(DBusConnection *conn, DBusMessage *msg,
							void *user_data)
{
	pr_info("=== chr_write_value enter ===\n");
	struct characteristic *chr = user_data;
	DBusMessageIter iter;
	const uint8_t *value;
	int len;
	const char *device;

	dbus_message_iter_init(msg, &iter);

	if (parse_value(&iter, &value, &len))
		return g_dbus_create_error(msg, DBUS_ERROR_INVALID_ARGS,
							"Invalid arguments");

	if (parse_options(&iter, &device))
		return g_dbus_create_error(msg, DBUS_ERROR_INVALID_ARGS,
							"Invalid arguments");

	chr_write(chr, value, len);
	if(len == 0 || chr->value == NULL) {
		pr_info("chr_write_value is null\n");
		return dbus_message_new_method_return(msg);
	}

	if (ble_content_internal->cb_ble_recv_fun) {
		ble_content_internal->cb_ble_recv_fun(chr->uuid, (char *)chr->value, len);
	} else {
		pr_info("cb_ble_recv_fun is null !!! \n");
	}

	pr_info("=== chr_write_value exit ===\n");
	return dbus_message_new_method_return(msg);
}

static DBusMessage *chr_start_notify(DBusConnection *conn, DBusMessage *msg,
							void *user_data)
{
	return g_dbus_create_error(msg, DBUS_ERROR_NOT_SUPPORTED,
							"Not Supported");
}

static DBusMessage *chr_stop_notify(DBusConnection *conn, DBusMessage *msg,
							void *user_data)
{
	return g_dbus_create_error(msg, DBUS_ERROR_NOT_SUPPORTED,
							"Not Supported");
}

static const GDBusMethodTable chr_methods[] = {
	{ GDBUS_ASYNC_METHOD("ReadValue", GDBUS_ARGS({ "options", "a{sv}" }),
					GDBUS_ARGS({ "value", "ay" }),
					chr_read_value) },
	{ GDBUS_ASYNC_METHOD("WriteValue", GDBUS_ARGS({ "value", "ay" },
						{ "options", "a{sv}" }),
					NULL, chr_write_value) },
	{ GDBUS_ASYNC_METHOD("StartNotify", NULL, NULL, chr_start_notify) },
	{ GDBUS_METHOD("StopNotify", NULL, NULL, chr_stop_notify) },
	{ }
};

static DBusMessage *desc_read_value(DBusConnection *conn, DBusMessage *msg,
							void *user_data)
{
	struct descriptor *desc = user_data;
	DBusMessage *reply;
	DBusMessageIter iter;
	const char *device;

	if (!dbus_message_iter_init(msg, &iter))
		return g_dbus_create_error(msg, DBUS_ERROR_INVALID_ARGS,
							"Invalid arguments");

	if (parse_options(&iter, &device))
		return g_dbus_create_error(msg, DBUS_ERROR_INVALID_ARGS,
							"Invalid arguments");

	reply = dbus_message_new_method_return(msg);
	if (!reply)
		return g_dbus_create_error(msg, DBUS_ERROR_NO_MEMORY,
							"No Memory");

	dbus_message_iter_init_append(reply, &iter);

	desc_read(desc, &iter);

	return reply;
}

static DBusMessage *desc_write_value(DBusConnection *conn, DBusMessage *msg,
							void *user_data)
{
	struct descriptor *desc = user_data;
	DBusMessageIter iter;
	const char *device;
	const uint8_t *value;
	int len;

	if (!dbus_message_iter_init(msg, &iter))
		return g_dbus_create_error(msg, DBUS_ERROR_INVALID_ARGS,
							"Invalid arguments");

	if (parse_value(&iter, &value, &len))
		return g_dbus_create_error(msg, DBUS_ERROR_INVALID_ARGS,
							"Invalid arguments");

	if (parse_options(&iter, &device))
		return g_dbus_create_error(msg, DBUS_ERROR_INVALID_ARGS,
							"Invalid arguments");

	desc_write(desc, value, len);

	return dbus_message_new_method_return(msg);
}

static const GDBusMethodTable desc_methods[] = {
	{ GDBUS_ASYNC_METHOD("ReadValue", GDBUS_ARGS({ "options", "a{sv}" }),
					GDBUS_ARGS({ "value", "ay" }),
					desc_read_value) },
	{ GDBUS_ASYNC_METHOD("WriteValue", GDBUS_ARGS({ "value", "ay" },
						{ "options", "a{sv}" }),
					NULL, desc_write_value) },
	{ }
};

static gboolean unregister_ble(void)
{
	int i;

	for (i = 0; i < gdesc_id; i++) {
		pr_info("%s: desc_uuid[%d]: %s, gdesc[%d]->path: %s\n", __func__, i, gdesc[i]->uuid, i, gdesc[i]->path);
		g_dbus_unregister_interface(dbus_conn, gdesc[i]->path, GATT_DESCRIPTOR_IFACE);
	}

	for (i = 0; i < gid; i++) {
		pr_info("%s: char_uuid[%d]: %s, gchr[%d]->path: %s\n", __func__, i, ble_content_internal->char_uuid[i], i, gchr[i]->path);
		g_dbus_unregister_interface(dbus_conn, gchr[i]->path, GATT_CHR_IFACE);
	}

	pr_info("%s: gservice_path: %s\n", __func__, gservice_path);
	g_dbus_unregister_interface(dbus_conn, gservice_path, GATT_SERVICE_IFACE);

	return TRUE;
}

static gboolean register_characteristic(const char *chr_uuid,
						const uint8_t *value, int vlen,
						const char **props,
						const char *desc_uuid,
						const char **desc_props,
						const char *service_path)
{
	struct characteristic *chr;
	struct descriptor *desc;

	chr = g_new0(struct characteristic, 1);
	chr->uuid = g_strdup(chr_uuid);
	chr->value = g_memdup(value, vlen);
	chr->vlen = vlen;
	chr->props = props;
	chr->service = g_strdup(service_path);
	chr->path = g_strdup_printf("%s/characteristic%d", service_path, characteristic_id++);
	pr_info("register_characteristic chr->uuid: %s, chr->path: %s\n", chr->uuid, chr->path);
	if (!g_dbus_register_interface(dbus_conn, chr->path, GATT_CHR_IFACE,
					chr_methods, NULL, chr_properties,
					chr, chr_iface_destroy)) {
		pr_info("Couldn't register characteristic interface\n");
		chr_iface_destroy(chr);
		return FALSE;
	}

	gchr[gid++] = chr;

	if (!desc_uuid)
		return TRUE;

	desc = g_new0(struct descriptor, 1);
	desc->uuid = g_strdup(desc_uuid);
	desc->chr = chr;
	desc->props = desc_props;
	desc->path = g_strdup_printf("%s/descriptor%d", chr->path, characteristic_id++);

	if (!g_dbus_register_interface(dbus_conn, desc->path,
					GATT_DESCRIPTOR_IFACE,
					desc_methods, NULL, desc_properties,
					desc, desc_iface_destroy)) {
		pr_info("Couldn't register descriptor interface\n");
		g_dbus_unregister_interface(dbus_conn, chr->path,
							GATT_CHR_IFACE);

		desc_iface_destroy(desc);
		return FALSE;
	}

	gdesc[gdesc_id++] = desc;
	return TRUE;
}

static char *register_service(const char *uuid)
{
	static int id = 1;
	char *path;

	path = g_strdup_printf("/service%d", service_id++);
	if (!g_dbus_register_interface(dbus_conn, path, GATT_SERVICE_IFACE,
				NULL, NULL, service_properties,
				g_strdup(uuid), g_free)) {
		pr_info("Couldn't register service interface\n");
		g_free(path);
		return NULL;
	}

	return path;
}

static void gatt_create_services(void)
{
	char *service_path;
	uint8_t level = ' ';
	int i;

	pr_info("server_uuid: %s\n", ble_content_internal->server_uuid);
	service_path = register_service(ble_content_internal->server_uuid);
	if (!service_path)
		return;

	gservice_path = service_path;

	for (i = 0; i < ble_content_internal->char_cnt; i++) {
		pr_info("char_uuid[%d]: %s\n", i, ble_content_internal->char_uuid[i]);
		gboolean mcharacteristic = register_characteristic(ble_content_internal->char_uuid[i],
							&level, sizeof(level),
							chr_props,
							NULL,
							desc_props,
							service_path);
		/* Add Alert Level Characteristic to Immediate Alert Service */
		if (!mcharacteristic) {
			pr_info("Couldn't register characteristic.\n");
			g_dbus_unregister_interface(dbus_conn, service_path,
								GATT_SERVICE_IFACE);
			g_free(service_path);
			return;
		}
	}

	pr_info("Registered service: %s\n", service_path);
}

int gatt_write_data(char *uuid, void *data, int len)
{
	int i;
	struct characteristic *chr;

	if (!ble_dev) {
		pr_info("gatt_write_data: ble not connect!\n");
		return 0;
	}

	pr_info("gatt_write uuid: %s, len: [%d], data[%p]: %s\n", uuid, len, data, (char *)data);

	if (!gchr[0])
		while(1);

	for (i = 0; i < gid; i++) {
		pr_info("gatt_write[%d] uuid: %s\n", i, gchr[i]->uuid);
		if (strcmp(gchr[i]->uuid, uuid) == 0) {
			chr = gchr[i];
			break;
		}
	}

	if (chr == NULL) {
		pr_info("gatt_write invaild uuid: %s.\n", uuid);
		return -1;
	}

	chr_write(chr, data, len);
	return 0;
}

int ble_enable_adv(void)
{
	char ret_buff[1024];

	if(gatt_set_on_adv() < 0) {
		pr_err("%s: gatt_set_on_adv failed\n", __func__);
		return -1;
	}

	execute(CMD_EN, ret_buff, 1024);
	return 0;
}

void ble_disable_adv(void)
{
	char ret_buff[1024];

	pr_info("=== ble_disable_adv ===\n");
	//g_dis_adv_close_ble = true;
	execute(CMD_DISEN, ret_buff, 1024);
	execute(CMD_DISEN, ret_buff, 1024);
}

int gatt_set_on_adv(void)
{
	char ret_buff[1024];
	char CMD_ADV_DATA[128] = "hcitool -i hci0 cmd 0x08 0x0008";
	char CMD_ADV_RESP_DATA[128] = "hcitool -i hci0 cmd 0x08 0x0009";
	char temp[32];
	int i;

	if(gatt_is_stopping) {
		pr_info("%s: ble is stopping\n", __func__);
		return -1;
	}

	if(!ble_content_internal) {
		pr_err("%s: ble_content_internal is NULL\n", __func__);
		return -1;
	}

	if (ble_content_internal->advDataLen <= 0) {
		pr_err("%s: invalid advDataLen = %d\n", __func__, ble_content_internal->advDataLen);
		return -1;
	}

	//LE Set Random Address Command
	execute(g_cmd_ra, ret_buff, 1024);
	pr_info("CMD_RA buff: %s", ret_buff);
	sleep(1);
	//LE SET PARAMETERS
	execute(g_cmd_para, ret_buff, 1024);
	pr_info("CMD_PARA buff: %s", ret_buff);

	// LE Set Advertising Data Command
	memset(temp, 0, 32);
	for(i = 0; i < ble_content_internal->advDataLen; i++) {
		sprintf(temp,"%02x", ble_content_internal->advData[i]);
		strcat(CMD_ADV_DATA, " ");
		strcat(CMD_ADV_DATA, temp);
	}
	pr_info("CMD_ADV_DATA: %s\n", CMD_ADV_DATA);
	execute(CMD_ADV_DATA, ret_buff, 1024);

	if(ble_content_internal->respDataLen > 0) {
		memset(temp, 0, 32);
		for (i = 0; i < ble_content_internal->respDataLen; i++) {
			sprintf(temp, "%02x", ble_content_internal->respData[i]);
			strcat(CMD_ADV_RESP_DATA, " ");
			strcat(CMD_ADV_RESP_DATA, temp);
		}
		usleep(500000);
		pr_info("CMD_ADV_RESP_DATA: %s\n", CMD_ADV_RESP_DATA);
		execute(CMD_ADV_RESP_DATA, ret_buff, 1024);
	}

	// LE Set Advertise Enable Command
	execute(CMD_EN, ret_buff, 1024);
	return 0;
}

static void register_app_reply(DBusMessage *reply, void *user_data)
{
	DBusError derr;

	dbus_error_init(&derr);
	dbus_set_error_from_message(&derr, reply);

	if (dbus_error_is_set(&derr))
		pr_info("RegisterApplication: %s\n", derr.message);
	else
		pr_info("RegisterApplication: OK\n");

	//send_advertise();
	//gatt_set_on_adv();

	dbus_error_free(&derr);
}

static void register_app_setup(DBusMessageIter *iter, void *user_data)
{
	const char *path = "/";
	DBusMessageIter dict;

	dbus_message_iter_append_basic(iter, DBUS_TYPE_OBJECT_PATH, &path);

	dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY, "{sv}", &dict);

	/* TODO: Add options dictionary */

	dbus_message_iter_close_container(iter, &dict);
}

void register_app(GDBusProxy *proxy)
{
	ble_proxy = proxy;

	if (!g_dbus_proxy_method_call(proxy, "RegisterApplication",
					register_app_setup, register_app_reply,
					NULL, NULL)) {
		pr_info("Unable to call RegisterApplication\n");
		return;
	}
}

static void unregister_app_reply(DBusMessage *message, void *user_data)
{
	DBusError error;

	dbus_error_init(&error);

	if (dbus_set_error_from_message(&error, message) == TRUE) {
		pr_info("Failed to unregister application: %s\n",
				error.name);
		dbus_error_free(&error);
		return;
	}

	pr_info("%s: Application unregistered\n", __func__);
}

static void unregister_app_setup(DBusMessageIter *iter, void *user_data)
{
	const char *path = "/";

	dbus_message_iter_append_basic(iter, DBUS_TYPE_OBJECT_PATH, &path);
}

void unregister_app(GDBusProxy *proxy)
{
	if (g_dbus_proxy_method_call(proxy, "UnregisterApplication",
						unregister_app_setup,
						unregister_app_reply, NULL,
						NULL) == FALSE) {
		pr_info("Failed unregister profile\n");
		return;
	}
}

int gatt_setup(void)
{
	pr_info("gatt_setup\n");
	gatt_create_services();
	register_app(ble_proxy);

	return 1;
}

void gatt_cleanup(void)
{
	if(ble_content_internal) {
		unregister_ble();
		ble_content_internal = NULL;
	}

	if(gservice_path) {
		g_free(gservice_path);
		gservice_path = NULL;
	}
}

#define HOSTNAME_MAX_LEN	250	/* 255 - 3 (FQDN) - 2 (DNS enc) */
static void bt_gethostname(char *hostname_buf)
{
	char hostname[HOSTNAME_MAX_LEN + 1];
	size_t buf_len;

	buf_len = sizeof(hostname);
	if (gethostname(hostname, buf_len) != 0)
		pr_info("gethostname error !!!!!!!!\n");
	hostname[buf_len - 1] = '\0';

	/* Deny sending of these local hostnames */
	if (hostname[0] == '\0' || hostname[0] == '.' || strcmp(hostname, "(none)") == 0)
		pr_info("gethostname format error !!!\n");
	else
		pr_info("gethostname: %s, len: %d \n", hostname, strlen(hostname));

	strcpy(hostname_buf, hostname);
}

static int bt_string_to_uuid128(uuid128_t *uuid, const char *string, int rever)
{
	uint32_t data0, data4;
	uint16_t data1, data2, data3, data5;
	uuid128_t u128;
	uint8_t *val = (uint8_t *) &u128;
	uint8_t tmp[16];

	if (sscanf(string, "%08x-%04hx-%04hx-%04hx-%08x%04hx",
				&data0, &data1, &data2,
				&data3, &data4, &data5) != 6)
		return -EINVAL;

	data0 = htonl(data0);
	data1 = htons(data1);
	data2 = htons(data2);
	data3 = htons(data3);
	data4 = htonl(data4);
	data5 = htons(data5);

	memcpy(&val[0], &data0, 4);
	memcpy(&val[4], &data1, 2);
	memcpy(&val[6], &data2, 2);
	memcpy(&val[8], &data3, 2);
	memcpy(&val[10], &data4, 4);
	memcpy(&val[14], &data5, 2);

	if (rever) {
		memcpy(tmp, val, 16);
		pr_info("UUID: ");
		for (int i = 0; i < 16; i++) {
			val[15 - i] = tmp[i];
			pr_info("0x%x ", tmp[i]);
		}
		pr_info("\n");
	}

	memset(uuid, 0, sizeof(uuid128_t));
	memcpy(uuid, &u128, sizeof(uuid128_t));
	return 0;
}

static int ble_adv_set(RkBtContent *bt_content, ble_content_t *ble_content)
{
	char hostname[HOSTNAME_MAX_LEN + 1];
	int i, name_len, uuid_len;
	struct AdvDataContent advdata;
	struct AdvRespDataContent advdataresp;
	uuid128_t uuid;

	if (bt_content->ble_content.advDataType == BLE_ADVDATA_TYPE_USER) {
		if (bt_content->ble_content.advDataLen <= 0) {
			pr_info("ERROR:Under the premise that advDataType is BLE_ADVDATA_TYPE_USER,"
				"the user must set the correct advData");
			return -1;
		}

		memcpy(ble_content->advData, bt_content->ble_content.advData, MXA_ADV_DATA_LEN);
		ble_content->advDataLen = bt_content->ble_content.advDataLen;
		if (bt_content->ble_content.respDataLen > 0) {
			memcpy(ble_content->respData, bt_content->ble_content.respData, MXA_ADV_DATA_LEN);
			ble_content->respDataLen = bt_content->ble_content.respDataLen;
		}
	} else {
		advdata.adv_length = 0x15;
		advdata.flag_length = 2;
		advdata.flag = AD_FLAGS;
		advdata.flag_value = 0x1a;
		advdata.service_uuid_length = 0x10 + 1;
		advdata.service_uuid_flag = AD_COMPLETE_128_SERVICE_UUID;

		if(bt_string_to_uuid128(&(advdata.service_uuid_value), bt_content->ble_content.server_uuid.uuid, 1) < 0) {
			pr_err("%s: bt_string_to_uuid128 failed\n", __func__);
			return -1;
		}

		ble_content->advDataLen = sizeof(struct AdvDataContent);
		memcpy(ble_content->advData, (uint8_t *)(&advdata), sizeof(struct AdvDataContent));

		//============================================================================
		if (bt_content->ble_content.ble_name) {
			name_len = strlen(bt_content->ble_content.ble_name);
			if (name_len > sizeof(advdataresp.local_name_value))
				name_len = sizeof(advdataresp.local_name_value);
			memcpy(advdataresp.local_name_value, bt_content->ble_content.ble_name, name_len);
			advdataresp.local_name_length = name_len + 1;
		} else {
			bt_gethostname(hostname);
			name_len = strlen(hostname);
			if (name_len > sizeof(advdataresp.local_name_value))
				name_len = sizeof(advdataresp.local_name_value);
			memcpy(advdataresp.local_name_value, hostname, name_len);
			advdataresp.local_name_length = name_len + 1;
		}
		advdataresp.local_name_flag = AD_COMPLETE_LOCAL_NAME;
		advdataresp.adv_resp_length = advdataresp.local_name_length + 1;

		ble_content->respDataLen = advdataresp.adv_resp_length + 1;
		memcpy(ble_content->respData, (uint8_t *)(&advdataresp), ble_content->respDataLen);
	}

	uuid_len = MAX_UUID_LEN > strlen(bt_content->ble_content.server_uuid.uuid) ? strlen(bt_content->ble_content.server_uuid.uuid) : MAX_UUID_LEN;
	memcpy(ble_content->server_uuid, bt_content->ble_content.server_uuid.uuid, uuid_len);

	/* set chr uuid */
	for (i = 0; i < bt_content->ble_content.chr_cnt; i++) {
		uuid_len = MAX_UUID_LEN > strlen(bt_content->ble_content.chr_uuid[i].uuid) ? strlen(bt_content->ble_content.chr_uuid[i].uuid) : MAX_UUID_LEN;
		memcpy(ble_content->char_uuid[i], bt_content->ble_content.chr_uuid[i].uuid, uuid_len);
	}

	ble_content->char_cnt = bt_content->ble_content.chr_cnt;
	ble_content->cb_ble_recv_fun = bt_content->ble_content.cb_ble_recv_fun;
	ble_content->cb_ble_request_data = bt_content->ble_content.cb_ble_request_data;
	return 0;
}

int gatt_init(RkBtContent *bt_content)
{
	int i;
	bool is_random_addr = true;
	uint8_t *ble_addr;
	uint8_t le_random_addr[DEVICE_ADDR_LEN];

	characteristic_id = 1;
	service_id = 1;
	gid = 0;
	gdesc_id = 0;
	gatt_is_stopping = false;

	if(bt_content->ble_content.server_uuid.len <= 0) {
		pr_info("%s: invalid server_uuid len = %d\n", __func__,
			bt_content->ble_content.server_uuid.len);
		return -1;
	}

	for(i = 0; i < DEVICE_ADDR_LEN; i++) {
		if(bt_content->ble_content.ble_addr[i] != 0) {
			is_random_addr = false;
			break;
		}
	}

	if(is_random_addr) {
		//random addr
		srand(time(NULL) + getpid() + getpid() * 987654 + rand());
		for(i = 0; i < DEVICE_ADDR_LEN;i++)
			le_random_addr[i] = rand() & 0xFF;

		//Clear two most significant bits
		le_random_addr[5] &= 0x3f;

		//Set second most significant bit, Private resolvable
		//le_random_addr[5] |= 0xc0;
		le_random_addr[5] |= 0x40;

		//Save random addr
		memcpy(bt_content->ble_content.ble_addr, le_random_addr, DEVICE_ADDR_LEN);
		ble_addr = le_random_addr;
	} else {
		ble_addr = bt_content->ble_content.ble_addr;
	}

	memset(g_cmd_ra, 0, 256);
	if(sprintf(g_cmd_ra, "%s %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx", CMD_RA,
			ble_addr[0], ble_addr[1], ble_addr[2],
			ble_addr[3], ble_addr[4], ble_addr[5]) < 0) {
		pr_err("%s: set ble address failed\n", __func__);
		return -1;
	}
	pr_info("CMD_RA: %d, %s\n", strlen(g_cmd_ra), g_cmd_ra);

	memset(g_cmd_para, 0, 256);
	memcpy(g_cmd_para, CMD_PARA, strlen(CMD_PARA));
	pr_info("CMD_PARA: %d, %s\n", strlen(g_cmd_para), g_cmd_para);

	//adv data set
	memset(&ble_content_internal_bak, 0, sizeof(ble_content_t));
	if(ble_adv_set(bt_content, &ble_content_internal_bak) < 0) {
		printf("%s: ble_adv_set failed\n", __func__);
		return -1;
	}

	ble_content_internal = &ble_content_internal_bak;
	gatt_create_services();
	return 0;
}

void gatt_set_stopping(bool stopping)
{
	gatt_is_stopping = stopping;
}

int ble_set_address(char *address)
{
	char ret_buff[1024];

	if(!address)
		return -1;

	memset(g_cmd_ra, 0, 256);
	if(sprintf(g_cmd_ra, "%s %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx", CMD_RA,
			address[0], address[1], address[2],
			address[3], address[4], address[5]) < 0) {
		pr_err("%s: set ble address failed\n", __func__);
		return -1;
	}
	pr_info("CMD_RA: %d, %s\n", strlen(g_cmd_ra), g_cmd_ra);

	execute(g_cmd_ra, ret_buff, 1024);
	pr_info("CMD_RA buff: %s", ret_buff);

	return 0;
}

int ble_set_adv_interval(unsigned short adv_int_min, unsigned short adv_int_max)
{
	char ret_buff[1024];
	char adv_min_low, adv_min_high;
	char adv_max_low, adv_max_high;

	if(adv_int_min < 32) {
		pr_err("%s: the minimum is 32(20ms), adv_int_min = %d", __func__, adv_int_min);
		adv_int_min = 32;
	}

	if(adv_int_max < adv_int_min)
		adv_int_max = adv_int_min;

	adv_min_low = adv_int_min & 0xFF;
	adv_min_high = (adv_int_min & 0xFF00) >> 8;
	adv_max_low = adv_int_max & 0xFF;
	adv_max_high = (adv_int_max & 0xFF00) >> 8;

	memset(g_cmd_para, 0, 256);
	if(sprintf(g_cmd_para, "%s %02hhx %02hhx %02hhx %02hhx %s",
			"hcitool -i hci0 cmd 0x08 0x0006",
			adv_min_low, adv_min_high, adv_max_low, adv_max_high,
			"00 01 00 00 00 00 00 00 00 07 00") < 0) {
		pr_err("%s: set ble adv interval failed\n", __func__);
		return -1;
	}
	pr_info("CMD_PARA: %d, %s\n", strlen(g_cmd_para), g_cmd_para);

	execute(g_cmd_para, ret_buff, 1024);
	pr_info("CMD_PARA buff: %s", ret_buff);
}
