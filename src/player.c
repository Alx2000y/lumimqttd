#include <syslog.h>
#include <stdarg.h>
#include "player.h"
#include "log.h"

int play_on_message(char *id, char *payload, int len) 
{
    char *temp = NULL;
    if (strcmp(id, "sound/set") == 0 || strcmp(id, "play/set") == 0)
    {
        _syslog(LOG_INFO, "sound %s %s\n", id, payload);
        if (len > 0)
        {
            temp = malloc(len + 1);
            strncpy(temp, payload, len);
        }
        else
        {
            temp = NULL;
        }
#ifdef USE_MPD
        mpd_play(temp);
#else
        call_play(temp,len,0);
#endif
        _syslog(LOG_INFO, "sound ended\n");
        if(temp != NULL)
        	free(temp);
    } else
        return 0;
    return 1;
}
void sockpipe_thread(int tfd)
{
    int z;
    unsigned char buf[PIPE_BUF];
    z = read(sock, buf, PIPE_BUF);
    unsigned char *start = strstr(buf, "\r\n\r\n");
    write(tfd, start + 4, z - ((start + 4) - (unsigned char *)buf));
    while (1)
    {
        z = read(sock, buf, PIPE_BUF);
        if (z <= 0 || stopplay == 1)
            break;
        write(tfd, buf, z);
    }
    _syslog(LOG_INFO, "sockpipe ssl end 0: %d\n",PIPE_BUF);

    close(sock);
    close(tfd);
    _syslog(LOG_INFO, "sockpipe end: %d\n",PIPE_BUF);

}

void sockpipessl_thread(int tfd)
{
    int z;
    unsigned char buf[PIPE_BUF];
    z = SSL_read(ssl, buf, PIPE_BUF);
    unsigned char *start = strstr(buf, "\r\n\r\n");
    write(tfd, start + 4, z - ((start + 4) - (unsigned char *)buf));
    while (1)
    {
        z = SSL_read(ssl, buf, PIPE_BUF);
        if (z <= 0 || stopplay == 1)
            break;
        write(tfd, buf, z);
    }
    _syslog(LOG_INFO, "sockpipe ssl end 0: %d\n",PIPE_BUF);

    SSL_shutdown(ssl);
    close(sock);
    close(tfd);
    _syslog(LOG_INFO, "sockpipe ssl end: %d\n",PIPE_BUF);

}

int return_sock(char *url)
{
    SSL_CTX *ctx;
    SSL_METHOD *meth;

    char *server = NULL;
    char *file = NULL, *sport = NULL;
    int i, len = strlen(url), port = 80, https = 0, ret=0;
    if (strncmp(url, "https", 5) == 0)
    {
        https = 1;
        port = 443;
    }

    for (i = 7 + https; (i < len) && (url[i] != '/'); i++)
        (url[i] == ':') && (sport = &url[i + 1]);
    if (sport)
    {
        url[i] = 0;
        port = atoi(sport);
        url[i] = '/';
    }
    server = (char *)malloc(sizeof(char) * (i - 7 - https));
    memcpy(server, &url[7 + https], (sport) ? (sport - url - 8 - https) : (i - 7 - https));
    server[(sport) ? (sport - url - 8 - https) : (i - 7 - https)] = 0;

    file = (char *)malloc(sizeof(char) * (len - i + 1 + 25) + strlen(server));
    (i == len) && (url[--i] = '/');
    sprintf(file, "GET %s HTTP/1.1\r\nHost: %s\r\n\r\n", url + i, server);
    if (https)
    {
        OpenSSL_add_ssl_algorithms();
        meth = TLS_client_method();
        SSL_load_error_strings();
        ctx = SSL_CTX_new(meth);
        if ((ctx) == NULL)
        {
            _syslog(LOG_ERR, "ssl ctx error");
            free(server);
            free(file);
            return -1;
        }
    }
    struct hostent *hent;
    _syslog(LOG_INFO, "server: %s port: %d file: %s\n", server, port, file);
    if ((hent = gethostbyname(server)) == NULL)
    {
        _syslog(LOG_ERR, "gethostbyname error %s", server);
        ret = -1;
        goto out;
    }
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        _syslog(LOG_ERR, "socket error");
        ret = -1;
        goto out;
    }
    struct sockaddr_in add;
    add.sin_family = AF_INET;
    add.sin_port = htons(port);
    memcpy(&add.sin_addr, hent->h_addr_list[0], hent->h_length);
    if (connect(sock, (struct sockaddr *)&add, sizeof(add)))
    {
        _syslog(LOG_ERR, "connect error");
        ret = -1;
        goto out;
    }
    if (https)
    {
        ssl = SSL_new(ctx);
        if ((ssl) == NULL)
        {
            _syslog(LOG_ERR, "ssl error");
	        ret = -1;
    	    goto out;
        }

        SSL_set_fd(ssl, sock);
        i = SSL_connect(ssl);
        if ((i) == -1)
        {
            _syslog(LOG_ERR, "ssl connect error");
    	    ret = -1;
	        goto out;
        }

        SSL_write(ssl, file, strlen(file));
    }
    else
    {
        write(sock, file, strlen(file));
    }
    pipe(pfd);
    mpg123_open_fd_64(mh, pfd[0]);

    if (https)
    {
        pthread_create(&sockplay_thread, NULL, (void *(*)(void *))sockpipessl_thread, (void *)pfd[1]);
    }
    else
    {
        pthread_create(&sockplay_thread, NULL, (void *(*)(void *))sockpipe_thread, (void *)pfd[1]);
    }
    pthread_detach(sockplay_thread);
