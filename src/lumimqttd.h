#ifndef _LUMI_H_
#define _LUMI_H_

#define DEFAULT_CONFIG_FILE "lumimqttd.json"
#define UNUSED(x) (void)(x)

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

/* Get Opt long. */
#include <getopt.h>

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
#include "player.h"
#include "log.h"
#include "tts.h"
#include "volume.h"
#include "leds.h"

static pthread_t button_thread;
static pthread_t periodic_thread;
struct mosquitto *mosq = NULL;

typedef struct state_t
{
    uint8_t btn0;
    uint16_t lux;
#ifdef USE_CPUTEMP
    uint32_t cputemp;
#endif
} state_t;

state_t curstate;
static FILE *luxfile;
#ifdef USE_CPUTEMP
static FILE *cputempfile;
#endif

extern bool mqtt_publish(const char *topic, const char *message);
extern bool mqtt_publish_once(const char *topic, const char *message);
void periodical_check(void);
void auto_discover(void);

#endif /* _LUMI_H_ */