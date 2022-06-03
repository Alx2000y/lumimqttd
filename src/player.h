#ifndef _PLAY_H_
#define _PLAY_H_

#include <stdio.h>
#include <stdlib.h>
#include <mpg123.h>
#include <out123.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <time.h>
#include <sys/types.h>
#include <syslog.h>
#include <stdarg.h>
#include <linux/soundcard.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pthread.h>
#include <errno.h>
#include <linux/limits.h>
#include <sys/wait.h>
#include "syslog.h"

#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

typedef struct
{
    int action;
    int channel;
    int slen;
    char *playfile;
} playArgs_t;

static mpg123_handle *mh;
static out123_handle *ao;
static pthread_t play_thread;
static pthread_t sockplay_thread;
static int sock;
static SSL *ssl;
static int pfd[2];
static int stopplay = 0;
static playArgs_t playargs;

static void sockpipe_thread(int tfd);
static int return_sock(char *url);
int play_on_message(char *id, char *payload, int len);
void call_play(char *playfile, int len, int channel);
void stop_play();
void exit_play(void);
void *playc(void *arg);
int play(char *playfile, char *outfile);
#endif /* _PLAY_H_ */