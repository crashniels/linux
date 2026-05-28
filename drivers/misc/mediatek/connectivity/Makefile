#
# Copyright (C) 2015 MediaTek Inc.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#

# Connectivity combo driver
# If KERNELRELEASE is defined, we've been invoked from the
# kernel build system and can use its language.
ifneq ($(KERNELRELEASE),)
subdir-ccflags-y += -D MTK_WCN_REMOVE_KERNEL_MODULE
ifeq ($(CONFIG_ARM64), y)
subdir-ccflags-y += -D CONFIG_MTK_WCN_ARM64
endif

subdir-ccflags-y += -D CFG_SUPPORT_AEE=0

# Do build-in for Makefile checking
#export CONFIG_WLAN_DRV_BUILD_IN=y

ifeq ($(CONFIG_MTK_CONN_LTE_IDC_SUPPORT),y)
    subdir-ccflags-y += -D WMT_IDC_SUPPORT=1
else
    subdir-ccflags-y += -D WMT_IDC_SUPPORT=0
endif
    subdir-ccflags-y += -D MTK_WCN_WMT_STP_EXP_SYMBOL_ABSTRACT
    obj-$(CONFIG_MTK_COMBO_COMM) += common/
    obj-$(CONFIG_MTK_COMBO) += conninfra/

ifeq ($(CONFIG_MTK_COMBO),$(filter $(CONFIG_MTK_COMBO),m y))
    subdir-ccflags-y += -D MTK_BT_HCI -D REMOVE_MK_NODE=1
    obj-$(CONFIG_MTK_COMBO) += bt/
endif

ifeq ($(CONFIG_MTK_COMBO_BT_MT7668_SDIO),$(filter $(CONFIG_MTK_COMBO_BT_MT7668_SDIO),m y))
    obj-$(CONFIG_MTK_COMBO_BT_MT7668_SDIO) += bt_mt7668_sdio/
endif

ifeq ($(CONFIG_MTK_WIFI_DEVICE),$(filter $(CONFIG_MTK_WIFI_DEVICE),m y))
    obj-$(CONFIG_MTK_WIFI_DEVICE) += wlan/core/gen4-mt7668/
endif

ifeq ($(CONFIG_MTK_COMBO_WIFI),$(filter $(CONFIG_MTK_COMBO_WIFI),m y))
    obj-$(CONFIG_MTK_COMBO_WIFI) += wlan/adaptor/
endif
    obj-$(CONFIG_MTK_COMBO_GPS) += gps/
    obj-n := dummy.o

# Otherwise we were called directly from the command line;
# invoke the kernel build system.
else
    KERNELDIR ?= /lib/modules/$(shell uname -r)/build
    PWD  := $(shell pwd)
default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) PLATFORM_FLAGS="$(subdir-ccflags-y)" modules KBUILD_VERBOSE=1
endif
