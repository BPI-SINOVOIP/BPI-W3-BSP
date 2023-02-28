
hcq@ubuntu:~/RK3399_ANDROID7.1-TABLET-SDK_V1.00/kernel/drivers/net/ethernet/realtek$ git diff .
diff --git a/drivers/net/ethernet/realtek/Kconfig b/drivers/net/ethernet/realtek/Kconfig
old mode 100644
new mode 100755
index 7c69f4c..4f89db7
--- a/drivers/net/ethernet/realtek/Kconfig
+++ b/drivers/net/ethernet/realtek/Kconfig
@@ -106,4 +106,17 @@ config R8169
          To compile this driver as a module, choose M here: the module
          will be called r8169.  This is recommended.

+
+config R8125
+       tristate "Realtek 2.5Gigabit Ethernet controllers with PCI-Express interface"
+       depends on PCI
+       select CRC32
+       select MII
+       ---help---
+         This is a driver for the Fast Ethernet PCI network cards based on
+         the RTL  chips. If you have one of those, say Y here.
+
+         To compile this driver as a module, choose M here: the module
+         will be called RTL8125.  This is recommended.
+
 endif # NET_VENDOR_REALTEK
diff --git a/drivers/net/ethernet/realtek/Makefile b/drivers/net/ethernet/realtek/Makefile
old mode 100644
new mode 100755
index 71b1da3..aa3b7cc
--- a/drivers/net/ethernet/realtek/Makefile
+++ b/drivers/net/ethernet/realtek/Makefile
@@ -6,3 +6,4 @@ obj-$(CONFIG_8139CP) += 8139cp.o
 obj-$(CONFIG_8139TOO) += 8139too.o
 obj-$(CONFIG_ATP) += atp.o
 obj-$(CONFIG_R8169) += r8169.o
+obj-$(CONFIG_R8125) += r8125/

