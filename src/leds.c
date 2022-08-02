#include "leds.h"

int leds_on_message(char *id, char *payload, int len)
{
    char *temp = NULL;
    unsigned long val = 0;
	struct json_tokener *tok;
	struct json_object *jobj;
    if (strcmp(id, "light/set") == 0)
    {
        _syslog(LOG_INFO, "Light set to %s \nlen: %d\n", payload,len);
        struct leds_rgb leds = get_leds();
        if (strcmp(payload, "ON") == 0){
        	leds.state = 1;
        } else
        if (strcmp(payload, "OFF") == 0){
        	leds.state = 0;
        } else
        if (strlen(payload) == 7)
        {
            temp = strdup(payload);
            if (temp[0] == '#')
            {
                val = strtoul(temp + 1, NULL, 16);
                leds.r = ((val >> 16) & 0xff);
                leds.g = ((val >> 8) & 0xff);
                leds.b = (val & 0xff);
                leds.state = 1;
                leds.brightness = 255;
            }
            free(temp);
        }
        else if (strchr(payload, '{'))
        {
            enum json_type type;
            tok = json_tokener_new();	
            if (!tok)		return 0;
            jobj = json_tokener_parse_ex(tok, (char *)payload, len);	
            if (tok->err != json_tokener_success)	{
	            _syslog(LOG_ERR, "json fail %s \n", payload);
	            return 0;
	        }	
            json_tokener_free(tok);
            json_object_object_foreach(jobj, key, val)
            {
                type = json_object_get_type(val);
                if (strcmp(key, "state") == 0)
                {
                    switch (type)
                    {
                    case json_type_boolean:
                        leds.state = json_object_get_boolean(val) ? 1 : 0;
                        break;
                    case json_type_double:
                        leds.state = json_object_get_double(val) > 0 ? 1 : 0;
                        break;
                    case json_type_int:
                        leds.state = (uint8_t)json_object_get_int(val) > 0 ? 1 : 0;
                        break;
                    case json_type_string:
                        leds.state = 0;
                        if (strncmp(json_object_get_string(val), "ON", 2) == 0)
                        {
                            leds.state = 1;
                        }
                        else if (strncmp(json_object_get_string(val), "TOGGLE", 6) == 0)
                        {
                            leds.state = !leds.state;
                        }
                        break;
                    default:
                        break;
                    }
                }
                if (strcmp(key, "brightness") == 0)
                {
                    leds.brightness = (uint8_t)json_object_get_int(val);
                    _syslog(LOG_INFO, "brightness: %d\n", leds.brightness);
                }
                if (strcmp(key, "duration") == 0)
                {
                    leds.duration = (uint8_t)json_object_get_int(val);
                    _syslog(LOG_INFO, "duration: %d\n", leds.duration);
                }
                if (strcmp(key, "pattern") == 0)
                {
                    if (ledpattern != NULL) free(ledpattern);
                    ledpattern = strdup(json_object_get_string(val));
                }
                if (strcmp(key, "effect") == 0)
                {
                    leds.effect = 0;
                    if (strncmp(json_object_get_string(val), "fade", 4) == 0)
                    {
                        leds.effect = 1;
                    }
                    if (strncmp(json_object_get_string(val), "pattern", 7) == 0)
                    {
                        leds.effect = 2;
                    }
                    if (strncmp(json_object_get_string(val), "wheel", 5) == 0)
                    {
                        leds.effect = 3;
                    }
                }
                if (strcmp(key, "color") == 0)
                {
                    leds.r = 255;
                    leds.g = 255;
                    leds.b = 255;
                    if (type == json_type_object)
                    {
                        json_object *jcolor;
                        json_object *jtmp;

                        int exists = json_object_object_get_ex(jobj, "color", &jcolor);
                        if (exists)
                        {
                            exists = json_object_object_get_ex(jcolor, "r", &jtmp);
                            if (exists)
                            {
                                leds.r = (uint8_t)json_object_get_int(jtmp);
                            }
                            exists = json_object_object_get_ex(jcolor, "g", &jtmp);
                            if (exists)
                            {
                                leds.g = (uint8_t)json_object_get_int(jtmp);
                            }
                            exists = json_object_object_get_ex(jcolor, "b", &jtmp);
                            if (exists)
                            {
                                leds.b = (uint8_t)json_object_get_int(jtmp);
                            }
                        }
                        _syslog(LOG_INFO, "color: %d\n", type);
                    }
                }
                if (strcmp(key, "r") == 0)
                {
                    leds.r = (uint8_t)json_object_get_int(val);
                    _syslog(LOG_INFO, "color red: %d\n", leds.r);
                }
                if (strcmp(key, "g") == 0)
                {
                    leds.g = (uint8_t)json_object_get_int(val);
                    _syslog(LOG_INFO, "color green: %d\n", leds.g);
                }
                if (strcmp(key, "b") == 0)
                {
                    leds.b = (uint8_t)json_object_get_int(val);
                    _syslog(LOG_INFO, "color blue: %d\n", leds.b);
                }
                if (strcmp(key, "red") == 0)
                {
                    leds.r = (uint8_t)json_object_get_int(val);
                    _syslog(LOG_INFO, "color red: %d\n", leds.r);
                }
                if (strcmp(key, "green") == 0)
                {
                    leds.g = (uint8_t)json_object_get_int(val);
                    _syslog(LOG_INFO, "color green: %d\n", leds.g);
                }
                if (strcmp(key, "blue") == 0)
                {
                    leds.b = (uint8_t)json_object_get_int(val);
                    _syslog(LOG_INFO, "color blue: %d\n", leds.b);
                }
            }
            json_object_put(jobj);
        }
        _syslog(LOG_INFO, "vals: %d %d %d %d %d\n", leds.state, leds.brightness, leds.r, leds.g, leds.b);
        set_leds(leds);
    }
    else
    {
        return 0;
    }
    return 1;
}
void init_leds()
{
    curstateleds.r = 255;
    curstateleds.g = 255;
    curstateleds.b = 255;
    curstateleds.brightness = 255;
    curstateleds.state = 0;

    curstateleds.duration = config.led_duration;
    curstateleds.effect = config.led_effect;
}
void init_fleds()
{
    ledrfile = fopen(config.red_led, "r+");
    if (ledrfile == NULL)
    {
        _syslog(LOG_ERR, "could not read %s:%s", config.red_led, strerror(errno));
        return;
    }
    ledgfile = fopen(config.green_led, "r+");
    if (ledgfile == NULL)
    {
        _syslog(LOG_ERR, "could not read %s:%s", config.green_led, strerror(errno));
        return;
    }
    ledbfile = fopen(config.blue_led, "r+");
    if (ledbfile == NULL)
    {
        _syslog(LOG_ERR, "could not read %s:%s", config.blue_led, strerror(errno));
        return;
    }
}
void led_sigc() 
{
	if (ledrfile != NULL)
	    fclose(ledrfile);
	if (ledgfile != NULL)
	    fclose(ledgfile);
	if (ledbfile != NULL)
	    fclose(ledbfile);
}

