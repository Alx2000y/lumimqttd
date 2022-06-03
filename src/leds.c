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
        //memcpy(&leds, &curstateleds, sizeof leds_rgb);
        struct leds_rgb leds = get_leds();
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
            //json_object *jobj = json_tokener_parse(payload);
        _syslog(LOG_INFO, "Light set to %s \n", payload);

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
                        leds.state = json_object_get_int(val) > 0 ? 1 : 0;
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
                    leds.brightness = json_object_get_int(val);
                    _syslog(LOG_INFO, "brightness: %d\n", leds.brightness);
                }
                if (strcmp(key, "speed") == 0)
                {
                    leds.speed = json_object_get_int(val);
                    _syslog(LOG_INFO, "speed: %d\n", leds.speed);
                }
                if (strcmp(key, "effect") == 0)
                {
                    leds.effect = 0;
                    if (strncmp(json_object_get_string(val), "transition", 10) == 0)
                    {
                        leds.effect = 1;
                    }
                    if (strncmp(json_object_get_string(val), "pattern", 7) == 0)
                    {
                        leds.effect = 2;
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
                                leds.r = json_object_get_int(jtmp);
                            }
                            exists = json_object_object_get_ex(jcolor, "g", &jtmp);
                            if (exists)
                            {
                                leds.g = json_object_get_int(jtmp);
                            }
                            exists = json_object_object_get_ex(jcolor, "b", &jtmp);
                            if (exists)
                            {
                                leds.b = json_object_get_int(jtmp);
                            }
                        }
                        _syslog(LOG_INFO, "color: %d\n", type);
                    }
                }
                if (strcmp(key, "r") == 0)
                {
                    leds.r = json_object_get_int(val);
                    _syslog(LOG_INFO, "color red: %d\n", leds.r);
                }
                if (strcmp(key, "g") == 0)
                {
                    leds.g = json_object_get_int(val);
                    _syslog(LOG_INFO, "color green: %d\n", leds.g);
                }
                if (strcmp(key, "b") == 0)
                {
                    leds.b = json_object_get_int(val);
                    _syslog(LOG_INFO, "color blue: %d\n", leds.b);
                }
                if (strcmp(key, "red") == 0)
                {
                    leds.r = json_object_get_int(val);
                    _syslog(LOG_INFO, "color red: %d\n", leds.r);
                }
                if (strcmp(key, "green") == 0)
                {
                    leds.g = json_object_get_int(val);
                    _syslog(LOG_INFO, "color green: %d\n", leds.g);
                }
                if (strcmp(key, "blue") == 0)
                {
                    leds.b = json_object_get_int(val);
                    _syslog(LOG_INFO, "color blue: %d\n", leds.b);
                }
            }
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

    curstateleds.speed = config.led_speed;
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
/*
struct led_effect_data {
	unsigned int start;
	unsigned int period;
    unsigned int stepr;
    unsigned int stepb;
    unsigned int stepg;
    struct leds_rgb leds;
	struct timer_list timer;
};


static void led_effect_function(struct timer_list *t)
{
	struct led_effect_data *effect_data = from_timer(effect_data, t, timer);
	unsigned long delay = 0;
    
    if(effect_data->start==0) {
        effect_data->start=jiffies;
        effect_data->stepr=255/period*80;
    }

   	effect_data->period = msecs_to_jiffies(effect_data->period);
	delay = msecs_to_jiffies(70);
  	delay = effect_data->period / 4 - msecs_to_jiffies(70);

    fprintf(ledrfile, "%d", (brightness));
    fflush(ledrfile);

    fprintf(ledgfile, "%d", (brightness));
    fflush(ledgfile);

    fprintf(ledbfile, "%d", (brightness));
    fflush(ledbfile);

	mod_timer(&effect_data->timer, jiffies + delay);
}
*/
void set_leds(struct leds_rgb leds)
{
    if (leds.brightness == 0)
    {
        leds.brightness = 255;
        leds.state = 0;
    }
/*    if(leds.effect!=0 && leds.speed!=0) {

        struct led_effect_data *effect_data;

    	effect_data = kzalloc(sizeof(*effect_data), GFP_KERNEL);

    	timer_setup(&effect_data->timer, led_effect_function, 0);

    	effect_data->period = leds.speed;

    	led_effect_function(&effect_data->timer);

    }else{
*/    if (ledrfile == NULL)
        init_fleds();

    fprintf(ledrfile, "%d", (leds.r * leds.brightness * leds.state / 255));
    fflush(ledrfile);

    fprintf(ledgfile, "%d", (leds.g * leds.brightness * leds.state / 255));
    fflush(ledgfile);

    fprintf(ledbfile, "%d", (leds.b * leds.brightness * leds.state / 255));
    fflush(ledbfile);
    //}
    if (leds.state == 0 && leds.r==0 && leds.g==0 && leds.b==0)
    {
        leds.r = curstateleds.r;
        leds.g = curstateleds.g;
        leds.b = curstateleds.b;
    }
    //if(leds.r != curstateleds.r || leds.g != curstateleds.g || leds.b != curstateleds.b || leds.brightness != curstateleds.brightness || leds.state != curstateleds.state )
        leds.status = 1;
    memcpy(&curstateleds, &leds, sizeof(leds));
    //curstateleds = leds;
}
struct leds_rgb get_leds()
{
    struct leds_rgb leds;
    uint8_t tmp;
    leds.state = curstateleds.state;
    leds.brightness = curstateleds.brightness;
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
    leds.r = tmp * 255 / leds.brightness;

