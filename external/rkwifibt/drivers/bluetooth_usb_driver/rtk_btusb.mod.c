#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

MODULE_INFO(depends, "");

MODULE_ALIAS("usb:v0BDAp*d*dc*dsc*dp*icE0isc01ip01in*");
MODULE_ALIAS("usb:v13D3p*d*dc*dsc*dp*icE0isc01ip01in*");
MODULE_ALIAS("usb:v0489p*d*dc*dsc*dp*icE0isc01ip01in*");
MODULE_ALIAS("usb:v1358p*d*dc*dsc*dp*icE0isc01ip01in*");
MODULE_ALIAS("usb:v04CAp*d*dc*dsc*dp*icE0isc01ip01in*");
MODULE_ALIAS("usb:v2FF8p*d*dc*dsc*dp*icE0isc01ip01in*");
MODULE_ALIAS("usb:v0B05p*d*dc*dsc*dp*icE0isc01ip01in*");
MODULE_ALIAS("usb:v0930p*d*dc*dsc*dp*icE0isc01ip01in*");
MODULE_ALIAS("usb:v10ECp*d*dc*dsc*dp*icE0isc01ip01in*");
MODULE_ALIAS("usb:v04C5p*d*dc*dsc*dp*icE0isc01ip01in*");
MODULE_ALIAS("usb:v0CB5p*d*dc*dsc*dp*icE0isc01ip01in*");
MODULE_ALIAS("usb:v0CB8p*d*dc*dsc*dp*icE0isc01ip01in*");

MODULE_INFO(srcversion, "0F1F7A5CBD5402CB023DD9F");