void set_leds(struct leds_rgb leds)
{
    if (leds.brightness == 0)
    {
        leds.brightness = 255;
        leds.state = 0;
    }
    if (ledrfile == NULL){
        init_fleds();
    }
	if(leds.effect==1 && leds.duration!=0) {
		int ticks=255*leds.duration;
		int i=ticks;
		int cr, fr;
		int cg, fg;
		int cb, fb;
		cr=(curstateleds.r * curstateleds.brightness * curstateleds.state);
		fr=(leds.r * leds.brightness * leds.state);
		cg=(curstateleds.g * curstateleds.brightness * curstateleds.state);
		fg=(leds.g * leds.brightness * leds.state);
		cb=(curstateleds.b * curstateleds.brightness * curstateleds.state);
		fb=(leds.b * leds.brightness * leds.state);
		while(i>0) {
			usleep(4000);
			applyRGB(
				(uint8_t)((fr * (ticks - i) + (cr * i)) / ticks / 255), 
                (uint8_t)((fg * (ticks - i) + (cg * i)) / ticks / 255),
                (uint8_t)((fb * (ticks - i) + (cb * i)) / ticks / 255)
			);
			i--;
		}
	}
	if(leds.effect==2 && leds.duration!=0) {
		int ticks=255*leds.duration;
		int i=ticks;
		int cr, fr;
		int cg, fg;
		int cb, fb;
		cr=(curstateleds.r * curstateleds.brightness * curstateleds.state);
		fr=(leds.r * leds.brightness * leds.state);
		cg=(curstateleds.g * curstateleds.brightness * curstateleds.state);
		fg=(leds.g * leds.brightness * leds.state);
		cb=(curstateleds.b * curstateleds.brightness * curstateleds.state);
		fb=(leds.b * leds.brightness * leds.state);
		while(i>0) {
			usleep(4000);
			applyRGB(
				(uint8_t)(((fr/2 + rand()%120) * (ticks - i) + (cr * i)) / ticks / 255), 
                (uint8_t)((fg * (ticks - i) + (cg * i)) / ticks / 255),
                (uint8_t)((fb * (ticks - i) + (cb * i)) / ticks / 255)
			);
			i--;
		}
	}


	if(leds.effect==3 && leds.duration!=0) {
		int i, k, r,g,b;
		for(k = 255*leds.duration; k >= 0 ; k--){
			i=k%255;
            r = 0;
            g = 0;
            b = 0;
    		if(i < 85)
		    {
                r = 255 - i * 3;
                b = i * 3;
		    }
		    else if(i < 170)
		    {
        		i -= 85;
                g = i * 3;
                b = 255 - i * 3;
		    }
		    else
		    {
        		i -= 170;
                r = i * 3;
                g = 255 - i * 3;
		    }
            applyRGB((uint8_t)r, (uint8_t)g, (uint8_t)b);
		    usleep(4000);
		}
	}
   	applyRGB((uint8_t)(leds.r * leds.brightness * leds.state / 255), (uint8_t)(leds.g * leds.brightness * leds.state / 255), (uint8_t)(leds.b * leds.brightness * leds.state / 255));
    leds.status = 1;

    if (leds.state == 0 && leds.r==0 && leds.g==0 && leds.b==0) {
        leds.r = curstateleds.r;
        leds.g = curstateleds.g;
        leds.b = curstateleds.b;
    }
    memcpy(&curstateleds, &leds, sizeof(leds));
}
void applyRGB(uint8_t r, uint8_t g, uint8_t b) {
		fprintf(ledrfile, "%d", r);
	    fflush(ledrfile);

    	fprintf(ledgfile, "%d", g);
	    fflush(ledgfile);

    	fprintf(ledbfile, "%d", b);
	    fflush(ledbfile);
}
struct leds_rgb get_leds()
{
    struct leds_rgb leds;
    uint8_t tmp;
    leds.state = curstateleds.state;
    leds.brightness = curstateleds.brightness;
    leds.duration = curstateleds.duration;
    leds.effect = curstateleds.effect;
    
