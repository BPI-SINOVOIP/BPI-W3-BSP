#ifndef __RK_BT_SOURCE_COMMON__
#define __RK_BT_SOURCE_COMMON__

#define msleep(x) usleep(x * 1000)

#define PRINT_FLAG_RKBTSOURCE "[RK_BT_RKBTSOURCE]"
#define PRINT_FLAG_SCAN "[RK_BT_SCAN]"
#define PRINT_FLAG_ERR "[RK_BT_ERROR]"
#define PRINT_FLAG_SUCESS "[RK_BT_SUCESS]"

typedef struct {
	char cmd[24];
	char name[48];
	char addr[17];
	char resverd[7];
} bt_msg_t;

typedef struct {
	char name[128];
	char addr[18];
	int rssi;
} scan_devices_t;

typedef struct {
	char magic[4]; //rkbt
	char start_flag; //0x01
	char devices_cnt;
	//scan_devices_t *devices;
	//char end_flg; //0x04
} scan_msg_t;

typedef struct {
	const char *cmd;
	const char *desc;
	int cmd_id;
} bt_command_t;

enum {
	RK_BT_SOURCE_INIT,
	RK_BT_SOURCE_DEINIT,
	RK_BT_SOURCE_SCAN_ON,
	RK_BT_SOURCE_SCAN_OFF,
	RK_BT_SOURCE_CONNECT,
	RK_BT_SOURCE_DISCONNECT,
	RK_BT_SOURCE_REMOVE,
};

static bt_command_t bt_command_table[] = {
	{"init", "init bluetooth source server", RK_BT_SOURCE_INIT},
	{"deinit", "deinit bluetooth source server", RK_BT_SOURCE_DEINIT},
	{"scan_on", "scan on sink devices", RK_BT_SOURCE_SCAN_ON},
	{"scan_off", "scan off sink devices", RK_BT_SOURCE_SCAN_OFF},
	{"connect", "connect [address]", RK_BT_SOURCE_CONNECT},
	{"disconnect", "discon [address]", RK_BT_SOURCE_DISCONNECT},
	{"remove", "remove [address]", RK_BT_SOURCE_REMOVE},
};

#endif