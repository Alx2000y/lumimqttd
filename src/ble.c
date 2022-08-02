#include "ble.h"

#define EIR_FLAGS 0x01		   /* flags */
#define EIR_UUID16_SOME 0x02   /* 16-bit UUID, more available */
#define EIR_UUID16_ALL 0x03	   /* 16-bit UUID, all listed */
#define EIR_UUID32_SOME 0x04   /* 32-bit UUID, more available */
#define EIR_UUID32_ALL 0x05	   /* 32-bit UUID, all listed */
#define EIR_UUID128_SOME 0x06  /* 128-bit UUID, more available */
#define EIR_UUID128_ALL 0x07   /* 128-bit UUID, all listed */
#define EIR_NAME_SHORT 0x08	   /* shortened local name */
#define EIR_NAME_COMPLETE 0x09 /* complete local name */
#define EIR_TX_POWER 0x0A	   /* transmit power level */
#define EIR_DEVICE_ID 0x10	   /* device ID */

void init_maclist(int num) {
	blemacs = (ble_state_t*) malloc(num * sizeof(ble_state_t *) + num * 18 + 1);
	blemacs_size = num;
}
void add_mac(char* mac, int id) {
	blemacs[id].addr=strdup(mac);
	blemacs[id].lastseen = 0;
	blemacs[id].active=1;
	blemacs[id].state=0;
}
void set_ble_timeout(int timeout) {
	ble_timeout = timeout;
}
void print_mac() {
	int i;
	for(i = 0; i< blemacs_size; i++) {
		printf("%s = %d; %d %d\n", blemacs[i].addr, blemacs[i].lastseen, blemacs[i].active, blemacs[i].state);
	}
}
void ble_periodical_check() {
	if(config.disable_ble!=0 && config.disable_bt!=0) return;
	int i;
    static int period = 1;
	for(i = 0; i< blemacs_size; i++) {
		if(blemacs[i].state == 1 && blemacs[i].lastseen + ble_timeout < (uint32_t) time(NULL)) {
			char *topic;
			char *message;
			topic = malloc(strlen(config.topic) + 22);
			message = malloc(21);
			blemacs[i].state = 0;
			sprintf(message, "{'state': %d}", blemacs[i].state);
			sprintf(topic, "%s%s/%s", config.topic, "ble", blemacs[i].addr);
			mqtt_publish_once(topic, message);
			free(topic);
			free(message);
		}
	}
    if (period % config.btscan_interval == 0)
    {
        blescan();
    }
    if (period % config.btscan_interval == config.btscan_duration)
    {
        blescan_stop();
    }
    period++;
    if (period > config.btscan_interval*100)
        period = 1;
}
void blescan_stop()
{
	signal_received = 1;
	int status;
	struct timespec ts;

	if(config.disable_ble!=0 || !ble_thread) return;

	if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
	    _syslog(LOG_ERR, "can't get time ble thread\n");
    }
    ts.tv_sec += 10;
	status = pthread_timedjoin_np(ble_thread, NULL, &ts);
	//status = pthread_join(ble_thread, NULL);
	if (status != 0)
	{
		_syslog(LOG_ERR, "can't join ble thread, status = %d\n", status);
		status = pthread_cancel(ble_thread);
		if (status != 0)
		{
			_syslog(LOG_ERR, "can't cancel ble thread, status = %d\n", status);
		}
		signal_received = 0;
	}
	ble_thread = 0;
}
void blescan()
{
	_syslog(LOG_INFO, "btble scan started");
	int status;
	if (config.disable_bt==0 && !bt_thread)
	{
		status = pthread_create(&bt_thread, NULL, btscan_thread, NULL);
		if (status != 0)
		{
			_syslog(LOG_ERR, "bt error: can't create thread, status = %d\n", status);
		}
		status = pthread_detach(bt_thread);
		if (status)
			_syslog(LOG_ERR, "Failed to detach bt Thread : %s", strerror(status));
	}
	if (config.disable_ble==0 && ble_thread)
	{
		blescan_stop();
	}
	if (config.disable_ble==0 && !ble_thread)
	{
		signal_received = 0;
		status = pthread_create(&ble_thread, NULL, blescan_thread, NULL);
		if (status != 0)
		{
			_syslog(LOG_ERR, "ble error: can't create thread, status = %d\n", status);
		}
	}
	_syslog(LOG_INFO, "btble scan ended");
}
static void eir_parse_name(uint8_t *eir, size_t eir_len,
						   char *buf, size_t buf_len)
{
	size_t offset;

	offset = 0;
	while (offset < eir_len)
	{
		uint8_t field_len = eir[0];
		size_t name_len;

		/* Check for the end of EIR */
		if (field_len == 0)
			break;

		if (offset + field_len > eir_len)
			goto failed;

		switch (eir[1])
		{
		case EIR_NAME_SHORT:
		case EIR_NAME_COMPLETE:
			name_len = field_len - 1;
			if (name_len > buf_len)
				goto failed;

			memcpy(buf, &eir[2], name_len);
			return;
		}

		offset += field_len + 1;
		eir += field_len + 1;
	}

failed:
	snprintf(buf, buf_len, "[]");
}

