#include "lumimqttd.h"

void on_connect(struct mosquitto *mosq, void *obj, int ret)
{
    int err, qos = 1;
    char *id = malloc(strlen(config.topic)+2);
    char *lwt = malloc(strlen(config.topic)+8);

    UNUSED(obj);
    UNUSED(ret);

    sprintf(id, "%s%s", config.topic, "#");
    sprintf(lwt, "%s%s", config.topic, "status");
    _syslog(LOG_INFO, "Connected waiting for Published Messages \n");

    err = mosquitto_publish(mosq, NULL, lwt, 6, "online", 1, true);
    if (err != MOSQ_ERR_SUCCESS)
        _syslog(LOG_ERR, "Error: mosquitto_publish lwt failed [%s]\n", mosquitto_strerror(err));

    _syslog(LOG_INFO, "Connect call SUCCESS subscribe: %s lwt: %s.\n", id, lwt);
    err = mosquitto_subscribe(mosq, NULL, id, qos);
    if (err != MOSQ_ERR_SUCCESS)
    {
        _syslog(LOG_INFO, "Subscribe filed %s\n", id);
    }
}
void on_disconnect(struct mosquitto *clnt_inst, void *obj, int ret)
{
    UNUSED(clnt_inst);
    UNUSED(obj);
    _syslog(LOG_INFO, "Mosq disconnected %d\n",ret);
}
void on_log_callback(struct mosquitto *clnt_inst, void *obj, int level, const char *str)
{
    UNUSED(clnt_inst);
    UNUSED(obj);
    UNUSED(level);
    _syslog(LOG_DEBUG, "mosqcb: %s\n", str);
}
void on_message(struct mosquitto *clnt_inst, void *obj, const struct mosquitto_message *msg)
{
    char *id = NULL;
    if (strncmp(config.topic, msg->topic, strlen(config.topic)) == 0)
    {
        id = strdup(msg->topic + strlen(config.topic));
    }
    else
        return;

    if (leds_on_message(id, (char *)msg->payload, msg->payloadlen))
    {
        _syslog(LOG_INFO, "leds %s %s \n", id, (char *)msg->payload);
    }
    else if (volume_on_message(id, (char *)msg->payload, msg->payloadlen))
    {
        _syslog(LOG_INFO, "volume %s %s \n", id, (char *)msg->payload);
    }
    else if (play_on_message(id, (char *)msg->payload,msg->payloadlen))
    {
        _syslog(LOG_INFO, "sound %s %s \n", id, (char *)msg->payload);
    }
    else if (msg->payloadlen > 0 && tts_on_message(id, (char *)msg->payload,msg->payloadlen)){
        _syslog(LOG_INFO, "tts %s %s \n", id, (char *)msg->payload);
    }
    else
    {
        _syslog(LOG_INFO, "Topic :: %s Message Length :: %d\nReceived Message :: %s\n", msg->topic, msg->payloadlen, (char *)msg->payload);
    }
    _syslog(LOG_DEBUG, "on_message ended\n");
    free(id);
}
bool mqtt_publish(const char *topic, const char *message)
{
    bool success = true;
    int err;
    if (mosq)
    {
        _syslog(LOG_INFO, "%s %s \n", topic, message);
        err = mosquitto_publish(mosq, NULL, topic, strlen(message), message, 0, config.mqtt_retain == 1);
        if (err != MOSQ_ERR_SUCCESS)
        {
            _syslog(LOG_ERR, "Error: mosquitto_publish failed [%s]\n", mosquitto_strerror(err));
            success = false;
        }
    }
    else
    {
        _syslog(LOG_ERR, "Error: mosq == NULL, Init failed?\n");
        success = false;
    }

    return success;
}
bool mqtt_publish_once(const char *topic, const char *message)
{
    bool success = true;
    int err;
    if (mosq)
    {
        _syslog(LOG_INFO, "%s %s \n", topic, message);
        err = mosquitto_publish(mosq, NULL, topic, strlen(message), message, 0, 0);
        if (err != MOSQ_ERR_SUCCESS)
        {
            _syslog(LOG_ERR, "Error: mosquitto_publish failed [%s]\n", mosquitto_strerror(err));
            success = false;
        }
    }
    else
    {
        _syslog(LOG_ERR, "Error: mosq == NULL, Init failed?\n");
        success = false;
    }

    return success;
}
void *periodical_thread(void *args)
{
    while (1)
    {
        sleep(config.readinterval);
        periodical_check();
    }
}
void periodical_check(void)
{
    char *topic;
    char *message;
    uint16_t lux = 0;
#ifdef USE_CPUTEMP
    uint32_t cputemp = 0;
#endif
    static int period = 0;
#ifdef USE_CPUTEMP
	if(config.disable_cputemp==0) {
        if (cputempfile != NULL)
        {
            rewind(cputempfile);
            fscanf(cputempfile, "%lu", &cputemp);
        }
        if (period == 0 && cputemp == 0)
        {
            if (cputempfile != NULL)
                fclose(cputempfile);
            cputempfile = fopen(config.cputemp_file, "r");
            if (cputempfile == NULL)
            {
                _syslog(LOG_ERR, "could not read %s:%s", config.cputemp_file, strerror(errno));
            }
        }

        if (curstate.cputemp > cputemp + config.cputemp_treshold || curstate.cputemp < cputemp - config.cputemp_treshold)
        {
            _syslog(LOG_INFO, "Last cputemp %d; new %d;\n", curstate.cputemp, cputemp);
            curstate.cputemp = cputemp;
            topic = malloc(strlen(config.topic) + 8);
            message = malloc(13);
            sprintf(topic, "%s%s", config.topic, "cputemp");
            sprintf(message, "%d.%d", cputemp / 1000, cputemp % 1000);
            mqtt_publish(topic, message);
            free(topic);
            free(message);
        }
	    //_syslog(LOG_INFO, "Periodical cputemp end\n");
    }
#endif
    if(config.disable_illuminance==0) {
        if (luxfile != NULL)
        {
            rewind(luxfile);
            fscanf(luxfile, "%hu", &lux);
        }
        if (period == 0 && lux == 0)
        {
            if (luxfile != NULL)
                fclose(luxfile);
            luxfile = fopen(config.lux_file, "r");
            if (luxfile == NULL)
            {
                _syslog(LOG_ERR, "could not read %s:%s", config.lux_file, strerror(errno));
            }
        }

        if (lux > 0 && (((lux > 20 || curstate.lux > 20) && (curstate.lux * 100 > lux * (100 + config.treshold) || curstate.lux * 100 < lux * (100 - config.treshold))) || period == 0))
        {
            _syslog(LOG_INFO, "Last lux %d; new %d; treshold=%d\n", curstate.lux, lux, config.treshold);
            curstate.lux = lux;
            topic = malloc(strlen(config.topic) + 13);
            message = malloc(13);
            sprintf(topic, "%s%s", config.topic, "illuminance");
            sprintf(message, "%d", lux / 4);
            mqtt_publish(topic, message);
            free(topic);
            free(message);
        }
	    //_syslog(LOG_INFO, "Periodical illuminance end\n");
    }
#ifdef USE_BLE
    ble_periodical_check();
//    _syslog(LOG_INFO, "Periodical bt end\n");
#endif
    leds_periodical_check();
//    _syslog(LOG_INFO, "Periodical leds end\n");
    volume_periodical_check();
//    _syslog(LOG_INFO, "Periodical volume end\n");

    period++;
    if (period > 600)
        period = 0;
}
void *button_check(void *args)
{
    struct input_event ev;
	unsigned int size;
    int start=0, interval=99999, hsleep=100, clickn=0;
    int fd, grabbed;
    char *topic;
    char *message;

    if ((fd = open("/dev/input/event0", O_RDONLY)) < 0) {
		perror("");
		if (errno == EACCES && getuid() != 0) {
			_syslog(LOG_ERR, "You do not have access to /dev/input/event0. Try running as root instead.\n");
		}
		return;
	}

	grabbed = ioctl(fd, EVIOCGRAB, (void *) 1);
	ioctl(fd, EVIOCGRAB, (void *) 0);
	if (grabbed) {
		_syslog(LOG_ERR, "This device is grabbed by another process.\n");
		return;
	}
    topic = malloc(strlen(config.topic) + 4);
    sprintf(topic, "%s%s", config.topic, "btn");
    while (1) {
		size = read(fd, &ev, sizeof(struct input_event));
		if (size < sizeof(struct input_event)) {
			_syslog(LOG_ERR, "error reading: expected %u bytes, got %u\n", sizeof(struct input_event), size);
            free(topic);
			return;
		}
		_syslog(LOG_INFO, "Event: time %ld.%06ld, ", ev.input_event_sec, ev.input_event_usec);
		_syslog(LOG_INFO, "type: %i, code: %i, value: %i\n", ev.type, ev.code, ev.value);
        if(ev.type==1) {
            hsleep=1;
            if(ev.value==1) {
                start=(ev.input_event_sec)*1000 + (ev.input_event_usec)/1000;
                if(interval>50 && interval<300) clickn++;
            }else{
                interval=(ev.input_event_sec)*1000 + (ev.input_event_usec)/1000 - start;
            }
            message = malloc(13);
            sprintf(message, "%d", ev.value);
            mqtt_publish(topic, message);
            free(message);
        }
        if(interval>60000) {
            start=0;
            hsleep=100;
            clickn=0;
        }
        usleep(hsleep*1000);
	}
    free(topic);
                // @mrG1K wrote:
                // type 1 code 256 value 1 or 0

                // 300 ms value 1 без 0 = hold
                // если state hold то при value 0 = event release

                // single 1, 0 без ожидания. если следующий event с 1 не пришел в течении Х ms
                // или как у Ивана) https://github.com/openlumi/lumimqtt/blob/main/lumimqtt/button.py

}

