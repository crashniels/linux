export KBUILD_VERBOSE=1
export LINUX_SRC := $(srctree) #/lib/modules/$(shell uname -r)/build

###############################################################################
# SDIO
###############################################################################
ifneq ($(KERNELRELEASE),)
ccflags-y += -DDBG=0
ccflags-y += -I$(src)/../wlan/os
ccflags-y += -I$(src)/../wlan/core/gen4-mt7668/os/linux/include
ccflags-y += -I$(src)/../common/common_main/linux/include
ccflags-y += -I$(src)/../conninfra/base/include
ccflags-y += -I$(src)/../common/common_main/include

ifeq ($(MTK_CHIP_PCM), TRUE)
    ccflags-y += -DMTK_CHIP_PCM
endif

SDIO_MOD_NAME := btmtksdio
SDIO_CFILES := \
	btmtk_sdio.c \
        btmtk_main.c
export CONFIG_AMAZON_A2DP_TS_SDIO=y

ifeq ($(CONFIG_AMAZON_A2DP_TS_SDIO),y)
    SDIO_CFILES += btif_audio_ts.c
endif

ifneq ($(TARGET_BUILD_VARIANT), user)
    ccflags-y += -DMTK_KERNEL_DEBUG
endif

$(SDIO_MOD_NAME)-objs := $(SDIO_CFILES:.c=.o)

###############################################################################
# Common
###############################################################################
obj-$(CONFIG_MTK_COMBO_BT_MT7668_SDIO) := $(SDIO_MOD_NAME).o

else
PWD   := $(shell pwd)

all:
	make -C $(LINUX_SRC) M=$(PWD) PLATFORM_FLAGS="$(ccflags-y)" modules

sdio:
	make -C $(LINUX_SRC) M=$(PWD) $(SDIO_MOD_NAME).ko
modules_install:
	make -C $(LINUX_SRC) M=`pwd` modules_install INSTALL_MOD_PATH=$(LINUX_SRC)-build
clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions bin2h
	rm -f Module.markers
	rm -f Module.symvers
	rm -f modules.order

# Check coding style
export IGNORE_CODING_STYLE_RULES := NEW_TYPEDEFS,LEADING_SPACE,CODE_INDENT,SUSPECT_CODE_INDENT
ccs:
	./util/checkpatch.pl --no-tree --show-types --max-line-length=120 --ignore $(IGNORE_CODING_STYLE_RULES) -f btmtk_main.c
	./util/checkpatch.pl --no-tree --show-types --max-line-length=120 --ignore $(IGNORE_CODING_STYLE_RULES) -f btmtk_sdio.c
	./util/checkpatch.pl --no-tree --show-types --max-line-length=120 --ignore $(IGNORE_CODING_STYLE_RULES) -f btmtk_sdio.h
	./util/checkpatch.pl --no-tree --show-types --max-line-length=120 --ignore $(IGNORE_CODING_STYLE_RULES) -f btmtk_config.h
	./util/checkpatch.pl --no-tree --show-types --max-line-length=120 --ignore $(IGNORE_CODING_STYLE_RULES) -f btmtk_define.h
	./util/checkpatch.pl --no-tree --show-types --max-line-length=120 --ignore $(IGNORE_CODING_STYLE_RULES) -f btmtk_drv.h
endif