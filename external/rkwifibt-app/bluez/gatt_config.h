#ifndef __BT_GATT_CONFIG_H__
#define __BT_GATT_CONFIG_H__

#include <RkBtBase.h>

#ifdef __cplusplus
extern "C" {
#endif

int gatt_init(RkBtContent *bt_content);
int ble_enable_adv(void);
void ble_disable_adv(void);
int gatt_write_data(char *uuid, void *data, int len);
int gatt_setup(void);
void gatt_cleanup(void);
void gatt_set_stopping(bool stopping);
int ble_set_address(char *address);
int ble_set_adv_interval(unsigned short adv_int_min, unsigned short adv_int_max);

#ifdef __cplusplus
}
#endif

#endif /* __BT_GATT_CONFIG_H__ */

