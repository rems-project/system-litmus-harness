PREFIX = aarch64-linux-gnu-
CC = $(PREFIX)gcc -I . -DDEBUG
OBJCOPY = $(PREFIX)objcopy
OBJDUMP = $(PREFIX)objdump
LD = $(PREFIX)ld
ROOTDIR =
Q = @

all: qemu_stub.bin armstub8.bin bcm2710-rpi-3-b-plus.dtb

%.o: %.S
	@echo "[AS]      $(ROOTDIR)$@"
	$(Q)$(CC) -c $< -o $@

armstub8.elf: armstub8.o uart.o
	@echo "[LD]      $(ROOTDIR)$@"
	$(Q)$(LD) --section-start=.text=0 -o $@ $^
	@echo "[OBJDUMP] $(ROOTDIR)$@.S"
	$(Q)$(OBJDUMP) -D $@ > $@.S

qemu_stub.elf: qemu_stub.o
	@echo "[LD]      $(ROOTDIR)$@"
	$(Q)$(LD) --section-start=.text=0 -o $@ $^
	@echo "[OBJDUMP] $(ROOTDIR)$@.S"
	$(Q)$(OBJDUMP) -D $@ > $@.S

%.bin: %.elf
	@echo "[OBJCOPY] $(ROOTDIR)$@"
	$(Q)$(OBJCOPY) $< -O binary $@

bcm2710-rpi-3-b-plus.dtb: rpi3.dts
	@echo "[DTC]     $(ROOTDIR)$@"
	$(Q)dtc -I dts -O dtb $< -o $@ -W no-unit_address_vs_reg

clean:
	rm -f bcm2710-rpi-3-b-plus.dtb
	rm -f armstub8.o armstub8.elf armstub8.elf.S armstub8.bin
	rm -f uart.o
	rm -f qemu_stub.o qemu_stub.elf qemu_stub.elf.S qemu_stub.bin