void auto_discover(void)
{
    _syslog(LOG_INFO, "Autodiscover started\n");
    leds_auto_discover();
    _syslog(LOG_INFO, "Leds discovered\n");
    volume_auto_discover();
    _syslog(LOG_INFO, "Volume discovered\n");
    if(config.disable_illuminance==1 && config.disable_cputemp==1) {
    	return;
    }
    char *topic, *message;
    topic = malloc(1000);
    message = malloc(1000);
    { //BTN
	    sprintf(topic, "\"device\": {\"identifiers\": [\"xiaomi_gateway_%s\"] ,\"name\": \"xiaomi_gateway_%s\", \"sw_version\": \"%s\", \"model\": \"Xiaomi Gateway\", \"manufacturer\": \"Xiaomi\"},\"availability_topic\": \"%sstatus\",",
        config.device_id, config.device_id, VERSION, config.topic);

        sprintf(message, "{\"name\": \"Button %s\", \"unique_id\": \"%s_btn\", %s \"state_topic\": \"%sbtn\", \"payload_on\": \"1\", \"payload_off\": \"0\"}",
            config.device_id, config.device_id, topic, config.topic);
        sprintf(topic, "homeassistant/binary_sensor/%s/btn/config", config.device_id);
        if (!mqtt_publish(topic, message))
            _syslog(LOG_ERR, "Error: mosquitto_publish failed\n");
	}
	if(config.disable_illuminance==0) {
	    sprintf(topic, "\"device\": {\"identifiers\": [\"xiaomi_gateway_%s\"] ,\"name\": \"xiaomi_gateway_%s\", \"sw_version\": \"%s\", \"model\": \"Xiaomi Gateway\", \"manufacturer\": \"Xiaomi\"},\"availability_topic\": \"%sstatus\",",
        config.device_id, config.device_id, VERSION, config.topic);

        sprintf(message, "{\"name\": \"illuminance %s\", \"unique_id\": \"%s_illuminance\", \"schema\": \"json\", %s \"state_topic\": \"%silluminance\",\"device_class\": \"illuminance\",\"unit_of_measurement\": \"lx\"}",
            config.device_id, config.device_id, topic, config.topic);
        sprintf(topic, "homeassistant/sensor/%s/illuminance/config", config.device_id);
        if (!mqtt_publish(topic, message))
            _syslog(LOG_ERR, "Error: mosquitto_publish failed\n");
    }
#ifdef USE_CPUTEMP
    if (config.disable_cputemp == 0) {
	    sprintf(topic, "\"device\": {\"identifiers\": [\"xiaomi_gateway_%s\"] ,\"name\": \"xiaomi_gateway_%s\", \"sw_version\": \"%s\", \"model\": \"Xiaomi Gateway\", \"manufacturer\": \"Xiaomi\"},\"availability_topic\": \"%sstatus\",",
        config.device_id, config.device_id, VERSION, config.topic);

        sprintf(message, "{\"name\": \"Cpu Temperature %s\", \"unique_id\": \"%s_cputemp\", \"schema\": \"json\", %s \"state_topic\": \"%scputemp\",\"device_class\": \"temperature\",\"unit_of_measurement\": \"C\"}",
            config.device_id, config.device_id, topic, config.topic);
        sprintf(topic, "homeassistant/sensor/%s/cputemp/config", config.device_id);
        if (!mqtt_publish(topic, message))
            _syslog(LOG_ERR, "Error: mosquitto_publish failed\n");
    }
#endif
    free(topic);
    free(message);

}

