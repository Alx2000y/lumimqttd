#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "cfg.h"
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <netdb.h>

cfg_t config;

char *getmac()
{
	char *buf = malloc(13);
	struct ifreq s;
	int fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
	strcpy(s.ifr_name, "wlan0");
	if (0 == ioctl(fd, SIOCGIFHWADDR, &s))
	{
		sprintf(buf, "%02x%02x%02x%02x%02x%02x",
				s.ifr_addr.sa_data[0], s.ifr_addr.sa_data[1], s.ifr_addr.sa_data[2],
				s.ifr_addr.sa_data[3], s.ifr_addr.sa_data[4], s.ifr_addr.sa_data[5]);
		return buf;
	}
	return "001";
}

int cfg_load(char *file)
{

	FILE *f = fopen(file, "rb");
	fseek(f, 0, SEEK_END);
	size_t fsize = (size_t)ftell(f);
	fseek(f, 0, SEEK_SET);
	memset(&config, 0, sizeof(config));

	config.mqtt_port = 1883;
	config.mqtt_host = strdup("127.0.0.1");
	config.mqtt_user = strdup("");
	config.mqtt_user_pw = strdup("");
	config.mqtt_keepalive = 60;
	config.mqtt_retain = 0;
	config.device_id = strdup(getmac());
	config.topic = strdup("lumi/{device_id}/");
	config.red_led = strdup("/sys/class/leds/red/brightness");
	config.green_led = strdup("/sys/class/leds/green/brightness");
	config.blue_led = strdup("/sys/class/leds/blue/brightness");
	config.lux_file = strdup("/sys/bus/iio/devices/iio:device0/in_voltage5_raw");
	config.cputemp_file = strdup("/sys/devices/virtual/thermal/thermal_zone0/temp");
    config.led_effect=1;
    config.led_duration=1;
	config.ya_tts_api_key = strdup("");
	config.ya_tts_folder_id = strdup("");
    config.cache_tts_path = strdup("");
    config.cache_all = 0;
    config.cache_make_index = 0;
	config.readinterval = 1;
	config.treshold = 10;
	config.cputemp_treshold = 1200;
	config.verbosity = 2;
	config.auto_discovery = 1;
	config.disable_bt=0;
	config.disable_ble=0;
	config.disable_illuminance=0;
	config.disable_cputemp=0;
	config.disable_btn=0;
	config.btscan_interval = 60;
	config.btscan_duration = 50;

	char *string = malloc(fsize + 1);
	fread(string, 1, fsize, f);
	fclose(f);
	string[fsize] = 0;
	enum json_type type;
	enum json_tokener_error jerr;
	json_object *jobj = json_tokener_parse_verbose((char *)string, &jerr);
	if (jerr != json_tokener_success)
	{
		printf("%s\n", json_tokener_error_desc(jerr));
		return -1;
	}
	else if (json_object_is_type(jobj, json_type_object) == 0)
	{
		printf("bad config file %s: config object not found\n", file);
		return -1;
	}
	else
		printf("%s\n", json_tokener_error_desc(jerr));

	json_object_object_foreach(jobj, key, val)
	{
		type = json_object_get_type(val);
		if (strcmp(key, "mqtt_host") == 0 && type == json_type_string)
		{
			config.mqtt_host = strdup(json_object_get_string(val));
		}
		if (strcmp(key, "mqtt_user") == 0 && type == json_type_string)
		{
			config.mqtt_user = strdup(json_object_get_string(val));
		}
		if (strcmp(key, "mqtt_user_pw") == 0 && type == json_type_string)
		{
			config.mqtt_user_pw = strdup(json_object_get_string(val));
		}
		if (strcmp(key, "topic") == 0 && type == json_type_string)
		{
			config.topic = strdup(json_object_get_string(val));
		}
		if (strcmp(key, "device_id") == 0 && type == json_type_string)
		{
			config.device_id = strdup(json_object_get_string(val));
		}
		if (strcmp(key, "red_led") == 0 && type == json_type_string)
		{
			config.red_led = strdup(json_object_get_string(val));
		}
		if (strcmp(key, "green_led") == 0 && type == json_type_string)
		{
			config.green_led = strdup(json_object_get_string(val));
		}
    	if (strcmp(key, "blue_led") == 0 && type == json_type_string)
		{
			config.blue_led = strdup(json_object_get_string(val));
		}
    	if (strcmp(key, "led_effect") == 0 && type == json_type_string)
		{
			if(strcmp(json_object_get_string(val), "fade") == 0)
				config.led_effect = 1;
			if(strcmp(json_object_get_string(val), "pattern") == 0)
				config.led_effect = 2;
			if(strcmp(json_object_get_string(val), "wheel") == 0)
				config.led_effect = 3;
		}
    	if (strcmp(key, "led_duration") == 0 && type == json_type_int)
		{
			config.led_duration = (uint8_t)json_object_get_int(val);
		}

		if (strcmp(key, "lux_file") == 0 && type == json_type_string)
		{
			config.lux_file = strdup(json_object_get_string(val));
		}
		if (strcmp(key, "cputemp_file") == 0 && type == json_type_string)
		{
			config.cputemp_file = strdup(json_object_get_string(val));
		}
		if (strcmp(key, "ya_tts_api_key") == 0 && type == json_type_string)
		{
			config.ya_tts_api_key = strdup(json_object_get_string(val));
		}
		if (strcmp(key, "cache_tts_path") == 0 && type == json_type_string)
		{
			config.cache_tts_path = strdup(json_object_get_string(val));
		}
		if (strcmp(key, "cache_tts_all") == 0 && type == json_type_boolean)
		{
			config.cache_all = json_object_get_boolean(val) ? 1 : 0;
		}
		if (strcmp(key, "cache_tts_make_index") == 0 && type == json_type_boolean)
		{
			config.cache_make_index = json_object_get_boolean(val) ? 1 : 0;
		}
		if (strcmp(key, "ya_tts_folder_id") == 0 && type == json_type_string)
		{
			config.ya_tts_folder_id = strdup(json_object_get_string(val));
		}
		if (strcmp(key, "mqtt_port") == 0 && type == json_type_int)
		{
			config.mqtt_port = (uint16_t)json_object_get_int(val);
		}
		if (strcmp(key, "mqtt_keepalive") == 0 && type == json_type_int)
		{
			config.mqtt_keepalive = (uint8_t)json_object_get_int(val);
		}
		if (strcmp(key, "readinterval") == 0 && type == json_type_int)
		{
			config.readinterval = (uint8_t)json_object_get_int(val);
		}
		if (strcmp(key, "treshold") == 0 && type == json_type_int)
		{
			config.treshold = (uint8_t)json_object_get_int(val);
		}
		if (strcmp(key, "cputemp_treshold") == 0 && type == json_type_int)
		{
			config.cputemp_treshold = (uint16_t)json_object_get_int(val);
		}
		if (strcmp(key, "mqtt_retain") == 0 && type == json_type_boolean)
		{
			config.mqtt_retain = (uint8_t)json_object_get_boolean(val) ? 1 : 0;
		}
		if (strcmp(key, "auto_discovery") == 0 && type == json_type_boolean)
		{
			config.auto_discovery = (uint8_t)json_object_get_boolean(val) ? 1 : 0;
		}
		if (strcmp(key, "log_level") == 0 && type == json_type_int)
		{
			config.verbosity = (uint8_t)json_object_get_int(val);
		}
		if (strcmp(key, "btscan_interval") == 0 && type == json_type_int)
		{
			config.btscan_interval = (uint8_t)json_object_get_int(val);
		}
		if (strcmp(key, "btscan_duration") == 0 && type == json_type_int)
		{
			config.btscan_duration = (uint8_t)json_object_get_int(val);
		}
		
		if (strcmp(key, "disable") == 0 && type == json_type_array)
		{
			array_list * components = json_object_get_array(val);
			size_t len = array_list_length(components);
			for (size_t j = 0; j < len; ++j) {
				json_object * elem = json_object_array_get_idx(val, j);
				if(strcmp(json_object_get_string(elem), "bt") == 0) {
					config.disable_bt=1;
				}
				if(strcmp(json_object_get_string(elem), "ble") == 0) {
					config.disable_ble=1;
				}
				if(strcmp(json_object_get_string(elem), "illuminance") == 0) {
					config.disable_illuminance=1;
				}
				if(strcmp(json_object_get_string(elem), "cputemp") == 0) {
					config.disable_cputemp=1;
				}
				if(strcmp(json_object_get_string(elem), "btn") == 0) {
					config.disable_btn=1;
				}
			}
		}
#ifdef USE_BLE
		if (strcmp(key, "ble_list") == 0 && type == json_type_array)
		{
			array_list * ble_arr = json_object_get_array(val);
			int ble_len = array_list_length(ble_arr);
			init_maclist(ble_len);
			printf("ble list length:  %d\n", ble_len);
			for (int j = 0; j < ble_len; ++j) {
				json_object * elem = json_object_array_get_idx(val, j);
				char* mac = strdup(json_object_get_string(elem));
				add_mac(mac, j);
			}
		}
		if (strcmp(key, "ble_timeout") == 0 && type == json_type_int)
		{
			set_ble_timeout(json_object_get_int(val));
		}
		
#endif
	}
    if(strstr(config.topic, "{device_id}") != NULL) {
        char* tmp=malloc(strlen(config.topic)+strlen(config.device_id) + 1);

        long int len = strstr(config.topic, "{device_id}")-config.topic;
        strncpy(string, config.topic, (size_t)len);
        string[len]=0;
        sprintf(tmp, "%s%s%s", string, config.device_id, config.topic + len + 11);
        config.topic=strdup(tmp);
        free(tmp);
    }
	json_object_put(jobj);
	free(string);
	return 0;
}

void cfg_dump(void)
{
	printf("MQTT Address: %s:%d\n", config.mqtt_host, config.mqtt_port);
	printf("MQTT Keepalive: %d\n", config.mqtt_keepalive);
	printf("DeviceId: %s\n", config.device_id);
}
