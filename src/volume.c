#include "volume.h"

int volume_on_message(char *id, char *payload, int len)
{
    if (strcmp(id, "volume/set") == 0)
    {
        _syslog(LOG_INFO, "volume %s \n", config.topic);
        set_volume(atoi(payload), "Master");
#ifdef USE_MPD
        volumempd = atoi(payload);
#endif
    }
    else if (strcmp(id, "volumealert/set") == 0 || strcmp(id, "volume/alert/set") == 0)
    {
        _syslog(LOG_INFO, "alert %s \n", config.topic);
        set_volume(atoi(payload), "AlertVol");
    }
    else if (strcmp(id, "volume/snap/set") == 0)
    {
        _syslog(LOG_INFO, "snapVol %s \n", config.topic);
        set_volume(atoi(payload), "SnapVol");
    }
    else if (strncmp(id, "volume/", 7) == 0 && strncmp(id + strlen(id) - 4, "/set", 4) == 0)
    {
        char *channel = malloc(strlen(id) - 10);
        strncpy(channel, id + 7, strlen(id) - 10);
        _syslog(LOG_INFO, "snapVol %s \n", channel, config.topic);
        set_volume(atoi(payload), channel);
    }
    else
        return 0;
    return 1;

}
void set_volume(int vol, const char *selem_name)
{
    static uint8_t logerr = 0;
    long min_volume, max_volume;
    snd_mixer_t *mixer_handle;
    snd_mixer_selem_id_t *selem_id;
    snd_mixer_elem_t *elem;
    snd_mixer_open(&mixer_handle, 0);
    snd_mixer_attach(mixer_handle, "default");
    snd_mixer_selem_register(mixer_handle, NULL, NULL);
    snd_mixer_load(mixer_handle);
    snd_mixer_selem_id_malloc(&selem_id);
    snd_mixer_selem_id_set_index(selem_id, 0);
    snd_mixer_selem_id_set_name(selem_id, selem_name);

    elem = snd_mixer_find_selem(mixer_handle, selem_id);
    if (elem != NULL)
    {
        snd_mixer_selem_get_playback_volume_range(elem, &min_volume, &max_volume);

        vol = ((float)vol / 100 * max_volume);
        if (vol < min_volume || vol > max_volume)
        {
            _syslog(LOG_ERR, "invalid value\n");
            goto end;
        }
        snd_mixer_selem_set_playback_volume_all(elem, vol);
    }
    else if (1 != logerr) {
        _syslog(LOG_ERR, "sound channel not found '%s'\n", selem_name);
        logerr = 1;
    }
end:
    snd_mixer_selem_id_free(selem_id);
    snd_mixer_close(mixer_handle);
}
int get_volume(const char *selem_name)
{
    long current_vol = 0;
    int err;
    static uint8_t logerr = 0;

    long min_volume, max_volume;

    snd_mixer_t *mixer_handle;
    snd_mixer_selem_id_t *selem_id;
    snd_mixer_elem_t *elem;
    snd_mixer_open(&mixer_handle, 0);
    snd_mixer_attach(mixer_handle, "default");
    snd_mixer_selem_register(mixer_handle, NULL, NULL);
    snd_mixer_load(mixer_handle);
    snd_mixer_selem_id_malloc(&selem_id);
    snd_mixer_selem_id_set_index(selem_id, 0);
    snd_mixer_selem_id_set_name(selem_id, selem_name);
    elem = snd_mixer_find_selem(mixer_handle, selem_id);
    if (elem != NULL)
    {
        snd_mixer_selem_get_playback_volume_range(elem, &min_volume, &max_volume);
        snd_mixer_selem_get_playback_volume(elem, SND_MIXER_SCHN_FRONT_LEFT, &current_vol);
        current_vol = (int)(current_vol * 100 / max_volume);
    }
    else if (1!=logerr) {
        _syslog(LOG_ERR, "sound channel not found '%s'\n", selem_name);
        logerr = 1;
    }
    snd_mixer_selem_id_free(selem_id);
    snd_mixer_close(mixer_handle);
    return (int)current_vol;
}
void volume_periodical_check(void)
{

    char *topic=NULL;
    char *message=NULL;
    //_syslog(LOG_INFO, "Periodical volume start\n");
    int volume = get_volume("Master");
    //_syslog(LOG_INFO, "Periodical volume get Master\n");
    int volumealert = get_volume("AlertVol");
    //_syslog(LOG_INFO, "Periodical volume get Alert\n");
    if (curalert != volumealert)
    {
	    //_syslog(LOG_INFO, "Periodical volume send Alert\n");
        curalert = volumealert;
        topic = malloc(strlen(config.topic) + 12);
        message = malloc(12);
        sprintf(topic, "%s%s", config.topic, "volumealert");
        sprintf(message, "%d", volumealert);
        mqtt_publish(topic, message);
        free(topic);
        free(message);
    }
    if (curvolume != volume)
    {
	    //_syslog(LOG_INFO, "Periodical volume send Master\n");
        curvolume = volume;
        topic = malloc(strlen(config.topic) + 7);
        message = malloc(12);
        sprintf(topic, "%s%s", config.topic, "volume");
        sprintf(message, "%d", volume);
        mqtt_publish(topic, message);
        free(topic);
        free(message);
    }
#ifdef USE_MPD
    if (curvolumempd != volumempd)
    {
        curvolumempd = volumempd;
        topic = malloc(strlen(config.topic) + 10);
        message = malloc(12);
        sprintf(topic, "%s%s", config.topic, "volumempd");
        sprintf(message, "%d", volumempd);
        mqtt_publish(topic, message);
        free(topic);
        free(message);
    }
#endif
}

