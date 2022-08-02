#ifndef _LEDS_H_
#define _LEDS_H_

/* Include all the header files here. */
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* For syslog logging facility. */
#include <syslog.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

/*Json*/
#ifdef JSONC
#include <json.h>
#else
#include <json-c/json.h>
#endif

/*Timer*/
//#include <linux/timer.h>

/*Local*/
#include "cfg.h"
#include "player.h"
#include "log.h"
#include "tts.h"
//#include "lumimqttd.h"

typedef struct leds_rgb
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t brightness;
    uint8_t state;
    uint8_t duration;
    uint8_t effect;
    uint8_t status;
} leds_rgb;

static char* ledpattern = "#ff00ff 6 #00ff00 5 #000000 5";

static FILE *ledrfile;
static FILE *ledgfile;
static FILE *ledbfile;

static leds_rgb curstateleds;

//static struct timer_list effect_timer;

int leds_on_message(char *id, char *payload, int len);
void led_sigc(void);
void init_leds(void);
void init_fleds(void);
void set_leds(struct leds_rgb leds);
struct leds_rgb get_leds(void);
void leds_periodical_check(void);
void leds_auto_discover(void);
void applyRGB(uint8_t r, uint8_t g, uint8_t b);

#endif /* _LEDS_H_ */