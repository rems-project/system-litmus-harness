RUN_CMD_HOST_GIC = 	\
	$(QEMU) \
		-nodefaults -machine virt,gic-version=host --enable-kvm -cpu host \
		-device virtio-serial-device -device virtconsole \
		-display none -serial stdio \
		-m 1G \
		-kernel $(OUT_NAME) -smp 4 -append "$$*"

RUN_CMD_HOST_NO_GIC = 	\
	$(QEMU) \
		-nodefaults -machine virt --enable-kvm -cpu host \
		-device virtio-serial-device -device virtconsole \
		-display none -serial stdio \
		-m 1G \
		-kernel $(OUT_NAME) -smp 4 -append "$$*"

RUN_CMD_LOCAL = 	\
	$(QEMU) \
		-nodefaults -machine virt -cpu cortex-a57 \
		-device virtio-serial-device \
		-display none -serial stdio \
		-m 1G \
		-kernel $(OUT_NAME) -smp 4 -append "$$*"

# default HOST uses the gic
# otherwise can pass no-gic to use emulated irqchip
HOST = gic

ifeq ($(HOST),gic)
  RUN_CMD_HOST = $(RUN_CMD_HOST_GIC)
else ifeq ($(HOST),no-gic)
  RUN_CMD_HOST = $(RUN_CMD_HOST_NO_GIC)
else
  $(info $(USAGE))
  $(error Unexpected HOST="$(HOST)" param)
endif