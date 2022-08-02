#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "tts.h"

#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/md5.h>


tts_t tts;
const char *lvoices[] = {"google", "oksana", "jane", "omazh", "zahar", "ermil", "silaerkan", "erkanyavas", "alyss", "nick", "alena", "filipp", NULL};
const char *lvoicessex[] = {"F", "F", "F", "F", "M", "M", "F", "M", "F", "M", "F", "M", NULL};
const char *lvoiceslang[] = {"", "ru_RU", "ru_RU", "ru_RU", "ru_RU", "ru_RU", "tr_TR", "tr_TR", "en_US", "en_US", "ru_RU", "ru_RU", NULL};
const char *lemotions[] = {"good", "neutral", "evil", NULL};

void init_tts()
{
    if (strlen(config.ya_tts_api_key) < 10) {
   	    tts.voice = strdup("google");
        return;
    }
    char *topic, *message;

    tts.voice = strdup("alyss");
    tts.emotion = strdup("neutral");
    tts.speed = 10;
    topic = malloc(strlen(config.topic) + 50);
    message = malloc(500);

    sprintf(message, "%s", tts.voice);
    sprintf(topic, "%stts/voice", config.topic);
    if (!mqtt_publish_once(topic, message))
        _syslog(LOG_ERR, "Error: mosquitto_publish failed\n");

    sprintf(message, "%s", tts.emotion);
    sprintf(topic, "%stts/emotion", config.topic);
    if (!mqtt_publish_once(topic, message))
        _syslog(LOG_ERR, "Error: mosquitto_publish failed\n");

    sprintf(message, "%d", tts.speed);
    sprintf(topic, "%stts/speed", config.topic);
    if (!mqtt_publish_once(topic, message))
        _syslog(LOG_ERR, "Error: mosquitto_publish failed\n");

    if (config.auto_discovery)
    {
        json_object *root;

        topic = malloc(strlen(config.topic) + 50);
        message = malloc(500);

        root = json_object_new_array();
        int i;
        for (i = 0; lvoices[i] != NULL; ++i)
        {
            sprintf(message, "%s %s %s", lvoices[i], lvoicessex[i], lvoiceslang[i]);
            json_object_array_add(root, json_object_new_string(message));
        }
        sprintf(message, "%s", json_object_to_json_string_ext(root, 0));
        json_object_put(root);
        sprintf(topic, "%stts/voice/list", config.topic);
        if (!mqtt_publish_once(topic, message))
            _syslog(LOG_ERR, "Error: mosquitto_publish failed\n");

        root = json_object_new_array();
        for (i = 0; lemotions[i] != NULL; ++i)
        {
            sprintf(message, "%s", lemotions[i]);
            json_object_array_add(root, json_object_new_string(message));
        }
        sprintf(message, "%s", json_object_to_json_string_ext(root, 0));
        json_object_put(root);
        sprintf(topic, "%stts/emotion/list", config.topic);
        if (!mqtt_publish_once(topic, message))
            _syslog(LOG_ERR, "Error: mosquitto_publish failed\n");
    }
    free(topic);
    free(message);
}
int tts_on_message(char *id, char *payload, int len){
	struct json_tokener *tok;
	struct json_object *jobj;
    tts.updatecache=0;
    if (strcmp(id, "tts/set") == 0 || strcmp(id, "say") == 0 || strcmp(id, "tts/say") == 0 || strcmp(id, "tts") == 0)
    {
    	if (strchr(payload, '{')) {
    		char *text;
            enum json_type type;
            tok = json_tokener_new();	
            if (!tok)		return 0;
            jobj = json_tokener_parse_ex(tok, (char *)payload, len);	
            if (tok->err != json_tokener_success)	{
	            _syslog(LOG_ERR, "json fail %s \n", payload);
	            return 0;
	        }	
            tts_cache(config.cache_all);
            json_tokener_free(tok);
            json_object_object_foreach(jobj, key, val)
            {
                type = json_object_get_type(val);
                if (strcmp(key, "text") == 0 || strcmp(key, "say") == 0)
                {
                	text=strdup(json_object_get_string(val));
                }
                if (strcmp(key, "cache") == 0)
                {
                    tts_cache(json_object_get_int(val));
                }
                if (strcmp(key, "updatecache") == 0)
                {
                    tts.updatecache=1;
                }
                if (strcmp(key, "voice") == 0)
                {
                    tts_voice(json_object_get_string(val));
                }
                if (strcmp(key, "speed") == 0)
                {
                    tts_speed(json_object_get_int(val));
                }
                if (strcmp(key, "emotion") == 0)
                {
                    tts_emotion(json_object_get_string(val));
                }
            }
            json_object_put(jobj);
            tts_say(text);
            free(text);
        }else
        	tts_say(payload);
    }
    else if (strcmp(id, "tts/voice/to") == 0 || strcmp(id, "tts/voice/set") == 0 || strcmp(id, "ttsvoice/set") == 0)
    {
        tts_voice(payload);
    }
    else if (strcmp(id, "tts/emotion/to") == 0 || strcmp(id, "tts/emotion/set") == 0 || strcmp(id, "ttsemotion/set") == 0)
    {
        tts_emotion(payload);
    }
    else if (strcmp(id, "tts/speed/to") == 0 || strcmp(id, "tts/speed/set") == 0 || strcmp(id, "ttsspeed/set") == 0)
    {
        tts_speed(atoi(payload));
    }
    else if (strcmp(id, "tts/cache") == 0 || strcmp(id, "tts/cache/set") == 0 || strcmp(id, "ttscache/set") == 0)
    {
        tts_cache(atoi(payload));
    }
    else
        return 0;
    return 1;
}
void tts_say(char *text)
{
	if(tts.text != NULL) free(tts.text);

    if (strlen(text) == 0)
        return;
    _syslog(LOG_INFO, "tts say start %s\n", text);
   	tts.text = strdup(text);
   	if (tts_thread)
   	{
       	pthread_kill(tts_thread, SIGINT);
   	}
   	int status = pthread_create(&tts_thread, NULL, ttssay_thread, NULL);
   	if (status != 0)
   	{
       	_syslog(LOG_ERR, "tts error: can't create thread, status = %d\n", status);
   	}
}
void tts_voice(char *text)
{
    _syslog(LOG_INFO, "try tts voice set to %s\n", text);

    int i;
    for (i = 0; lvoices[i] != NULL; ++i)
        if (0 == strcmp(text, lvoices[i]))
        {
            _syslog(LOG_INFO, "tts voice set to %s\n", text);
            tts.voice = strdup(text);
            return;
        }
}
void tts_cache(uint8_t state)
{
    _syslog(LOG_INFO, "try tts cache set to %d\n", state);
    tts.cache = state>0 ? 1 : 0 ;
}
void tts_emotion(char *text)
{
    _syslog(LOG_INFO, "try tts emotion set to %s\n", text);

    int i;
    for (i = 0; lemotions[i] != NULL; ++i)
        if (0 == strcmp(text, lemotions[i]))
        {
            _syslog(LOG_INFO, "tts emotion set to %s\n", text);
            tts.emotion = strdup(text);
        }
}
void tts_speed(uint8_t speed)
{
    _syslog(LOG_INFO, "try tts speed set to %d.%d\n", (int)(speed / 10), speed % 10);

    if (speed > 1 && speed < 30)
    {
        _syslog(LOG_INFO, "tts speed set to %d\n", speed);
        tts.speed = speed;
    }
}