    rewind(ledgfile);
    fscanf(ledgfile, "%hhu", &tmp);
    leds.g = tmp * 255 / leds.brightness;

    rewind(ledbfile);
    fscanf(ledbfile, "%hhu", &tmp);
    leds.b = tmp * 255 / leds.brightness;
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
        topic = malloc(strlen(config.topic) + 5);
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
                str = json_object_new_string("transition");
            }
            else
            {
                str = json_object_new_string("none");
            }
            json_object_object_add(root, "effect", str);
            json_object_object_add(root, "speed", json_object_new_int(newleds.speed));
            sprintf(message, "%s", json_object_to_json_string_ext(root, NULL));
            json_object_put(root);
        }
        if (mqtt_publish(topic, message))
        {
            _syslog(LOG_INFO, "Last r %d g %d b %d s %d b %d c %d", curstateleds.r, curstateleds.g, curstateleds.b, curstateleds.state, curstateleds.brightness, curstateleds.status);
            _syslog(LOG_INFO, "Published %s r %d g %d b %d s %d b %d c %d", topic, newleds.r, newleds.g, newleds.b, newleds.state, newleds.brightness, newleds.status);
            if (newleds.state != 0){
                memcpy(&curstateleds, &newleds, sizeof(newleds));
                //curstateleds = newleds;
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

void leds_auto_discover(void)
{
    char *topic, *message;
    json_object *effect_list, *identifiers, *device, *root;

    topic = malloc(strlen(config.device_id) + 100);
    message = malloc(1000);

    device = json_object_new_object();
    sprintf(message, "xiaomi_gateway_%s", config.device_id);
    identifiers = json_object_new_array();
    json_object_array_add(identifiers, json_object_new_string(message));
    json_object_object_add(device, "identifiers", identifiers);
    json_object_object_add(device, "name", json_object_new_string(message));
    json_object_object_add(device, "sw_version", json_object_new_string("0.0.1"));
    json_object_object_add(device, "model", json_object_new_string("Xiaomi Gateway"));
    json_object_object_add(device, "manufacturer", json_object_new_string("Xiaomi"));

    root = json_object_new_object();
    if (root)
    {
        sprintf(message, "%s_light", config.device_id);
        json_object_object_add(root, "name", json_object_new_string(message));
        json_object_object_add(root, "unique_id", json_object_new_string(message));
        //json_object_object_add(root, "schema", json_object_new_string("json"));
        json_object_object_add(root, "device", json_object_get(device));
        sprintf(message, "%s%s", config.topic, "status");
        json_object_object_add(root, "availability_topic", json_object_new_string(message));
        sprintf(message, "%s%s", config.topic, "light");
        json_object_object_add(root, "state_topic", json_object_new_string(message));
        sprintf(message, "%s%s", config.topic, "light/set");
        json_object_object_add(root, "command_topic", json_object_new_string(message));
        json_object_object_add(root, "brightness", json_object_new_boolean(1));
        json_object_object_add(root, "rgb", json_object_new_boolean(1));
        json_object_object_add(root, "effect", json_object_new_boolean(1));
        effect_list = json_object_new_array();
        json_object_array_add(effect_list, json_object_new_string("off"));
        json_object_array_add(effect_list, json_object_new_string("transition"));
        json_object_array_add(effect_list, json_object_new_string("pattern"));
        json_object_object_add(root, "effect_list", effect_list);
        sprintf(message, "%s", json_object_to_json_string_ext(root, 0));
        //json_object_object_del(root, "device");
        //device=json_object_object_get(root, "device");
        //json_object_put(root);
        sprintf(topic, "homeassistant/light/%s/light/config", config.device_id);
        if (!mqtt_publish(topic, message))
            _syslog(LOG_ERR, "Error: mosquitto_publish failed\n");
    }
    json_object_put(device);
    json_object_put(effect_list);
    json_object_put(identifiers);
    free(topic);
    free(message);
}
