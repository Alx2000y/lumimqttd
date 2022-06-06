# MQTT agent for Xiaomi Lumi gateway

## Interaction

### Default devices

|      Action      |         Topic         |                Payload                |                                                          Expected values          |
|:----------------:|:---------------------:|:-------------------------------------:|:---------------------------------------------------------------------------------:|
| Read light state | lumi/&lt;ID&gt;/light       |                                       | {"state": "ON", "brightness": 255, "color": {"r": 255, "g": 0, "b": 0}}     |
| Switch light     | lumi/&lt;ID&gt;/light/set   | {"state": "ON"}                       |                                                                             |
| Set light color  | lumi/&lt;ID&gt;/light/set   | {"color": {"r": 255, "g": 0, "b": 0}} |                                                                             |
| Set brightness   | lumi/&lt;ID&gt;/light/set   | {"brightness": 255}                   |                                                                             |
| Read illuminance | lumi/&lt;ID&gt;/illuminance |                                       | 0-1000                                                                      |
| Read cpu temperature | lumi/&lt;ID&gt;/cputemp |                                       | <float> °C |
| Button           | lumi/&lt;ID&gt;/btn | | current position 0 or 1 |
| Volume           | lumi/&lt;ID&gt;/volume | | current volume 0-100 |
| Set volume           | lumi/&lt;ID&gt;/volume/set | 0-100 | |
| Volume Alert channel           | lumi/&lt;ID&gt;/volumealert | | current volume 0-100 |
| Set volume Alert channel           | lumi/&lt;ID&gt;/volumealert/set | 0-100 | |
| Play music file  | lumi/&lt;ID&gt;/sound/set | local path to mp3 file or mp3 url | |
| TTS say text  | lumi/&lt;ID&gt;/tts/set | text | |
| TTS yandex set voice  | lumi/&lt;ID&gt;/tts/voice/set | one from list of yandex voices | Default alyss |
| TTS yandex set emotion  | lumi/&lt;ID&gt;/tts/emotion/set | one from list of emotions | Default neutral |
| TTS yandex set speed  | lumi/&lt;ID&gt;/tts/speed/set | 1-30 | Default 10 |



The configuration file is a JSON with the following content:

```json
{
    "mqtt_host": "localhost",
    "mqtt_port": 1883,
    "mqtt_user": "",
    "mqtt_password": "",
	"mqtt_retain": true,
    "topic_root": "lumi/{device_id}",
    "device_id":"001"
    "auto_discovery": true,
	"connect_retries": 10,
	"log_level": 3,
	"readinterval": 1,
	"treshold": 30,
	"ya_tts_api_key": "",
	"ya_tts_folder_id": ""
}
```
Every line is optional. By default, LumiMQTT will use the connection
to localhost with the anonymous login.

`device_id` **if not provided** will be automatically replaced by a hex number 
representing a MAC address of the first network interface.

`auto_discovery` set to `false` to disable creating autodiscovery topics that
are user by Home Assistant to discover entities.

`mqtt_retain` is option to enable storing last sensor value on the broker

`threshold` is a threshold to avoid sending data to MQTT on small changes

`readinterval` value in seconds to send changed data

Для использования yandex голосов для ттс требуется указать `ya_tts_api_key` и
`ya_tts_folder_id` из консоли Yandex облака.



## OpenLumi OpenWrt installation

```sh 
opkg update 
opkg install lumimqttd
```

To upgrade you can just run

```sh
opkg update & opkg install lumimqttd
```
