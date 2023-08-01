#ifndef _CFG_H_
#define _CFG_H_

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
/*Json*/
#ifdef JSONC
#include <json.h>
#else
#include <json-c/json.h>
#endif
/*Local*/
#include "log.h"

#ifndef VERSION
#define VERSION "TestVer"
#endif

typedef struct cfg_t
{
    uint16_t mqtt_port;
    uint8_t mqtt_keepalive;
    uint8_t auto_discovery;
    uint8_t disable_ble;
    uint8_t disable_bt;
    uint8_t disable_cputemp;
    uint8_t disable_illuminance;
    uint8_t disable_btn;
    uint8_t btscan_duration;
    uint8_t btscan_interval;
    uint8_t cache_all;
    uint8_t cache_make_index;
    uint8_t *listen_address;
    uint8_t mqtt_retain;
    uint8_t readinterval;
    uint8_t treshold;
    uint16_t cputemp_treshold;
    uint8_t verbosity;
    uint8_t led_effect;
    uint8_t led_duration;
    char *mqtt_host;
    char *mqtt_user;
    char *mqtt_user_pw;
    char *red_led;
    char *green_led;
    char *blue_led;
    char *lux_file;
    char *cputemp_file;
    char *ya_tts_api_key;
    char *ya_tts_folder_id;
    char *device_id;
    char *topic;
    char *cache_tts_path;
    char *ya_tts_voice;
} cfg_t;

extern cfg_t config;

char *getmac();
extern int cfg_load(char *file);
extern void cfg_dump(void);

#endif /* _CFG_H_ */