out:
    free(server);
    free(file);

    return ret;
}
void exit_play()
{
    close(pfd[0]);
    out123_del(ao);
    mpg123_close(mh);
    mpg123_delete(mh);
    _syslog(LOG_INFO, "mpg123_exit\n");
    mpg123_exit();
}
void stop_play()
{
    int status;
    _syslog(LOG_INFO, "stop play thread\n");
    if (play_thread)
    {
        stopplay = 1;
        status = pthread_join(play_thread, NULL);
        if (status != 0)
        {
            _syslog(LOG_ERR, "can't join play thread, status = %d\n", status);
        }
        _syslog(LOG_INFO, "play thread ended\n");
        stopplay = 0;
        play_thread = 0;
        if (playargs.playfile)
            free(playargs.playfile);
    }
}
void call_play(char *playfile, int len, int channel)
{
    int status;
    stop_play();
    if (playfile != NULL && len > 0)
    {
        playargs.playfile = malloc(len + 1);
        strncpy(playargs.playfile, playfile, len);
        playargs.playfile[len]=0;
        playargs.slen=len;
        playargs.channel = channel;
        playargs.action = 1;
        _syslog(LOG_INFO, "call_play thread create %s\n", playfile);
        status = pthread_create(&play_thread, NULL, playc, (void*) &playargs);
        if (status != 0)
        {
            _syslog(LOG_ERR, "can't create play thread, status = %d\n", status);
        }
    }

    _syslog(LOG_INFO, "call_play ended\n");
}
void *playc(void *args)
{
    int ret = 0;
    char *outfile = NULL;
    playArgs_t *playarg = (playArgs_t*) args;
    _syslog(LOG_INFO, "play %s [%d]\n", playarg->playfile, playarg->slen);
    if(playarg->channel!=0)
        outfile=strdup("alert");
    ret = play(playarg->playfile, outfile);
    _syslog(LOG_INFO, "end play %d\n", ret);
    if(outfile != NULL) free(outfile);
    return 0;
}
int play(char *playfile, char *outfile)
{
    _syslog(LOG_INFO, "play %s\n", playfile);

    int channels, encoding;
    long rate;
    unsigned char *buf = NULL;
    size_t buf_size = 0;
    char *driver = "alsa";
    
    int err = MPG123_OK;
    off_t samples = 0;
    int framesize = 1;
    size_t done = 0;

    stopplay = 0;
    mpg123_init();
    _syslog(LOG_INFO, "mpg123_init ended\n");
    if ((mh = mpg123_new(NULL, NULL)) == NULL)
    {
        _syslog(LOG_ERR, "mpg123_new");
        exit_play();
        return -1;
    }
    ao = out123_new();
    if (!ao)
    {
        _syslog(LOG_ERR, "Cannot create output handle.\n");
        exit_play();
        return -1;
    }
    _syslog(LOG_INFO, "out123_new ended\n");
    int res = 0;

    if (strncmp(playfile, "http://", 7) == 0 || strncmp(playfile, "https://", 8) == 0)
    {
        res = return_sock(playfile);
    }
    else
    {
        res = mpg123_open(mh, playfile);
    }
    _syslog(LOG_INFO, "mpgopen %d\n", res);
    if (res < 0)
    {
        exit_play();
        return -1;
    }
    if (out123_open(ao, driver, outfile) != OUT123_OK)
    {
        _syslog(LOG_ERR, "Trouble with out123: %s\n", out123_strerror(ao));
        exit_play();
        return -1;
    }
    out123_driver_info(ao, &driver, &outfile);
    _syslog(LOG_INFO, "Effective output driver: %s\n", driver ? driver : "<nil> (default)");
    _syslog(LOG_INFO, "Effective output file:   %s\n", outfile ? outfile : "<nil> (default)");

    _syslog(LOG_INFO, "mpgcheck %d\n", res);

    mpg123_getformat(mh, &rate, &channels, &encoding);
    _syslog(LOG_INFO, "mpgformat %d %d %d\n", rate, channels, encoding);
	if(config.verbosity > 4) {
	    _syslog(LOG_INFO, "Playing with %i channels and %li Hz, encoding %s.\n", channels, rate, out123_enc_name(encoding));
	}
    if (out123_start(ao, rate, channels, encoding) || out123_getformat(ao, NULL, NULL, NULL, &framesize))
    {
        _syslog(LOG_ERR, "Cannot start output / get framesize: %s\n", out123_strerror(ao));
        exit_play();
        return -1;
    }

    buf_size = mpg123_outblock(mh);
    buf = malloc(buf_size + 1);

    do
    {
        size_t played;
        err = mpg123_read(mh, buf, buf_size, &done);
        played = out123_play(ao, buf, done);
        if (played != done)
        {
            _syslog(LOG_ERR, "Warning: written less than gotten from libmpg123: %li != %li\n", (long)played, (long)done);
        }
        samples += played / framesize;
    } while (done && err == MPG123_OK && stopplay != 1);

    free(buf);

    if (err != MPG123_DONE)
        _syslog(LOG_ERR, "Warning: Decoding ended prematurely because: %s\n", err == MPG123_ERR ? mpg123_strerror(mh) : mpg123_plain_strerror(err));

    _syslog(LOG_INFO, "%li samples written.\n", (long)samples);
    close(pfd[0]);

    exit_play();

    _syslog(LOG_INFO, "play end %d\n", res);

    return 0;
}