void daemonize(bool demo)
{
    pid_t PID, SID;
    int err, status_addr;
    char *lwt = malloc(strlen(config.topic)+8);

    bool clean_session = true;
    int major = 0, minor = 0, revision = 0;
    if (!demo)
    {
        openlog("lumimqttd", LOG_PERROR, LOG_SYSLOG);
        err = mosquitto_lib_version(&major, &minor, &revision);
        _syslog(LOG_INFO, "Lumimqtt Version: %s\n", VERSION);
        _syslog(LOG_INFO, "Mosquitto Library Version: %d.%d.%d\n", major, minor, revision);
    }
    else
    {
        openlog("lumimqttd", LOG_PID, LOG_SYSLOG); //LOG_NOWAIT
        _syslog(LOG_INFO, "Lumimqtt Version: %s\n", VERSION);
        PID = fork();
        if (PID > 0)
        {
            exit(EXIT_SUCCESS);
        }
        if (PID < 0)
        {
            exit(EXIT_FAILURE);
        }
        umask(0);

        if ((SID = setsid()) < 0)
        {
            _syslog(LOG_ERR, "Daemonize: Failed setsid");
            exit(EXIT_FAILURE);
        }

        if (chdir("/") < 0)
        {
            _syslog(LOG_ERR, "Daemonize: Failed chdir");
            exit(EXIT_FAILURE);
        }

        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);

        _syslog(LOG_INFO, "Daemonize: Success.");
    }
    init_leds();

    err = mosquitto_lib_init();
    if (err == MOSQ_ERR_SUCCESS)
    {
        mosq = mosquitto_new(config.device_id, clean_session, NULL);
        mosquitto_username_pw_set(mosq, config.mqtt_user, config.mqtt_user_pw);
        mosquitto_connect_callback_set(mosq, on_connect);
        mosquitto_disconnect_callback_set(mosq, on_disconnect);
        mosquitto_message_callback_set(mosq, on_message);
        //mosquitto_log_callback_set(mosq, on_log_callback);
    	sprintf(lwt, "%s%s", config.topic, "status");
    	if (mosquitto_will_set(mosq, lwt, 7, "offline", 1, true) != MOSQ_ERR_SUCCESS)
	    {
    	    _syslog(LOG_ERR, "LWT set filed %s\n", lwt);
	    }
        err = mosquitto_connect(mosq, config.mqtt_host, config.mqtt_port, config.mqtt_keepalive);
        if(err == MOSQ_ERR_SUCCESS) 
        {
            _syslog(LOG_INFO, "Connect call success.\n");
        }
        else if (err == MOSQ_ERR_INVAL)
        {
            _syslog(LOG_ERR, "Connect call invalid parameters.\n");
            return;
        }
        else if (err == MOSQ_ERR_ERRNO)
        {
            _syslog(LOG_ERR, "Connect call ERROR.\n");
            return;
        }else {
            _syslog(LOG_ERR, "Mosquitto err.\n");
            return;        
        }
    }
    else {
        _syslog(LOG_ERR, "Mosquitto init err.\n");
        return;        
    }


    err = pthread_create(&periodic_thread, NULL, periodical_thread, NULL);
    if (err != 0)
    {
        _syslog(LOG_ERR, "main error: can't create thread, status = %d\n", err);
        exit(EXIT_FAILURE);
    }

    err = pthread_create(&button_thread, NULL, button_check, NULL);
    if (err != 0)
    {
        _syslog(LOG_ERR, "main error: can't create thread, status = %d\n", err);
        exit(EXIT_FAILURE);
    }
    if (config.auto_discovery == 1) {
        auto_discover();
        init_tts();
        _syslog(LOG_INFO, "Init TTS finished\n");
        config.auto_discovery = 0;
        _syslog(LOG_INFO, "Autodiscover finished\n");
    }
    _syslog(LOG_DEBUG, "mosquitto loop.\n");
    err = mosquitto_loop_forever(mosq, -1, 1);
    if (err) {    
        _syslog(LOG_DEBUG, "MQTT is broken - %d\n", err);
    } 
    _syslog(LOG_DEBUG, "mosquitto loop started.\n");
    err = pthread_join(periodic_thread, (void **)&status_addr);
	if(config.disable_btn==0)
	    err = pthread_join(button_thread, (void **)&status_addr);

    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();

    if (demo)
        exit(EXIT_SUCCESS);
}