static void cmd_reset(int ctl, int hdev)
{
	if ((ctl = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI)) < 0) {
		_syslog(LOG_ERR, "Can't open device hci%d\n",hdev);
	}
	// Stop HCI device
	if (ioctl(ctl, HCIDEVDOWN, hdev) < 0) {
		_syslog(LOG_ERR, "Can't down device hci%d\n",hdev);
		return;
	}
	// Start HCI device
	if (ioctl(ctl, HCIDEVUP, hdev) < 0) {
		if (errno == EALREADY)
			return;
		_syslog(LOG_ERR, "Can't init device hci%d\n",hdev);
		return;
	}
}

void *blescan_thread(void *args)
{
	int err, opt, dd, i;
	int dev_id = -1;
	uint8_t own_type = LE_PUBLIC_ADDRESS;
	//own_type = LE_RANDOM_ADDRESS; //Enable privacy
	uint8_t scan_type = 0x01;
	scan_type = 0x00; /* Passive */
	uint8_t filter_type = 0;
	uint8_t filter_policy = 0x00;
	uint16_t interval = htobs(0x0010);
	uint16_t window = htobs(0x0010);
	uint8_t filter_dup = 0x01;
	//filter_dup = 0x00; // not filter dup

	unsigned char buf[HCI_MAX_EVENT_SIZE], *ptr;

	struct hci_filter nf, of;
	socklen_t olen;
	int len;

	char *topic;
	char *message;

	_syslog(LOG_INFO, "ble scan started");

	if (dev_id < 0)
		dev_id = hci_get_route(NULL);

	dd = hci_open_dev(dev_id);
	if (dd < 0)
	{
		_syslog(LOG_ERR, "Could not open device %d", dev_id);
		return;
	}
	err = hci_le_set_scan_parameters(dd, scan_type, interval, window, own_type, filter_policy, 10000);
	if (err < 0)
	{
		_syslog(LOG_ERR, "Set scan parameters failed %d, restarting hci%d", err, dev_id);
		cmd_reset(dd, dev_id);
		err = hci_le_set_scan_parameters(dd, scan_type, interval, window, own_type, filter_policy, 10000);
		if (err < 0)
		{
			_syslog(LOG_ERR, "Set scan parameters failed %d", err);
			return;
		}
	}

	err = hci_le_set_scan_enable(dd, 0x01, filter_dup, 10000);
	if (err < 0)
	{
		_syslog(LOG_ERR, "Enable scan failed");
		return;
	}

	_syslog(LOG_INFO, "LE Scan ...\n");

	olen = sizeof(of);
	if (getsockopt(dd, SOL_HCI, HCI_FILTER, &of, &olen) < 0)
	{
		_syslog(LOG_ERR, "Could not get socket options\n");
		return;
	}

	hci_filter_clear(&nf);
	hci_filter_set_ptype(HCI_EVENT_PKT, &nf);
	hci_filter_set_event(EVT_LE_META_EVENT, &nf);
	//setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(struct timeval));

	if (setsockopt(dd, SOL_HCI, HCI_FILTER, &nf, sizeof(nf)) < 0)
	{
		_syslog(LOG_ERR, "Could not set socket options\n");
		return;
	}
	struct pollfd fds[1];
	fds[0].fd = dd;
	fds[0].events = POLLIN;

	while (1)
	{
		evt_le_meta_event *meta;
		le_advertising_info *info;
		char addr[18];
		if (signal_received != 0)
		{
			_syslog(LOG_DEBUG, "stop=0\n");
			goto done;
		}
		int ret = poll( &fds, 1, 3000);
		if ( ret == -1 ) {
		    _syslog(LOG_ERR, "Poll error\n");
		    goto done;
		} else if ( ret == 0 ) {
			//_syslog(LOG_DEBUG, "blewait\n");
			usleep(10000);
		    continue;
		} 
		else
		{
			if ( fds[0].revents & POLLIN )
		    fds[0].revents = 0;
		    len = read(dd, buf, sizeof(buf));
		    if(len<0) continue;
		    _syslog(LOG_DEBUG, "len=%d\n", len);
		}
		ptr = buf + (1 + HCI_EVENT_HDR_SIZE);
		len -= (1 + HCI_EVENT_HDR_SIZE);

		meta = (void *)ptr;

		if (meta->subevent != 0x02)
		{
			_syslog(LOG_DEBUG, "subevent!=2\n");
			goto done;
		}
		_syslog(LOG_DEBUG, "subevent=%d, len=%d\n", meta->subevent, len);
		/* Ignoring multiple reports */
		info = (le_advertising_info *)(meta->data + 1);
		char name[30];
		memset(name, 0, sizeof(name));
		ba2str(&info->bdaddr, addr);
		eir_parse_name(info->data, info->length, name, sizeof(name) - 1);
		addr[2] = '-';
		addr[5] = '-';
		addr[8] = '-';
		addr[11] = '-';
		addr[14] = '-';

		if(ble_full!=0 || blemacs_size==0) {
			topic = malloc(strlen(config.topic) + 22);
			message = malloc(strlen(name) + 21);
			sprintf(message, "%s - RSSI %d", name, (char)info->data[info->length]);
			sprintf(topic, "%s%s/%s", config.topic, "ble", addr);
			mqtt_publish_once(topic, message);
			free(topic);
			free(message);
		}else{
			for(i = 0; i< blemacs_size;i++) {
				if(strcmp(blemacs[i].addr, addr)==0) {
					blemacs[i].lastseen = (uint32_t) time(NULL);
					blemacs[i].state = 1;
        			topic = malloc(strlen(config.topic) + 22);
        			message = malloc(strlen(name) + 21);
        			sprintf(message, "{'state': %d}", blemacs[i].state);
					sprintf(topic, "%s%s/%s", config.topic, "ble", addr);
        			mqtt_publish_once(topic, message);
					free(topic);
					free(message);
				}
			}
		}
		_syslog(LOG_DEBUG, "%s %s rssi %d\n", addr, name, (char)info->data[info->length]);
	}

done:
	_syslog(LOG_INFO, "ble scan exit");
	setsockopt(dd, SOL_HCI, HCI_FILTER, &of, sizeof(of));

	err = hci_le_set_scan_enable(dd, 0x00, filter_dup, 10000);
	if (err < 0)
	{
		_syslog(LOG_ERR, "Disable scan failed");
		//exit(1);
	}

	hci_close_dev(dd);
	_syslog(LOG_INFO, "ble scan ended");
}
void *btscan_thread(void *args)
{

	inquiry_info *ii = NULL;
	int max_rsp, num_rsp;
	int dev_id, sock, len, flags;
	int i, n;
	char addr[19] = {0};
	char name[249] = {0};
	char *topic;
	char *message;
	_syslog(LOG_INFO, "bt scan started");

	dev_id = hci_get_route(NULL);
	sock = hci_open_dev(dev_id);
	if (dev_id < 0 || sock < 0)
	{
		_syslog(LOG_ERR, "bt opening socket id: %d, s:%d", dev_id, sock);
		return -1;
	}

	len = 8;
	max_rsp = 255;
	int8_t rssi = 0;
	flags = IREQ_CACHE_FLUSH;
	ii = (inquiry_info *)malloc(max_rsp * sizeof(inquiry_info));

	num_rsp = hci_inquiry(dev_id, len, max_rsp, NULL, &ii, flags);
	if (num_rsp < 0)
		_syslog(LOG_ERR, "hci_inquiry");
	_syslog(LOG_INFO, "bt scan ended");

	json_object *root = json_object_new_object();

	for (i = 0; i < num_rsp; i++)
	{
		ba2str(&(ii + i)->bdaddr, addr);
		memset(name, 0, sizeof(name));
		//        if (hci_read_remote_name(sock, &(ii+i)->bdaddr, sizeof(name), name, 0) < 0)
		//            strcpy(name, "[]");

		if (hci_read_remote_name_with_clock_offset(sock, &(ii + i)->bdaddr, (ii + i)->pscan_rep_mode, (ii + i)->clock_offset | 0x8000, sizeof(name), name, 100000) < 0)
			strcpy(name, "[]");

		for (n = 0; n < 248 && name[n]; n++)
		{
			if ((unsigned char)name[i] < 32 || name[i] == 127)
				name[i] = '.';
		}
		name[248] = '\0';

		json_object_object_add(root, addr, json_object_new_string(name));

		addr[2] = '-';
		addr[5] = '-';
		addr[8] = '-';
		addr[11] = '-';
		addr[14] = '-';

		if(ble_full!=0 || blemacs_size==0) {
			topic = malloc(strlen(config.topic) + 22);
			message = malloc(strlen(name) + 41);
			sprintf(message, "name: %s [mode %d, clkoffset 0x%4.4x]", name, (ii + i)->pscan_rep_mode, btohs((ii + i)->clock_offset));
			sprintf(topic, "%s%s/%s", config.topic, "bt", addr);
			if (!mqtt_publish_once(topic, message))
			{
				_syslog(LOG_ERR, "Error: mosquitto_publish bt failed\n");
			}
			free(topic);
			free(message);
		}else{
			for(i = 0; i< blemacs_size;i++) {
				if(strcmp(blemacs[i].addr, addr)==0) {
					blemacs[i].lastseen = (uint32_t) time(NULL);
					blemacs[i].state = 1;
        			topic = malloc(strlen(config.topic) + 22);
        			message = malloc(strlen(name) + 21);
        			sprintf(message, "{'state': %d}", blemacs[i].state);
					sprintf(topic, "%s%s/%s", config.topic, "bt", addr);
					if (!mqtt_publish_once(topic, message))
					{
						_syslog(LOG_ERR, "Error: mosquitto_publish bt = failed\n");
					}
					free(topic);
					free(message);
				}
			}
		}

		_syslog(LOG_ERR, "%s  %s [mode %d, clkoffset 0x%4.4x]\n", addr, name, (ii + i)->pscan_rep_mode, btohs((ii + i)->clock_offset));
	}

	topic = malloc(strlen(config.topic) + 3);
	sprintf(topic, "%s%s", config.topic, "bt");
	if (!mqtt_publish(topic, json_object_to_json_string_ext(root, NULL)))
	{
		_syslog(LOG_ERR, "Error bt publish");
	}

	json_object_put(root);
	free(topic);

	_syslog(LOG_INFO, "bt scan end. found %d", num_rsp);

	bt_free(ii);

	hci_close_dev(sock);

	return;
}
