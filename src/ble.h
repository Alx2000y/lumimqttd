#ifndef _BLE_H_
#define _BLE_H_

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>


#include <time.h>

/*Json*/
#ifdef JSONC
	#include <json.h>
#else
	#include <json-c/json.h>
#endif


/*Local*/
#include "log.h"

typedef struct ble_state_t
{
    uint8_t active;
    uint8_t state;
    uint32_t lastseen;
    char* addr;
} ble_state_t;

ble_state_t* blemacs;
int blemacs_size;
int ble_full=0;

int ble_timeout = 30;
extern void set_ble_timeout(int timeout);
extern void init_maclist(int num);
extern void add_mac(char* mac, int id);
extern void print_mac();
extern void ble_periodical_check();

static pthread_t ble_thread;
static pthread_t bt_thread;
static int signal_received=0;

extern void blescan_stop(void);
extern void blescan(void);
void* blescan_thread(void* args);
void* btscan_thread(void* args);
#endif /* _BLE_H_ */
