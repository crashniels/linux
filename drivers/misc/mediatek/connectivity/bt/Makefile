#########################################################
# Bluetooth character device driver

ccflags-y += -D CREATE_NODE_DYNAMIC=1 #1
ccflags-y += -D MTK_KERNEL_DEBUG

ccflags-y += \
	-I$(src)/../common/common_main/include \
	-I$(src)/../../include/mt-plat \
	-I$(src)/../common/common_main/core/include \
	-I$(src)/../common/common_detect \
	-I$(src)/../conninfra/base/include \
	-I$(src)/../common/common_main/linux/include

ifeq ($(CONFIG_MTK_COMBO_BT_BDROID),y)
	obj-$(CONFIG_MTK_COMBO) += stp_chrdev_bt.o
else ifeq ($(CONFIG_MTK_COMBO_BT_HCI),y)
	obj-$(CONFIG_MTK_COMBO) += stp_chrdev_bt_hci.o
endif

