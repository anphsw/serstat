CC=/opt/toolchains/crosstools-mips-gcc-4.6-linux-3.4-uclibc-0.9.32-binutils-2.21/usr/bin/mips-unknown-linux-uclibc-gcc
CFLAGS=-lm -ldl -Wall -Os -s

all: clean serstat

serstat:
	$(CC) $(CFLAGS) serstat.c -o serstat

clean:
	$(RM) serstat
