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

MODULE_ALIAS("sdio:c*v02D0d0000*");
MODULE_ALIAS("sdio:c*v02D0d4362*");
MODULE_ALIAS("sdio:c*v02D0dAAE7*");
MODULE_ALIAS("sdio:c*v02D0dAAE8*");
MODULE_ALIAS("sdio:c*v02D0dA804*");
MODULE_ALIAS("sdio:c*v02D0dA806*");
MODULE_ALIAS("sdio:c*v02D0d4495*");
MODULE_ALIAS("sdio:c*v02D0d4496*");
MODULE_ALIAS("sdio:c*v02D0d4497*");
MODULE_ALIAS("sdio:c*v02D0dA805*");
MODULE_ALIAS("sdio:c*v02D0d4498*");
MODULE_ALIAS("sdio:c*v02D0d4499*");
MODULE_ALIAS("sdio:c*v02D0d449A*");
MODULE_ALIAS("sdio:c00v*d*");
