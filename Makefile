.PHONY : all
CURDIR=$(shell pwd)
CPUS?=$(shell echo $$(( $$(( `awk -F- '{ print $$2 }' /sys/devices/system/cpu/present` + 1 )) / 2 )))
V?=0

all: install

config:
	$(Q)mkdir -p $(CURDIR)/build
	$(Q)$(MAKE) V=$(V) -C $(CURDIR)/linux O=$(CURDIR)/build CROSS_COMPILE=arm-linux-gnueabihf- ARCH=arm caninos_labrador_defconfig
	
menuconfig:
	$(Q)$(MAKE) V=$(V) -C $(CURDIR)/linux O=$(CURDIR)/build CROSS_COMPILE=arm-linux-gnueabihf- ARCH=arm menuconfig
	
kernel: config
	$(Q)$(MAKE) V=$(V) -C $(CURDIR)/linux O=$(CURDIR)/build CROSS_COMPILE=arm-linux-gnueabihf- ARCH=arm dtbs
	$(Q)$(MAKE) V=$(V) -C $(CURDIR)/linux O=$(CURDIR)/build CROSS_COMPILE=arm-linux-gnueabihf- ARCH=arm -j$(CPUS) uImage

mods-prepare: kernel
	$(Q)$(MAKE) V=$(V) -C $(CURDIR)/linux O=$(CURDIR)/build CROSS_COMPILE=arm-linux-gnueabihf- ARCH=arm -j$(CPUS) modules_prepare

modules: mods-prepare
	$(Q)$(MAKE) V=$(V) -C $(CURDIR)/linux O=$(CURDIR)/build CROSS_COMPILE=arm-linux-gnueabihf- ARCH=arm -j$(CPUS) modules

mods-install: modules
	$(Q)$(MAKE) V=$(V) -C $(CURDIR)/linux O=$(CURDIR)/build CROSS_COMPILE=arm-linux-gnueabihf- ARCH=arm -j$(CPUS) INSTALL_MOD_PATH=$(CURDIR)/build modules_install

install: kernel mods-install
	$(Q)rm -rf $(CURDIR)/output
	$(Q)mkdir -p $(CURDIR)/output
	$(Q)mkdir -p $(CURDIR)/output/lib
	$(Q)cp -rf $(CURDIR)/build/lib/modules $(CURDIR)/output/lib/; find $(CURDIR)/output/lib/ -type l -exec rm -f {} \;
	$(Q)cp $(CURDIR)/build/arch/arm/boot/uImage $(CURDIR)/output/
	$(Q)cp $(CURDIR)/build/arch/arm/boot/dts/caninos_labrador.dtb $(CURDIR)/output/kernel.dtb

single-mod: _check-mod mods-prepare
	$(Q)$(MAKE) V=$(V) -C $(CURDIR)/linux O=$(CURDIR)/build M=arch/arm/mach-caninos/$(MOD) CROSS_COMPILE=arm-linux-gnueabihf- ARCH=arm -j$(CPUS)

_check-mod:
	@[ "${MOD}" ] || ( echo ">> MOD= module is not set"; exit 1 )

clean:
	$(Q)rm -rf build output
	$(Q)$(MAKE) V=$(V) -C $(CURDIR)/linux mrproper
