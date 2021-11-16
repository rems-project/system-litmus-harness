In this directory is a custom BL31/BL33 level firmware image for the Raspberry Pi 3 (B+)

Running `make` should build everything required:
* armstub8.bin
* qemu_stub.bin
* bcm2710-rpi-3-b-plus.dtb

armstub8
--------

this stub is the EL3 non-secure firmware payload that can be copied directly to the
RPi3's sdcard, alongwith the included config.txt in the prebuilt/ directory

qemu_stub
---------

this is the pre-armstub payload for QEMU to run before the armstub gets ran.
This performs (some) of the loaders duties of overwriting magic locations with values from the config
(hardcoded)

bcm2710-rpi-3-b-plus.dtb
------------------------

the flattened device tree blob compiled from the included rpi3.dts
which includes the spintable locations in the custom armstub.


Running in QEMU
---------------

One can run a QEMU kernel image with the real firmware payload like so:
