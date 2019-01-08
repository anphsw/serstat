/* GPLv3 (C) [anp/hsw] 2019 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <inttypes.h>

#include <math.h>
#include <dlfcn.h>

int main(int argc, char ** argv) {

    static int bufsize=256;

    FILE* proc_file = fopen("/proc/tc_monitor","r");
    if (proc_file == 0) {
        perror("Cannot open file for read");
        return 1;
    }

    int rxpower_raw = 0, txpower_raw = 0, videopower_raw = 0, temperature_raw = 0, vcc_raw = 0, bias_raw = 0, genericvalue = 0;
    int onuid_ret = -1, onuid = -1, state_ret = -1, ostate = -1, astate = -1;
    uint8_t state[2] = {0};

    float rxpower = 0, txpower = 0, videopower = 0, temperature = 0, vcc = 0, bias = 0;
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

    rxpower = 10 * log10((float)rxpower_raw / 10000) / log10(10); // mW to dbm
    txpower = 10 * log10((float)txpower_raw / 10000) / log10(10);
    videopower = 10 * log10((float)videopower_raw / 10000) / log10(10);
    temperature = (((float)temperature_raw / 100) - 32) * 5 / 9; // farenheit to celsius
    vcc = (float)vcc_raw / 10000;
    bias = (float)bias_raw / 10000;

    void *handle = dlopen("libgponctl.so", RTLD_LAZY);	// load proprietary broadcom library
    if (!handle) {
        /* fail to load the library */
        fprintf(stderr, "dlopen error: %s\n", dlerror());
        goto print;
    }
    int (*gponCtl_getOnuId)(int*);
    *(void**)(&gponCtl_getOnuId) = dlsym(handle, "gponCtl_getOnuId");
    if (!gponCtl_getOnuId) {
	onuid = -1;
    } else {
	onuid_ret = gponCtl_getOnuId(&onuid);		// TODO: access /dev/bcm_ploam directly
	if (onuid_ret > 0) onuid = -1;
    }

    int (*gponCtl_getControlStates)(void*);
    *(void**)(&gponCtl_getControlStates) = dlsym(handle, "gponCtl_getControlStates");
    if (!gponCtl_getControlStates) {
	astate = -1;
	ostate = -1;
    } else {
	state_ret = gponCtl_getControlStates(&state);	// TODO: access /dev/bcm_ploam directly
	if (state_ret > 0) {
	    astate = -1;
	    ostate = -1;
	} else {
	    astate = state[0]; // administrative state
	    ostate = state[1]; // operational state
	}
    }

    dlclose(handle);

    print:
    if (argc > 1) {
	printf ("vcc: %.2f\nbias: %.2f\ntemperature: %.2f\nrxpower: %.2f\ntxpower: %.2f\nvideopower: %.2f\nonuid: %d\nostate: %d\n",
		vcc, bias, temperature, rxpower, txpower, videopower, onuid, ostate);
    } else {
	printf ("%.2f\n%.2f\n%.2f\n%.2f\n%.2f\n%.2f\n%d\n%d\n",
		vcc, bias, temperature, rxpower, txpower, videopower, onuid, ostate);
    }

    free(genericstring);
    free(contents);
    free(stringbuf);
    free(multibuf);

    fclose (proc_file);

    return 0;
}

