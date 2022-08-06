#ifndef __A2DP_SOURCE_CTRL__
#define __A2DP_SOURCE_CTRL__

#include "../gdbus/gdbus.h"
#include "RkBtBase.h"
#include "RkBle.h"
#include "RkBtSource.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DEV_PLATFORM_UNKNOWN    0 /* unknown platform */
#define DEV_PLATFORM_IOS        1 /* Apple iOS */
#define IOS_VENDOR_SOURCE_BT    76 /* Bluetooth SIG, apple id = 0x4c */
#define IOS_VENDOR_SOURCE_USB   1452 /* USB Implementer's Forum, apple id = 0x05ac */

typedef enum _bt_devices_type {
	BT_DEVICES_A2DP_SINK,
	BT_DEVICES_A2DP_SOURCE,
	BT_DEVICES_BLE,
	BT_DEVICES_HFP,
	BT_DEVICES_SPP,
} BtDeviceType;

void bt_register_state_callback(RK_BT_STATE_CALLBACK cb);
void bt_deregister_state_callback();
void bt_register_bond_callback(RK_BT_BOND_CALLBACK cb);
void bt_deregister_bond_callback();
void bt_register_discovery_callback(RK_BT_DISCOVERY_CALLBACK cb);
void bt_deregister_discovery_callback();
void bt_register_dev_found_callback(RK_BT_DEV_FOUND_CALLBACK cb);
void bt_deregister_dev_found_callback();
void ble_register_state_callback(RK_BLE_STATE_CALLBACK cb);
void ble_deregister_state_callback();
void a2dp_master_register_cb(void *userdata, RK_BT_SOURCE_CALLBACK cb);
void a2dp_master_deregister_cb();
void bt_register_name_change_callback(RK_BT_NAME_CHANGE_CALLBACK cb);
void bt_deregister_name_change_callback();
void ble_register_mtu_callback(RK_BT_MTU_CALLBACK cb);
void ble_deregister_mtu_callback();

void bt_state_send(RK_BT_STATE state);
void ble_state_send(RK_BLE_STATE status);
void ble_get_state(RK_BLE_STATE *p_state);
int bt_open(RkBtContent *bt_content);
void bt_close();
int a2dp_master_scan(void *data, int len, RK_BT_SCAN_TYPE scan_type);
int a2dp_master_connect(char *address);
int a2dp_master_status(char *addr_buf, int addr_len, char *name_buf, int name_len);
int remove_by_address(char *address);
void a2dp_master_event_send(RK_BT_SOURCE_EVENT event, char *dev_addr, char *dev_name);
int reconn_last_devices(BtDeviceType type);
int disconnect_current_devices();
int get_dev_platform(char *address);
int get_current_dev_platform();
int connect_by_address(char *addr);
int disconnect_by_address(char *addr);
int pair_by_addr(char *addr);
int unpair_by_addr(char *addr);
int bt_set_device_name(char *name);
int bt_get_device_name(char *name_buf, int name_len);
int bt_get_device_addr(char *addr_buf, int addr_len);
int bt_get_default_dev_addr(char *addr_buf, int addr_len);
void bt_display_devices();
void bt_display_paired_devices();
int bt_start_discovery(unsigned int mseconds, RK_BT_SCAN_TYPE scan_type);
int bt_cancel_discovery(RK_BT_DISCOVERY_STATE state);
int bt_get_scaned_devices(RkBtScanedDevice **dev_list, int *count, bool paired);
int bt_free_scaned_devices(RkBtScanedDevice *dev_list);
bool bt_is_discovering();
bool bt_is_scaning();
bool bt_is_connected();
int ble_disconnect(void);
void ble_clean(void);
int remove_ble_device();
RK_BT_PLAYROLE_TYPE bt_get_playrole_by_addr(char *addr);
void dev_found_send(GDBusProxy *proxy, RK_BT_DEV_FOUND_CALLBACK cb);
struct GDBusProxy *find_device_by_address(char *address);
void set_default_attribute(GDBusProxy *proxy);
void source_set_reconnect_tag(bool reconnect);
void source_stop_connecting();
bool get_device_connected_properties(char *addr);
int a2dp_master_save_status(char *address);
int bt_get_device_name_by_proxy(GDBusProxy *proxy, char *name_buf, int name_len);
int bt_get_device_addr_by_proxy(GDBusProxy *proxy, char *addr_buf, int addr_len);
int bt_get_eir_data(char *address, char *eir_data, int eir_len);

#ifdef __cplusplus
}
#endif

#endif /* __A2DP_SOURCE_CTRL__ */
