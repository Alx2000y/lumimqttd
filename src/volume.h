#ifndef _VOL_H_
#define _VOL_H_

/* Include all the header files here. */
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <mosquitto.h>
#include <pthread.h>

/* For syslog logging facility. */
#include <syslog.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

/*Alsa*/
#include <alsa/asoundlib.h>
#include <alsa/mixer.h>

/*Json*/
#ifdef JSONC
#include <json.h>
#else
#include <json-c/json.h>
#endif

#include <fcntl.h>
#include <linux/input.h>

/*Local*/
#include "cfg.h"
#include "log.h"
//#include "lumimqttd.h"


#ifdef USE_MPD
int volumempd = 100;
#endif

static uint8_t curvolume;
static uint8_t curalert;
#ifdef USE_MPD
static uint8_t curvolumempd;
#endif


extern int volume_on_message(char *id, char *payload, int len);
extern void init_volume(void);
extern void volume_periodical_check(void);
void set_volume(int vol, const char *selem_name);
int get_volume(const char *selem_name);
void volume_auto_discover(void);
#endif /* _VOL_H_ */