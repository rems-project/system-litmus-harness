# amount of memory for QEMU to allocate
QEMU_MEM = 1G

# default HOST uses the gic
# otherwise can pass no-gic to use emulated irqchip
HOST = gic

RUN_CMD_HOST_GIC = 	\
	$(QEMU) \
		-nodefaults -machine virt,gic-version=host --enable-kvm -cpu host \
		-device virtio-serial-device -device virtconsole \
		-display none -serial stdio \
		-m $(QEMU_MEM) \
		-kernel $(OUT_NAME) -smp 4 -append "$$*"

RUN_CMD_HOST_NO_GIC = 	\
	$(QEMU) \
		-nodefaults -machine virt --enable-kvm -cpu host \
		-device virtio-serial-device -device virtconsole \
		-display none -serial stdio \
		-m $(QEMU_MEM) \
		-kernel $(OUT_NAME) -smp 4 -append "$$*"

RUN_CMD_HOST = \
	$(if $(filter gic,$(HOST)),$(RUN_CMD_HOST_GIC),\
		$(if $(filter no-gic,$(HOST)),$(RUN_CMD_HOST_NO_GIC),\
			$(info $(USAGE)) \
  			$(error Unexpected HOST="$(HOST)" param)))

RUN_CMD_LOCAL_VIRT = 	\
	$(QEMU) \
		-nodefaults -machine virt,secure=on -cpu cortex-a57 \
		-device virtio-serial-device \
		-display none -serial stdio \
		-m $(QEMU_MEM) \
		-kernel $(OUT_NAME) -smp 4 -append "$$*"

RUN_CMD_LOCAL_RPI3 = 	\
	$(QEMU) \
		-machine raspi3 \
		-nographic \
		-serial null -serial mon:stdio \
		-m $(QEMU_MEM) \
		-kernel $(OUT_NAME) -smp 4 -append "$$*"