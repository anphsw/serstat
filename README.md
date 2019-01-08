# serstat
Small utility to get some GPON module statistics from Broadcom-based ONU using /proc/tc_monitor and broadcom proprietary library.
Tested on BCM68380 board + snmpd.
You will also need libgponctl.so found on any such ONU.

# note:
CC path is hardcoded in makefile.
Use command: CC="/path/to/your/cross/gcc" make
