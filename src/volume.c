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
        char *channel = malloc(strlen(id) - 11);
        strncpy(channel, id + 7, strlen(id) - 11);
        _syslog(LOG_INFO, "snapVol %s \n", channel, config.topic);
        set_volume(atoi(payload), channel);
    }
    else
        return 0;
    return 1;

}
void set_volume(int vol, const char *selem_name)
{

    long min_volume, max_volume;
    snd_mixer_t *mixer_handle;
    snd_mixer_selem_id_t *selem_id;
    snd_mixer_elem_t *elem;
    snd_mixer_open(&mixer_handle, 0);
    snd_mixer_attach(mixer_handle, "default");
    snd_mixer_selem_register(mixer_handle, NULL, NULL);
    snd_mixer_load(mixer_handle);
    snd_mixer_selem_id_alloca(&selem_id);
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
            return;
        }
        snd_mixer_selem_set_playback_volume_all(elem, vol);
    }
    else
    {
        _syslog(LOG_ERR, "sound channel not found '%s'\n", selem_name);
    }

    snd_mixer_close(mixer_handle);
}
int get_volume(const char *selem_name)
{
    long current_vol = 0;

    long min_volume, max_volume;

    snd_mixer_t *mixer_handle;
    snd_mixer_selem_id_t *selem_id;
    snd_mixer_elem_t *elem;
    snd_mixer_open(&mixer_handle, 0);
    snd_mixer_attach(mixer_handle, "default");
    snd_mixer_selem_register(mixer_handle, NULL, NULL);
    snd_mixer_load(mixer_handle);
    snd_mixer_selem_id_alloca(&selem_id);
    snd_mixer_selem_id_set_index(selem_id, 0);
    snd_mixer_selem_id_set_name(selem_id, selem_name);
    elem = snd_mixer_find_selem(mixer_handle, selem_id);
    if (elem != NULL)
    {
        snd_mixer_selem_get_playback_volume_range(elem, &min_volume, &max_volume);

        snd_mixer_selem_get_playback_volume(elem, SND_MIXER_SCHN_FRONT_LEFT, &current_vol);
        snd_mixer_close(mixer_handle);

        return (int)(current_vol * 100 / max_volume);
    }
    else
    {
        _syslog(LOG_ERR, "sound channel not found '%s'\n", selem_name);
    }
    snd_mixer_close(mixer_handle);
    return (int)current_vol;
}
void volume_periodical_check(void)
{

    char *topic=NULL;
    char *message=NULL;

    int volume = get_volume("Master");

    int volumealert = get_volume("AlertVol");
    if (curalert != volumealert)
    {
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

