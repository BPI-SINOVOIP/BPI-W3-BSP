#ifdef __cplusplus
extern "C" {
#endif

#include <RkBtObex.h>

void obex_pbap_register_status_cb(RK_BT_OBEX_STATE_CALLBACK cb);
void *obex_main_thread(void *arg);
int obex_connect_pbap(char *dev_addr);
int obex_get_pbap_pb(char *dir_name, char *dir_file);
int obex_disconnect(int argc, char *btaddr);
void obex_quit(void);

#ifdef __cplusplus
}
#endif
