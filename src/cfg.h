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

typedef struct cfg_t
{
    char *mqtt_host;
    uint16_t mqtt_port;
    uint16_t mqtt_keepalive;
    uint8_t auto_discovery;
    char *mqtt_user;
    char *mqtt_user_pw;
    uint8_t *listen_address;
    uint8_t mqtt_retain;
    uint16_t readinterval;
    uint16_t treshold;
    uint16_t verbosity;
    char *red_led;
    char *green_led;
    char *blue_led;
    uint8_t led_effect;
    uint8_t led_speed;
    char *lux_file;
    char *cputemp_file;
    char *ya_tts_api_key;
    char *ya_tts_folder_id;
    char *device_id;
    char *topic;
    char *cache_tts_path;
} cfg_t;

extern cfg_t config;

char *getmac(const char *def);
extern int cfg_load(char *file);
extern void cfg_dump(void);

#endif /* _CFG_H_ */