void *ttssay_thread(void *args)
{
    _syslog(LOG_INFO, "tts start %s\n", tts.text);
    char *cachefname = malloc(strlen(config.cache_tts_path) + MD5_DIGEST_LENGTH * 2 + 11 + 1);
    sprintf(cachefname, "/tmp/tts.tmp");
    int google=0;

    if (strlen(config.ya_tts_api_key) < 10 || strncmp(tts.voice, "google",6) == 0) 
        google = 1;

	_syslog(LOG_INFO, "tts engine %d\n", google);

    if(strlen(config.cache_tts_path)>0) {
        FILE *file;
        //mkdir(config.cache_tts_path, S_IRWXU);
        char hash[MD5_DIGEST_LENGTH];
        MD5(tts.text, strlen(tts.text), hash);
        //cachefname = malloc(strlen(config.cache_tts_path) + MD5_DIGEST_LENGTH * 2 + 11);
        sprintf(cachefname, "%stts-%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x-%d.mp3", config.cache_tts_path, 
        hash[0], hash[1], hash[2], hash[3], hash[4], hash[5], hash[6], hash[7], hash[8], hash[9], hash[10], hash[11], hash[12], hash[13], hash[14], hash[15], google);
        if (tts.updatecache == 0 && (file = fopen(cachefname, "r")))
        {
            fclose(file);
            if(google != 0) {
#ifdef USE_MPD
                mpd_play(cachefname);
#else
                call_play(cachefname,strlen(cachefname),1);
#endif
            }else{
                char *sst = malloc(strlen(cachefname) + 43);
                sprintf(sst, "aplay -traw -c1 -r48000 -fS16_LE -D alert %s", cachefname);
                system(sst);
                free(sst);
            }

            free(cachefname);
            return 1;
        }
	    if(tts.cache == 0) 
    		sprintf(cachefname, "/tmp/tts.tmp");
	    else if(config.cache_make_index == 1 && tts.updatecache == 0) {
	    	FILE *fp2;
	    	char *cacheindex = malloc(strlen(config.cache_tts_path) + 14);
	    	sprintf(cacheindex, "%stts-index.txt", config.cache_tts_path);
    		fp2 = fopen(cacheindex, "a+");
    		if (!fp2) {
    			printf("Unable to open/detect file(s)\n");
    		}
    		fprintf(fp2, "%s\t%s\n", cachefname, tts.text);
    		fclose(fp2);
    	}
    }
    int err;
    SSL_CTX *ctx;
    SSL *ssl;
    SSL_METHOD *meth;
    OpenSSL_add_ssl_algorithms();
    meth = TLS_client_method();
    SSL_load_error_strings();
    ctx = SSL_CTX_new(meth);
    if ((ctx) == NULL)
    {
        _syslog(LOG_ERR, "ssl ctx error");
        return -1;
    }
    int port = 443;
    char *temp = malloc(strlen(tts.text) * 6 + 11);
    temp = URIEncode(tts.text);

    char *file = malloc(609 + strlen(config.ya_tts_api_key) + strlen(config.ya_tts_folder_id) + strlen(temp));
    if(google != 0) {
        sprintf(file, "GET /translate_tts?ie=UTF-8&tl=ru&client=tw-ob&q=%s HTTP/1.1\r\nHost: translate.google.com\r\nUser-Agent: lumimqttd/%s (lumi)\r\nAccept: */*\r\nAccept-Encoding: identity\r\nConnection: Keep-Alive\r\n\r\n",temp, VERSION);
        _syslog(LOG_DEBUG, "get: %s\n\n", file);

    }else{
        sprintf(file, "POST /speech/v1/tts:synthesize HTTP/1.1\r\nHost: tts.api.cloud.yandex.net\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: %d\r\nAuthorization: Api-Key %s\r\n\r\nlang=ru-RU&folderId=%s&voice=%s&emotion=%s&speed=%d.%d&format=lpcm&sampleRateHertz=48000&text=%s\r\n", 
            strlen(config.ya_tts_folder_id) + strlen(temp) + 78 + 3 + 7 + strlen(tts.emotion) + strlen(tts.voice),
            config.ya_tts_api_key, config.ya_tts_folder_id, tts.voice, tts.emotion, (int)(tts.speed / 10), tts.speed % 10, temp);
        _syslog(LOG_DEBUG, "post: %s\n\n", file);
    }
    free(temp);

    struct hostent *hent;

    if (google != 0) {
        hent = gethostbyname("translate.google.com");
    }else{
        hent = gethostbyname("tts.api.cloud.yandex.net");
	}
    if( hent == NULL ) {
        _syslog(LOG_ERR, "gethostbyname error");
        return -1;
    }
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        _syslog(LOG_ERR, "socket error");
        return -1;
    }
    struct sockaddr_in add;
    add.sin_family = AF_INET;
    add.sin_port = htons(port);
    memcpy(&add.sin_addr, hent->h_addr_list[0], hent->h_length);
    if (connect(sock, (struct sockaddr *)&add, sizeof(add)))
    {
        _syslog(LOG_ERR, "connect error");
        return -1;
    }
    ssl = SSL_new(ctx);
    if ((ssl) == NULL)
    {
        _syslog(LOG_ERR, "ssl error");
        return -1;
    }

    SSL_set_fd(ssl, sock);
    err = SSL_connect(ssl);
    if ((err) == -1)
    {
        _syslog(LOG_ERR, "ssl connect error");
        return -1;
    }

    SSL_write(ssl, file, strlen(file));
    FILE *tfd = fopen(cachefname, "w+");
    int z, i, k, chunk_length = 0;

    unsigned char buf[BUFSZ];
    unsigned char *start;
    unsigned char *chunk;
    int recv_size;

    do {
        z = SSL_read(ssl, buf, BUFSZ);
        if ((z) == -1)
        {
            _syslog(LOG_ERR, "ssl read error");
            return -1;
        }

        start = strstr(buf, "\r\n\r\n");
        //_syslog(LOG_ERR, "server z: %d\nstart: %d\n", z, start);
        //_syslog(LOG_ERR, "server says: %s\n", buf);
    } while(start==0);


    chunk = strstr(start + 4, "\r\n");
    chunk_length = (int)strtol(start + 4, NULL, 16);
    recv_size = z - ((chunk + 2) - (unsigned char *)buf);
	
	if(chunk_length==0) {
        z = SSL_read(ssl, buf, BUFSZ);
        if ((z) == -1)
        {
            _syslog(LOG_ERR, "ssl read error");
            return -1;
        }
		chunk = strstr(buf, "\r\n");
	    chunk_length = (int)strtol(buf, NULL, 16);
    	recv_size = z - ((chunk + 2) - (unsigned char *)buf);
	}

    fwrite(chunk + 2, 1, recv_size, tfd);
    
    //_syslog(LOG_ERR, "server chunk_length: %d\nrecv_size: %d\n%d %d %d (-)%d\nsays: %s\n", chunk_length, recv_size ,z, start, buf, (start-buf), buf);
    
    int total_recv = recv_size;
    i = 1;
    while (1)
    {
        z = SSL_read(ssl, buf, BUFSZ);
        recv_size += z;
        if (z <= 0)
            break;
        if (recv_size > chunk_length)
        {
            if (recv_size - chunk_length <= 2)
            {
                fwrite(buf, 1, z - (recv_size - chunk_length), tfd);
                recv_size = chunk_length;
                continue;
            }
            i++;
            start = buf;
            if ((z - (recv_size - chunk_length)) != 0)
            {
                for (k = (z - (recv_size - chunk_length)) - 10; k < z; k++)
                {
                    if (buf[k] == '\n' && buf[k - 1] == '\r')
                    {
                        start = buf + k + 1;
                        break;
                    }
                }
            }
            for (k = (start - (unsigned char *)buf); k < z; k++)
            {
                if (buf[k] == '\n' && buf[k - 1] == '\r')
                {
                    chunk = buf + k - 1;
                    break;
                }
            }

            fwrite(buf, 1, ((start + 0) - (unsigned char *)buf), tfd);
            total_recv += ((start + 0) - (unsigned char *)buf);
            recv_size = z - ((chunk + 2) - (unsigned char *)buf);
            chunk_length = (int)strtol(start, NULL, 16);
            fwrite(chunk + 2, 1, z - ((chunk + 2) - (unsigned char *)buf), tfd);
            total_recv += recv_size;
            if (chunk_length == 0)
                break;
        }
        else
        {
            fwrite(buf, 1, z, tfd);
            total_recv += z;
        }
    }
    _syslog(LOG_DEBUG, "end %d chunks; recived %d\n", i, total_recv);
    SSL_shutdown(ssl); /* send SSL/TLS close_notify */

    close(sock);
    free(file);
    fclose(tfd);
    if(google != 0) {
#ifdef USE_MPD
        mpd_play(cachefname);
#else
        call_play(cachefname,strlen(cachefname),1);
#endif
    }else{
        char *sst = malloc(strlen(cachefname) + 43);
        sprintf(sst, "aplay -traw -c1 -r48000 -fS16_LE -D alert %s", cachefname);
        system(sst);
        free(sst);
    }
    _syslog(LOG_INFO, "tts end\n");
    free(cachefname);
    tts_cache(config.cache_all);
    return 0;
}

int isURIChar(const char ch)
{
    if (isalnum(ch))
        return 1;
    if (ch == ' ')
        return 0;
    switch (ch)
    {
    case '$':
    case '-':
    case '_':
    case '.':
    case '+':
    case '!':
    case '*':
    case '\'':
    case '(':
    case ')':
        return 1;
    default:
        return 0;
    }
}
char *URIEncode(const char *uri)
{
    int n = strlen(uri);
    char *cp = (char *)malloc(n * 6);
    memset(cp, 0, n * 6);
    int pn = 0;
    // scan uri
    for (int i = 0; i < n; i++)
    {
        if (isURIChar(uri[i]))
        {
            cp[pn++] = uri[i];
            continue;
        }
        char it[4] = {0};
        if ((unsigned char)uri[i] < 0x10)
            sprintf(it, "%%0%X", (unsigned char)uri[i]);
        else
            sprintf(it, "%%%X", (unsigned char)uri[i]);
        strcat(cp, it);
        pn += 3;
    }
    return cp;
}
