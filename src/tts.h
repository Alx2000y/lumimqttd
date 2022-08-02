#ifndef _TTS_H_
#define _TTS_H_
/*Local*/
#include "cfg.h"
#include "player.h"
#include "log.h"

#include <libwebsockets.h>
#include <string.h>
#include <signal.h>

/*Json*/
#ifdef JSONC
#include <json.h>
#else
#include <json-c/json.h>
#endif

/*Alsa*/
#include <alsa/asoundlib.h>

#define BUFSZ 512

typedef struct tts_t
{
    uint8_t speed;
    uint8_t cache;
    uint8_t updatecache;
    char *emotion;
    char *voice;
    char *text;
} tts_t;

extern tts_t tts;

static pthread_t tts_thread;

int isURIChar(const char ch);
char *URIEncode(const char *uri);

extern void tts_say(char *text);
extern void tts_voice(char *text);
extern void tts_emotion(char *text);
extern void tts_speed(uint8_t speed);
extern int tts_on_message(char *id, char *payload, int len);
void *ttssay_thread(void *args);
extern void init_tts(void);
#endif /* _TTS_H_ */