/* GPLv3 (C) [anp/hsw] 2019 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <inttypes.h>

#include <math.h>
#include <dlfcn.h>

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


int main(int argc, char ** argv) {

    float rxpower = -INFINITY, txpower = -INFINITY, videopower = -INFINITY, temperature = -INFINITY, vcc = -INFINITY, bias = -INFINITY;

    int onuid = -1, ostate = -1, astate = -1;
    uint8_t state[2] = {0};

#if defined(USE_TCMONITOR) || defined(USE_LASERDEV)
    int rxpower_raw = 0, txpower_raw = 0, videopower_raw = 0, temperature_raw = 0, vcc_raw = 0, bias_raw = 0;
#endif

#if defined(USE_TCMONITOR)
    static int bufsize=256;
    int genericvalue = 0;

    FILE* proc_file = fopen("/proc/tc_monitor","r");
    if (proc_file == 0) {
        perror("/proc/tc_monitor read failed");
        return 1;
    }

    char *genericstring = malloc(bufsize);

    char *contents = malloc(bufsize);
    char *stringbuf = malloc(bufsize);
    char *multibuf = malloc(bufsize);

    int multistring = 0;    
    while (!feof(proc_file) || multistring != 0)
    {
	if (multistring != 0) {
	    memcpy(contents, multibuf, bufsize);
	} else {
	    fgets(contents, bufsize, proc_file); //get string
	}
	if (strlen (contents) == 1) continue;				//newline
	if (sscanf (contents, "%[^,],%s\n", stringbuf, multibuf) == 2) {
	    multistring = 1;
	} else if (sscanf (contents, "%s\n", stringbuf) == 1) {
	    multistring = 0;
	} else {
	    multistring = 0;
	    continue;
	}
	if (sscanf(stringbuf, "%[^=]=%10d\n", genericstring, &genericvalue) == 2) {
	    if	(!strcasecmp("pon_rx_power", genericstring))	rxpower_raw = genericvalue;
	    if	(!strcasecmp("pon_tx_power", genericstring))	txpower_raw = genericvalue;
	    if	(!strcasecmp("catv_rx_power", genericstring))	videopower_raw = genericvalue;
	    if	(!strcasecmp("temperature", genericstring))	temperature_raw = genericvalue;
	    if	(!strcasecmp("vcc", genericstring))		vcc_raw = genericvalue;
	    if	(!strcasecmp("bias", genericstring))		bias_raw = genericvalue;
	}
    }
#endif

#if defined(USE_LASERDEV)
    int laserdev_fd = open("/dev/laser_dev", O_RDWR);
    if (laserdev_fd == -1) {
        fprintf(stderr, "open /dev/laser_dev failed");
        return 1;
    }
    if (ioctl(laserdev_fd, 0x40024f05, &rxpower_raw)) {		//0x40024f05 - rxpower
            fprintf(stderr, "rxpower ioctl failed\n");
	    temperature_raw = -1;
    }
    if (ioctl(laserdev_fd, 0x40024f06, &txpower_raw)) {		//0x40024f06 - txpower
            fprintf(stderr, "txpower ioctl failed\n");
	    temperature_raw = -1;
    }
    if (ioctl(laserdev_fd, 0x40024f08, &temperature_raw)) {	//0x40024f08 - temperature
            fprintf(stderr, "temperature ioctl failed\n");
	    temperature_raw = -1;
    }

    if (ioctl(laserdev_fd, 0x40024f09, &vcc_raw)) {		//0x40024f09 - vcc
            fprintf(stderr, "vcc ioctl failed\n");
	    vcc_raw = -1;
    }

    if (ioctl(laserdev_fd, 0x40024f0a, &bias_raw)) {		//0x40024f0a - bias
            fprintf(stderr, "bias ioctl failed\n");
	    vcc_raw = -1;
    }
    close(laserdev_fd);
#endif

#if defined(USE_TCMONITOR) || defined(USE_LASERDEV)
    rxpower = 10 * log10((float)rxpower_raw / 10000) / log10(10); // mW to dbm
    txpower = 10 * log10((float)txpower_raw / 10000) / log10(10);
    videopower = 10 * log10((float)videopower_raw / 10000) / log10(10);
    temperature = (((float)temperature_raw / 100) - 32) * 5 / 9; // farenheit to celsius
    vcc = (float)vcc_raw / 10000;
    bias = (float)bias_raw / 10000;
#endif

#if defined(USE_I2C)
    void *i2cctl_handle = dlopen("libi2cctl.so", RTLD_LAZY);	// load proprietary broadcom library TODO: access i2c directly
    if (!i2cctl_handle) {
        /* fail to load the library */
        fprintf(stderr, "dlopen error: %s\n", dlerror());
        goto no_libi2cctl;
    }

    float (*i2cCtl_getRxPower)();
    float (*i2cCtl_getTxPower)();
    float (*i2cCtl_getVideoInput)();
    float (*i2cCtl_getTemperature)();
    float (*i2cCtl_getSupplyVoltage)();
    float (*i2cCtl_getTxBias)();

    *(void**)(&i2cCtl_getRxPower)	= dlsym(i2cctl_handle, "i2cCtl_getRxPower");
    *(void**)(&i2cCtl_getTxPower)	= dlsym(i2cctl_handle, "i2cCtl_getTxPower");
    *(void**)(&i2cCtl_getVideoInput)	= dlsym(i2cctl_handle, "i2cCtl_getVideoInput");
    *(void**)(&i2cCtl_getTemperature)	= dlsym(i2cctl_handle, "i2cCtl_getTemperature");
    *(void**)(&i2cCtl_getSupplyVoltage)	= dlsym(i2cctl_handle, "i2cCtl_getSupplyVoltage");
    *(void**)(&i2cCtl_getTxBias)	= dlsym(i2cctl_handle, "i2cCtl_getTxBias");

    if (i2cCtl_getRxPower)		rxpower = i2cCtl_getRxPower();
    if (i2cCtl_getTxPower)		txpower = i2cCtl_getTxPower();
    if (i2cCtl_getVideoInput)		videopower = i2cCtl_getVideoInput();
    if (i2cCtl_getTemperature)		temperature = i2cCtl_getTemperature();
    if (i2cCtl_getSupplyVoltage)	vcc = i2cCtl_getSupplyVoltage();
    if (i2cCtl_getTxBias)		bias = i2cCtl_getTxBias();

    dlclose(i2cctl_handle);
    no_libi2cctl:;
