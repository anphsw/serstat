CC=/opt/toolchains/crosstools-mips-gcc-4.6-linux-3.4-uclibc-0.9.32-binutils-2.21/usr/bin/mips-unknown-linux-uclibc-gcc
CFLAGS=-lm -Wall -O1 -s
# do not use -Os and -O2, output trashed by some optimizations

all: clean serstat serstat_i2c serstat_laserdev

serstat:
	$(CC) $(CFLAGS) serstat.c -DUSE_TCMONITOR -o serstat

serstat_i2c:
	$(CC) $(CFLAGS) serstat.c -DUSE_I2C -o serstat_i2c -ldl 

serstat_laserdev:
	$(CC) $(CFLAGS) serstat.c -DUSE_LASERDEV -o serstat_laserdev

clean:
	$(RM) serstat serstat_i2c serstat_laserdev
