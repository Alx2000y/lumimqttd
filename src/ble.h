#ifndef _BLE_H_
#define _BLE_H_

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

/*Json*/
#ifdef JSONC
	#include <json.h>
#else
	#include <json-c/json.h>
#endif


/*Local*/
#include "log.h"

static pthread_t ble_thread;
static pthread_t bt_thread;
static int signal_received=0;

extern void blescan_stop(void);
extern void blescan(void);
void* blescan_thread(void* args);
void* btscan_thread(void* args);
#endif /* _BLE_H_ */