void volume_auto_discover(void) {
        char *topic, *message;
        message = malloc(1000);
        topic = malloc(1000);
        sprintf(topic, "\"device\" : {\"identifiers\": [\"xiaomi_gateway_%s\"] ,\"name\" : \"xiaomi_gateway_%s\", \"sw_version\" : \"%s\", \"model\" : \"Xiaomi Gateway\", \"manufacturer\" : \"Xiaomi\"},\"availability_topic\": \"%sstatus\",", config.device_id, config.device_id, VERSION, config.topic);
        sprintf(message, "{\"name\": \"Volume Master %s\", \"unique_id\" : \"%s_volume\", %s \"state_topic\" : \"%svolume\",\"command_topic\" : \"%svolume/set\", \"step\": 1, \"min\": 0, \"max\": 100, \"entity_category\": \"config\", \"icon\": \"mdi:volume-equal\", \"unit_of_measurement\": \"%%\"}", config.device_id, config.device_id, topic, config.topic, config.topic);

        sprintf(topic, "homeassistant/number/%s/volume/config", config.device_id);
        if (!mqtt_publish(topic, message))
            _syslog(LOG_ERR, "Error: mosquitto_publish failed\n");

        sprintf(topic, "\"device\" : {\"identifiers\": [\"xiaomi_gateway_%s\"] ,\"name\" : \"xiaomi_gateway_%s\", \"sw_version\" : \"%s\", \"model\" : \"Xiaomi Gateway\", \"manufacturer\" : \"Xiaomi\"},\"availability_topic\": \"%sstatus\",", config.device_id, config.device_id, VERSION, config.topic);
        sprintf(message, "{\"name\": \"Volume Alert %s\", \"unique_id\" : \"%s_volumealert\", %s \"state_topic\" : \"%svolumealert\",\"command_topic\" : \"%svolumealert/set\", \"step\": 1, \"min\": 0, \"max\": 100, \"entity_category\": \"config\", \"icon\": \"mdi:volume-equal\", \"unit_of_measurement\": \"%%\"}", config.device_id, config.device_id, topic, config.topic, config.topic);

        sprintf(topic, "homeassistant/number/%s/volumealert/config", config.device_id);
        if (!mqtt_publish(topic, message))
            _syslog(LOG_ERR, "Error: mosquitto_publish failed\n");
        free(topic);
        free(message);
}