    if (leds.status > 1) return leds;

    if (leds.brightness == 0)
    {
        leds.brightness = 255;
        leds.state = 0;
    }
    if(leds.state==0) {
        leds.r=curstateleds.r;
        leds.g=curstateleds.g;
        leds.b=curstateleds.b;
        leds.status = 0;
        return leds;
    }

    if (ledrfile == NULL)
        init_fleds();

    rewind(ledrfile);
    fscanf(ledrfile, "%hhu", &tmp);
    leds.r = (uint8_t)(tmp * 255 / leds.brightness);

    rewind(ledgfile);
    fscanf(ledgfile, "%hhu", &tmp);
    leds.g = (uint8_t)(tmp * 255 / leds.brightness);

    rewind(ledbfile);
    fscanf(ledbfile, "%hhu", &tmp);
    leds.b = (uint8_t)(tmp * 255 / leds.brightness);

    if (leds.b != 0 || leds.g != 0 || leds.r != 0)
        leds.state = 1;
    return leds;
}
void leds_periodical_check(void)
{
    char *topic;
    char *message;
    struct leds_rgb newleds = get_leds();

    if (curstateleds.status != newleds.status ||
        curstateleds.r * curstateleds.state != newleds.r * newleds.state ||
        curstateleds.g * curstateleds.state != newleds.g * newleds.state ||
        curstateleds.b * curstateleds.state != newleds.b * newleds.state ||
        curstateleds.state != newleds.state ||
        curstateleds.brightness != newleds.brightness)
    {
        topic = malloc(strlen(config.topic) + 6);
        sprintf(topic, "%s%s", config.topic, "light");

        message = malloc(100);

        _syslog(LOG_INFO, "Piblished %s %d %d %d", topic, newleds.r, newleds.g, newleds.b);

        json_object *color, *str;
        json_object *root = json_object_new_object();
        if (root)
        {
            json_object_object_add(root, "brightness", json_object_new_int(newleds.brightness));
            if (newleds.state == 1)
            {
                str = json_object_new_string("ON");
            }
            else
            {
                str = json_object_new_string("OFF");
            }
            json_object_object_add(root, "state", str);
            color = json_object_new_object();
            json_object_object_add(color, "r", json_object_new_int(newleds.r));
            json_object_object_add(color, "g", json_object_new_int(newleds.g));
            json_object_object_add(color, "b", json_object_new_int(newleds.b));
            json_object_object_add(root, "color", color);
            if (newleds.effect == 2)
            {
                str = json_object_new_string("pattern");
            }
            else
            if (newleds.effect == 1)
            {
                str = json_object_new_string("fade");
            }
            else
            {
                str = json_object_new_string("none");
            }
            json_object_object_add(root, "effect", str);
            json_object_object_add(root, "duration", json_object_new_int(newleds.duration));
            sprintf(message, "%s", json_object_to_json_string_ext(root, NULL));
            json_object_put(root);
        }
        if (mqtt_publish(topic, message))
        {
            _syslog(LOG_INFO, "Last r %d g %d b %d s %d b %d c %d", curstateleds.r, curstateleds.g, curstateleds.b, curstateleds.state, curstateleds.brightness, curstateleds.status);
            _syslog(LOG_INFO, "Published %s r %d g %d b %d s %d b %d c %d", topic, newleds.r, newleds.g, newleds.b, newleds.state, newleds.brightness, newleds.status);
            if (newleds.state != 0){
                memcpy(&curstateleds, &newleds, sizeof(newleds));
            }else
            {
                curstateleds.brightness = newleds.brightness;
                curstateleds.state = newleds.state;
            }
            curstateleds.status = 0;
        }
        free(topic);
        free(message);
    }
}
void leds_auto_discover(void) {
        char *topic, *message;
        message = malloc(1000);
        topic = malloc(1000);
        sprintf(topic, "\"device\" : {\"identifiers\": [\"xiaomi_gateway_%s\"] ,\"name\" : \"xiaomi_gateway_%s\", \"sw_version\" : \"%s\", \"model\" : \"Xiaomi Gateway\", \"manufacturer\" : \"Xiaomi\"},\"availability_topic\": \"%sstatus\",", config.device_id, config.device_id, VERSION, config.topic);
        sprintf(message, "{\"name\": \"%s_light\", \"unique_id\" : \"%s_light\", \"schema\" : \"json\", %s \"state_topic\" : \"%slight\",\"command_topic\" : \"%slight/set\",\"effect_list\" : [\"none\",\"fade\",\"pattern\",\"wheel\"],\"rgb\" : true,\"brightness\" : true,\"effect\" : true}", config.device_id, config.device_id, topic, config.topic, config.topic);
        sprintf(topic, "homeassistant/light/%s/light/config", config.device_id);
        if (!mqtt_publish(topic, message))
            _syslog(LOG_ERR, "Error: mosquitto_publish failed\n");
        free(topic);
        free(message);
}