void parent_sigc(int signo)
{
    static int processing = 0;
    if (processing == 0)
    {
        processing = 1;
        
        int err;
        char *lwt = malloc(strlen(config.topic)+8);
        sprintf(lwt, "%s%s", config.topic, "status");
        err = mosquitto_publish(mosq, NULL, lwt, 7, "offline", 1, true);
        if (err != MOSQ_ERR_SUCCESS)
            _syslog(LOG_ERR, "Error: mosquitto_publish lwt failed [%s]\n", mosquitto_strerror(err));

        signo && printf("\ncaught sigint in parent -- wait a sec cleaning state\n");
#ifdef USE_BLE
        blescan_stop();
#endif
#ifndef USE_MPD
        if (play_thread)
        {
            exit_play();
        }
#endif

        if (signo && periodic_thread)
        {
            pthread_kill(periodic_thread, SIGINT);
        }
        if (signo && play_thread)
        {
            pthread_kill(play_thread, SIGINT);
        }
        else if (!signo && play_thread)
        {
            pthread_join(play_thread, NULL);
        }
        printf("close leds \n");
        led_sigc();
        printf("close files \n");
        if(config.disable_illuminance==0 && luxfile != NULL)
	        fclose(luxfile);
#ifdef USE_CPUTEMP
		if(config.disable_cputemp==0 && cputempfile != NULL)
	        fclose(cputempfile);
#endif
        printf("finita (%d)", signo);
        mosquitto_disconnect(mosq);
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
    }
    exit(0);
}

