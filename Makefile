.PHONY : all
.PHONY : clean
CURDIR=$(shell pwd)
CPUS=$$(($(shell cat /sys/devices/system/cpu/present | awk -F- '{ print $$2 }')+1))

all: clean
	$(Q)mkdir $(CURDIR)/build
	$(Q)$(MAKE) -C $(CURDIR)/linux O=$(CURDIR)/build CROSS_COMPILE=arm-linux-gnueabihf- ARCH=arm caninos_labrador_defconfig
	$(Q)$(MAKE) -C $(CURDIR)/linux O=$(CURDIR)/build CROSS_COMPILE=arm-linux-gnueabihf- ARCH=arm menuconfig
	$(Q)$(MAKE) -C $(CURDIR)/linux O=$(CURDIR)/build CROSS_COMPILE=arm-linux-gnueabihf- ARCH=arm dtbs
	$(Q)$(MAKE) -C $(CURDIR)/linux O=$(CURDIR)/build CROSS_COMPILE=arm-linux-gnueabihf- ARCH=arm -j$(CPUS) uImage modules
	$(Q)$(MAKE) -C $(CURDIR)/linux O=$(CURDIR)/build CROSS_COMPILE=arm-linux-gnueabihf- ARCH=arm -j$(CPUS) INSTALL_MOD_PATH=$(CURDIR)/build modules_install
	$(Q)mkdir $(CURDIR)/output
	$(Q)mkdir $(CURDIR)/output/lib
	$(Q)cp -rf $(CURDIR)/build/lib/modules $(CURDIR)/output/lib/; find $(CURDIR)/output/lib/ -type l -exec rm -f {} \;
	$(Q)cp $(CURDIR)/build/arch/arm/boot/uImage $(CURDIR)/output/
	$(Q)cp $(CURDIR)/build/arch/arm/boot/dts/caninos_labrador.dtb $(CURDIR)/output/kernel.dtb

clean:
	$(Q)rm -rf build output