#endif

    int ploam_fd = open("/dev/bcm_ploam", O_RDWR);
    if (ploam_fd == -1) {
        fprintf(stderr, "open /dev/bcm_ploam failed");
        goto print;
    }

    if (ioctl(ploam_fd, 0x83, &onuid)) { // 0x83 - get onu id
            fprintf(stderr, "ioctl 0x83 failed\n");
	    onuid = -1;
    } else if (onuid < 0 || onuid > 255) {
            fprintf(stderr, "ioctl 0x83 returned invalid value\n");
	    onuid = -1;
    }

    if (ioctl(ploam_fd, 0x6f, &state)) { // 0x6f - get control states structure
            fprintf(stderr, "ioctl 0x6f failed\n");
	    astate = -1;
	    ostate = -1;
    } else {
	    astate = state[0]; // administrative state
	    ostate = state[1]; // operational state
    }
    close(ploam_fd);

    print:
    if (argc > 1) {
	printf ("vcc: %.2f\nbias: %.2f\ntemperature: %.2f\nrxpower: %.2f\ntxpower: %.2f\nvideopower: %.2f\nonuid: %d\nostate: %d\nastate: %d\n",
		vcc, bias, temperature, rxpower, txpower, videopower, onuid, ostate, astate);
    } else {
	printf ("%.2f\n%.2f\n%.2f\n%.2f\n%.2f\n%.2f\n%d\n%d\n%d\n",
		vcc, bias, temperature, rxpower, txpower, videopower, onuid, ostate, astate);
    }

#if defined(USE_TCMONITOR)
    free(genericstring);
    free(contents);
    free(stringbuf);
    free(multibuf);

    fclose (proc_file);
#endif

    return 0;
}

