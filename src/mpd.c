#include <mpd/client.h>
#include <mpd/status.h>
#include <mpd/song.h>
#include <mpd/entity.h>
#include <mpd/search.h>
#include <mpd/tag.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <syslog.h>

#include "mpd.h"
#include "log.h"

static void print_status(struct mpd_status *status)
{
	const struct mpd_audio_format *audio_format;

	_syslog(LOG_INFO, "volume: %i", mpd_status_get_volume(status));
	_syslog(LOG_INFO, "repeat: %i", mpd_status_get_repeat(status));
	_syslog(LOG_INFO, "single: %i", mpd_status_get_single(status));
	_syslog(LOG_INFO, "consume: %i", mpd_status_get_consume(status));
	_syslog(LOG_INFO, "random: %i", mpd_status_get_random(status));
	_syslog(LOG_INFO, "queue version: %u", mpd_status_get_queue_version(status));
	_syslog(LOG_INFO, "queue length: %i", mpd_status_get_queue_length(status));

	if (mpd_status_get_state(status) == MPD_STATE_PLAY || mpd_status_get_state(status) == MPD_STATE_PAUSE)
	{
		_syslog(LOG_INFO, "song: %i", mpd_status_get_song_pos(status));
		_syslog(LOG_INFO, "elaspedTime: %i", mpd_status_get_elapsed_time(status));
		_syslog(LOG_INFO, "elasped_ms: %u\n", mpd_status_get_elapsed_ms(status));
		_syslog(LOG_INFO, "totalTime: %i", mpd_status_get_total_time(status));
		_syslog(LOG_INFO, "bitRate: %i", mpd_status_get_kbit_rate(status));
	}

	audio_format = mpd_status_get_audio_format(status);
	if (audio_format != NULL)
	{
		_syslog(LOG_INFO, "sampleRate: %i\n", audio_format->sample_rate);
		_syslog(LOG_INFO, "bits: %i\n", audio_format->bits);
		_syslog(LOG_INFO, "channels: %i\n", audio_format->channels);
	}
}

void mpd_play(char *playfile)
{
	bool success;
	struct mpd_connection *conn = mpd_connection_new(NULL, 0, 30000);
	if (conn == NULL)
	{
		_syslog(LOG_ERR, "%s", "Out of memory");
		return;
	}
	if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS)
	{
		_syslog(LOG_ERR, "%s", mpd_connection_get_error_message(conn));
		mpd_connection_free(conn);
		conn = NULL;
		return;
	}
	if (!conn)
		return;

	if (config.verbosity > 5)
	{
		int i;
		for (i = 0; i < 3; ++i)
		{
			_syslog(LOG_INFO, "version[%i]: %i", i, mpd_connection_get_server_version(conn)[i]);
		}
		struct mpd_status *status;
		status = mpd_run_status(conn);
		if (!status)
		{
			_syslog(LOG_ERR, "%s", mpd_connection_get_error_message(conn));
			return;
		}
		print_status(status);
		mpd_status_free(status);
		mpd_response_finish(conn);
	}

	mpd_run_stop(conn);
	if (playfile != NULL && strlen(playfile) > 0)
	{
		if (!mpd_command_list_begin(conn, false))
		{
			_syslog(LOG_ERR, "MPD error: %s\n", mpd_connection_get_error_message(conn));
		}
		_syslog(LOG_INFO, "adding: %s\n", playfile);
		mpd_send_add(conn, playfile);
		if (!mpd_command_list_end(conn))
		{
			_syslog(LOG_ERR, "MPD error: %s\n", mpd_connection_get_error_message(conn));
		}
		if (!mpd_response_finish(conn))
		{
			_syslog(LOG_ERR, "MPD error: %s\n", mpd_connection_get_error_message(conn));
		}
		success = mpd_run_play(conn);
		if (!success)
		{
			_syslog(LOG_ERR, "MPD error: %s\n", mpd_connection_get_error_message(conn));
		}
	}
	mpd_connection_free(conn);
}