void handler_segv(int sig)
{
    //void *array[10];
    size_t size = 0;
    //size = backtrace(array, 10);
    fprintf(stderr, "Error: Signal %d; %zd frames found:\n", sig, size);
    //backtrace_symbols_fd(array, size, STDERR_FILENO);
    signal(sig, SIG_DFL);
    exit(1);
}
int main(int argc, char **argv)
{
    int opt = 0;
    char *options = "c:hd:"; // Can add options here.
    char *configfile = DEFAULT_CONFIG_FILE;
    int verbose=0;
    //signal(SIGSEGV, handler_segv);
    signal(SIGINT, parent_sigc);

    while (opt != -1)
    {
        opt = getopt(argc, argv, options);
        switch (opt)
        {
        case 'h':
            printf("Subscriber MQTT client.\n");
            printf("Options ::\t-h : help.\n");
            printf("\t\t-d : Daemonize the process.\n");
            printf("\t\t-с : path to config file.\n");
            return 0;
        case 'c':
            configfile = optarg;
            break;
        case 'v':
            verbose = atoi(optarg);
            break;
        case 'l':
            //logfile = optarg;
            break;
        case 'd':
            if (cfg_load(configfile) == -1)
            {
                printf("Error loading config.  Abortin\ng");
                return 0;
            }
            printf("Daemonizing this program.\n");
            if(verbose>0) {
            	printf("Verbosing set to %d.\n", verbose);
                config.verbosity=verbose;
            }
            daemonize(true);
            return 0;
        case '?':
            printf("Invalid option. Need some help.\n");
            printf("Subscriber MQTT client.\n");
            printf("Options ::\t-h : help.\n");
            printf("\t\t-d : Daemonize the process.\n");
            printf("\t\t-с : path to config file.\n");
            return 0;
        default:
            if (cfg_load(configfile) == -1)
            {
                printf("Error loading config.  Aborting\n");
                return 0;
            }
            cfg_dump();
            if(verbose>0) {
            	printf("Verbosing set to %d.\n", verbose);
                config.verbosity=verbose;
            }
            daemonize(false);
            return 0;
        }
    }
    return 0;
}
