/* upnp_transport.c - UPnP AVTransport routines
 *
 * Copyright (C) 2005-2007   Ivo Clarysse
 *
 * This file is part of GMediaRender.
 *
 * GMediaRender is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GMediaRender is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GMediaRender; if not, write to the Free Software 
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, 
 * MA 02110-1301, USA.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

//#include <glib.h>

#include <upnp/upnp.h>
#include <upnp/ithread.h>
#include <upnp/upnptools.h>


#include "logging.h"

#include "xmlescape.h"
#include "upnp.h"
#include "upnp_device.h"
#include "upnp_transport.h"
#include "output_gstreamer.h"

#include "debug.h"


//#define TRANSPORT_SERVICE "urn:upnp-org:serviceId:AVTransport"
#define TRANSPORT_SERVICE "urn:schemas-upnp-org:service:AVTransport:1"
#define TRANSPORT_TYPE "urn:schemas-upnp-org:service:AVTransport:1"
#define TRANSPORT_SCPD_URL "/upnp/rendertransportSCPD.xml"
#define TRANSPORT_CONTROL_URL "/upnp/control/rendertransport1"
#define TRANSPORT_EVENT_URL "/upnp/event/rendertransport1"


struct transport_event{
	struct action_event event;
	struct Upnp_Action_Request request;
	bool init ;
};
struct transport_event g_event ;



typedef enum {
	TRANSPORT_VAR_TRANSPORT_STATUS,
	TRANSPORT_VAR_NEXT_AV_URI,
	TRANSPORT_VAR_NEXT_AV_URI_META,
	TRANSPORT_VAR_CUR_TRACK_META,
	TRANSPORT_VAR_REL_CTR_POS,
	TRANSPORT_VAR_AAT_INSTANCE_ID,
	TRANSPORT_VAR_AAT_SEEK_TARGET,
	TRANSPORT_VAR_PLAY_MEDIUM,
	TRANSPORT_VAR_REL_TIME_POS,
	TRANSPORT_VAR_REC_MEDIA,
	TRANSPORT_VAR_CUR_PLAY_MODE,
	TRANSPORT_VAR_TRANSPORT_PLAY_SPEED,
	TRANSPORT_VAR_PLAY_MEDIA,
	TRANSPORT_VAR_ABS_TIME_POS,
	TRANSPORT_VAR_CUR_TRACK,
	TRANSPORT_VAR_CUR_TRACK_URI,
	TRANSPORT_VAR_CUR_TRANSPORT_ACTIONS,
	TRANSPORT_VAR_NR_TRACKS,
	TRANSPORT_VAR_AV_URI,
	TRANSPORT_VAR_ABS_CTR_POS,
	TRANSPORT_VAR_CUR_REC_QUAL_MODE,
	TRANSPORT_VAR_CUR_MEDIA_DUR,
	TRANSPORT_VAR_AAT_SEEK_MODE,
	TRANSPORT_VAR_AV_URI_META,
	TRANSPORT_VAR_REC_MEDIUM,

	TRANSPORT_VAR_REC_MEDIUM_WR_STATUS,
	TRANSPORT_VAR_LAST_CHANGE,
	TRANSPORT_VAR_CUR_TRACK_DUR,
	TRANSPORT_VAR_TRANSPORT_STATE,
	TRANSPORT_VAR_POS_REC_QUAL_MODE,
	TRANSPORT_VAR_UNKNOWN,
	TRANSPORT_VAR_COUNT
} transport_variable;

typedef enum {
	TRANSPORT_CMD_GETCURRENTTRANSPORTACTIONS,
	TRANSPORT_CMD_GETDEVICECAPABILITIES,
	TRANSPORT_CMD_GETMEDIAINFO,
	TRANSPORT_CMD_GETPOSITIONINFO,
	TRANSPORT_CMD_GETTRANSPORTINFO,
	TRANSPORT_CMD_GETTRANSPORTSETTINGS,
	TRANSPORT_CMD_NEXT,
	TRANSPORT_CMD_PAUSE,
	TRANSPORT_CMD_PLAY,
	TRANSPORT_CMD_PREVIOUS,
	TRANSPORT_CMD_SEEK,
	TRANSPORT_CMD_SETAVTRANSPORTURI,             
	TRANSPORT_CMD_SETPLAYMODE,
	TRANSPORT_CMD_STOP,
	TRANSPORT_CMD_SETNEXTAVTRANSPORTURI,
	//TRANSPORT_CMD_RECORD,
	//TRANSPORT_CMD_SETRECORDQUALITYMODE,
	TRANSPORT_CMD_UNKNOWN,                   
	TRANSPORT_CMD_COUNT
} transport_cmd ;

static const char *transport_variables[] = {
	[TRANSPORT_VAR_TRANSPORT_STATE] = "TransportState",
	[TRANSPORT_VAR_TRANSPORT_STATUS] = "TransportStatus",
	[TRANSPORT_VAR_PLAY_MEDIUM] = "PlaybackStorageMedium",
	[TRANSPORT_VAR_REC_MEDIUM] = "RecordStorageMedium",
	[TRANSPORT_VAR_PLAY_MEDIA] = "PossiblePlaybackStorageMedia",
	[TRANSPORT_VAR_REC_MEDIA] = "PossibleRecordStorageMedia",
	[TRANSPORT_VAR_CUR_PLAY_MODE] = "CurrentPlayMode",
	[TRANSPORT_VAR_TRANSPORT_PLAY_SPEED] = "TransportPlaySpeed",
	[TRANSPORT_VAR_REC_MEDIUM_WR_STATUS] = "RecordMediumWriteStatus",
	[TRANSPORT_VAR_CUR_REC_QUAL_MODE] = "CurrentRecordQualityMode",
	[TRANSPORT_VAR_POS_REC_QUAL_MODE] = "PossibleRecordQualityModes",
	[TRANSPORT_VAR_NR_TRACKS] = "NumberOfTracks",
	[TRANSPORT_VAR_CUR_TRACK] = "CurrentTrack",
	[TRANSPORT_VAR_CUR_TRACK_DUR] = "CurrentTrackDuration",
	[TRANSPORT_VAR_CUR_MEDIA_DUR] = "CurrentMediaDuration",
	[TRANSPORT_VAR_CUR_TRACK_META] = "CurrentTrackMetaData",
	[TRANSPORT_VAR_CUR_TRACK_URI] = "CurrentTrackURI",
	[TRANSPORT_VAR_AV_URI] = "AVTransportURI",
	[TRANSPORT_VAR_AV_URI_META] = "AVTransportURIMetaData",
	[TRANSPORT_VAR_NEXT_AV_URI] = "NextAVTransportURI",
	[TRANSPORT_VAR_NEXT_AV_URI_META] = "NextAVTransportURIMetaData",
	[TRANSPORT_VAR_REL_TIME_POS] = "RelativeTimePosition",
	[TRANSPORT_VAR_ABS_TIME_POS] = "AbsoluteTimePosition",
	[TRANSPORT_VAR_REL_CTR_POS] = "RelativeCounterPosition",
	[TRANSPORT_VAR_ABS_CTR_POS] = "AbsoluteCounterPosition",
	[TRANSPORT_VAR_LAST_CHANGE] = "LastChange",
	[TRANSPORT_VAR_AAT_SEEK_MODE] = "A_ARG_TYPE_SeekMode",
	[TRANSPORT_VAR_AAT_SEEK_TARGET] = "A_ARG_TYPE_SeekTarget",
	[TRANSPORT_VAR_AAT_INSTANCE_ID] = "A_ARG_TYPE_InstanceID",
	[TRANSPORT_VAR_CUR_TRANSPORT_ACTIONS] = "CurrentTransportActions",	/* optional */
	[TRANSPORT_VAR_UNKNOWN] = NULL
};

static char *transport_values[] = {
	[TRANSPORT_VAR_TRANSPORT_STATE] = "STOPPED",
	[TRANSPORT_VAR_TRANSPORT_STATUS] = "OK",
	[TRANSPORT_VAR_PLAY_MEDIUM] = "UNKNOWN",
	[TRANSPORT_VAR_REC_MEDIUM] = "NOT_IMPLEMENTED",
	[TRANSPORT_VAR_PLAY_MEDIA] = "NETWORK,UNKNOWN",
	[TRANSPORT_VAR_REC_MEDIA] = "NOT_IMPLEMENTED",
	[TRANSPORT_VAR_CUR_PLAY_MODE] = "NORMAL",
	[TRANSPORT_VAR_TRANSPORT_PLAY_SPEED] = "1",
	[TRANSPORT_VAR_REC_MEDIUM_WR_STATUS] = "NOT_IMPLEMENTED",
	[TRANSPORT_VAR_CUR_REC_QUAL_MODE] = "NOT_IMPLEMENTED",
	[TRANSPORT_VAR_POS_REC_QUAL_MODE] = "NOT_IMPLEMENTED",
	[TRANSPORT_VAR_NR_TRACKS] = "0",
	[TRANSPORT_VAR_CUR_TRACK] = "0",
	[TRANSPORT_VAR_CUR_TRACK_DUR] = "00:00:00",
	[TRANSPORT_VAR_CUR_MEDIA_DUR] = "",
	[TRANSPORT_VAR_CUR_TRACK_META] = "",
	[TRANSPORT_VAR_CUR_TRACK_URI] = "",
	[TRANSPORT_VAR_AV_URI] = "",
	[TRANSPORT_VAR_AV_URI_META] = "",
	[TRANSPORT_VAR_NEXT_AV_URI] = "",
	[TRANSPORT_VAR_NEXT_AV_URI_META] = "",
	[TRANSPORT_VAR_REL_TIME_POS] = "NOT_IMPLEMENTED",
	[TRANSPORT_VAR_ABS_TIME_POS] = "NOT_IMPLEMENTED",
	[TRANSPORT_VAR_REL_CTR_POS] = "2147483647",
	[TRANSPORT_VAR_ABS_CTR_POS] = "2147483647",
        [TRANSPORT_VAR_LAST_CHANGE] = "<Event xmlns=\"urn:schemas-upnp-org:metadata-1-0/AVT/\"/>",

	[TRANSPORT_VAR_AAT_SEEK_MODE] = "TRACK_NR",
	[TRANSPORT_VAR_AAT_SEEK_TARGET] = "",
	[TRANSPORT_VAR_AAT_INSTANCE_ID] = "0",
	[TRANSPORT_VAR_CUR_TRANSPORT_ACTIONS] = "Play,Pause,Seek,Stop",
	[TRANSPORT_VAR_UNKNOWN] = NULL
};

static char *g_str_transport_buf = NULL;

static const char *transport_states[] = {
	"STOPPED",
	"PAUSED_PLAYBACK",
	"PAUSED_RECORDING",
	"PLAYING",
	"RECORDING",
	"TRANSITIONING",
	"NO_MEDIA_PRESENT",
	NULL
};
static const char *transport_stati[] = {
	"OK",
	"ERROR_OCCURRED",
	" vendor-defined ",
	NULL
};
static const char *media[] = {
	"UNKNOWN",
	"DV",
	"MINI-DV",
	"VHS",
	"W-VHS",
	"S-VHS",
	"D-VHS",
	"VHSC",
	"VIDEO8",
	"HI8",
	"CD-ROM",
	"CD-DA",
	"CD-R",
	"CD-RW",
	"VIDEO-CD",
	"SACD",
	"MD-AUDIO",
	"MD-PICTURE",
	"DVD-ROM",
	"DVD-VIDEO",
	"DVD-R",
	"DVD+RW",
	"DVD-RW",
	"DVD-RAM",
	"DVD-AUDIO",
	"DAT",
	"LD",
	"HDD",
	"MICRO-MV",
	"NETWORK",
	"NONE",
	"NOT_IMPLEMENTED",
	" vendor-defined ",
	NULL
};

static const char *playmodi[] = {
	"NORMAL",
	//"SHUFFLE",
	//"REPEAT_ONE",
	"REPEAT_ALL",
	//"RANDOM",
	//"DIRECT_1",
	"INTRO",
	NULL
};

static const char *playspeeds[] = {
	"1",
	" vendor-defined ",
	NULL
};

static const char *rec_write_stati[] = {
	"WRITABLE",
	"PROTECTED",
	"NOT_WRITABLE",
	"UNKNOWN",
	"NOT_IMPLEMENTED",
	NULL
};

static const char *rec_quality_modi[] = {
	"0:EP",
	"1:LP",
	"2:SP",
	"0:BASIC",
	"1:MEDIUM",
	"2:HIGH",
	"NOT_IMPLEMENTED",
	" vendor-defined ",
	NULL
};

static const char *aat_seekmodi[] = {
	"ABS_TIME",
	"REL_TIME",
	"ABS_COUNT",
	"REL_COUNT",
	"TRACK_NR",
	"CHANNEL_FREQ",
	"TAPE-INDEX",
	"FRAME",
	NULL
};

static struct param_range track_range = {
	0,
	4294967295LL,
	1
};

static struct param_range track_nr_range = {
	0,
	4294967295LL,
	0
};

static struct var_meta transport_var_meta[] = {
	[TRANSPORT_VAR_TRANSPORT_STATE] =		{ SENDEVENT_NO, DATATYPE_STRING, transport_states, NULL },
	//[TRANSPORT_VAR_TRANSPORT_STATE] =		{ SENDEVENT_YES, DATATYPE_STRING, transport_states, NULL },
	
	[TRANSPORT_VAR_TRANSPORT_STATUS] =		{ SENDEVENT_NO, DATATYPE_STRING, transport_stati, NULL },
	[TRANSPORT_VAR_PLAY_MEDIUM] =			{ SENDEVENT_NO, DATATYPE_STRING, media, NULL },
	[TRANSPORT_VAR_REC_MEDIUM] =			{ SENDEVENT_NO, DATATYPE_STRING, media, NULL },
	[TRANSPORT_VAR_PLAY_MEDIA] =			{ SENDEVENT_NO, DATATYPE_STRING, NULL, NULL },
	[TRANSPORT_VAR_REC_MEDIA] =			{ SENDEVENT_NO, DATATYPE_STRING, NULL, NULL },
	[TRANSPORT_VAR_CUR_PLAY_MODE] =			{ SENDEVENT_NO, DATATYPE_STRING, playmodi, NULL, "NORMAL" },
	[TRANSPORT_VAR_TRANSPORT_PLAY_SPEED] =		{ SENDEVENT_NO, DATATYPE_STRING, playspeeds, NULL },
	[TRANSPORT_VAR_REC_MEDIUM_WR_STATUS] =		{ SENDEVENT_NO, DATATYPE_STRING, rec_write_stati, NULL },
	[TRANSPORT_VAR_CUR_REC_QUAL_MODE] =		{ SENDEVENT_NO, DATATYPE_STRING, rec_quality_modi, NULL },
	[TRANSPORT_VAR_POS_REC_QUAL_MODE] =		{ SENDEVENT_NO, DATATYPE_STRING, NULL, NULL },
	[TRANSPORT_VAR_NR_TRACKS] =			{ SENDEVENT_NO, DATATYPE_UI4, NULL, &track_nr_range }, /* no step */
	[TRANSPORT_VAR_CUR_TRACK] =			{ SENDEVENT_NO, DATATYPE_UI4, NULL, &track_range },
	[TRANSPORT_VAR_CUR_TRACK_DUR] =			{ SENDEVENT_NO, DATATYPE_STRING, NULL, NULL },
	[TRANSPORT_VAR_CUR_MEDIA_DUR] =			{ SENDEVENT_NO, DATATYPE_STRING, NULL, NULL },
	[TRANSPORT_VAR_CUR_TRACK_META] =		{ SENDEVENT_NO, DATATYPE_STRING, NULL, NULL },
	[TRANSPORT_VAR_CUR_TRACK_URI] =			{ SENDEVENT_NO, DATATYPE_STRING, NULL, NULL },
	[TRANSPORT_VAR_AV_URI] =			{ SENDEVENT_NO, DATATYPE_STRING, NULL, NULL },
	[TRANSPORT_VAR_AV_URI_META] =			{ SENDEVENT_NO, DATATYPE_STRING, NULL, NULL },
	[TRANSPORT_VAR_NEXT_AV_URI] =			{ SENDEVENT_NO, DATATYPE_STRING, NULL, NULL },
	[TRANSPORT_VAR_NEXT_AV_URI_META] =		{ SENDEVENT_NO, DATATYPE_STRING, NULL, NULL },
	[TRANSPORT_VAR_REL_TIME_POS] =			{ SENDEVENT_NO, DATATYPE_STRING, NULL, NULL },
	[TRANSPORT_VAR_ABS_TIME_POS] =			{ SENDEVENT_NO, DATATYPE_STRING, NULL, NULL },
	[TRANSPORT_VAR_REL_CTR_POS] =			{ SENDEVENT_NO, DATATYPE_I4, NULL, NULL },
	[TRANSPORT_VAR_ABS_CTR_POS] =			{ SENDEVENT_NO, DATATYPE_I4, NULL, NULL },
	[TRANSPORT_VAR_LAST_CHANGE] =			{ SENDEVENT_YES, DATATYPE_STRING, NULL, NULL },
	[TRANSPORT_VAR_AAT_SEEK_MODE] =			{ SENDEVENT_NO, DATATYPE_STRING, aat_seekmodi, NULL },
	[TRANSPORT_VAR_AAT_SEEK_TARGET] =		{ SENDEVENT_NO, DATATYPE_STRING, NULL, NULL },
	[TRANSPORT_VAR_AAT_INSTANCE_ID] =		{ SENDEVENT_NO, DATATYPE_UI4, NULL, NULL },
	[TRANSPORT_VAR_CUR_TRANSPORT_ACTIONS] =		{ SENDEVENT_NO, DATATYPE_STRING, NULL, NULL },
	[TRANSPORT_VAR_UNKNOWN] =			{ SENDEVENT_NO, DATATYPE_UNKNOWN, NULL, NULL }
};	

static struct argument *arguments_setavtransporturi[] = {
        & (struct argument) { "InstanceID", PARAM_DIR_IN, TRANSPORT_VAR_AAT_INSTANCE_ID },
        & (struct argument) { "CurrentURI", PARAM_DIR_IN, TRANSPORT_VAR_AV_URI },
        & (struct argument) { "CurrentURIMetaData", PARAM_DIR_IN, TRANSPORT_VAR_AV_URI_META },
        NULL
};

static struct argument *arguments_setnextavtransporturi[] = {
        & (struct argument) { "InstanceID", PARAM_DIR_IN, TRANSPORT_VAR_AAT_INSTANCE_ID },
        & (struct argument) { "NextURI", PARAM_DIR_IN, TRANSPORT_VAR_NEXT_AV_URI },
        & (struct argument) { "NextURIMetaData", PARAM_DIR_IN, TRANSPORT_VAR_NEXT_AV_URI_META },
        NULL
};

static struct argument *arguments_getmediainfo[] = {
        & (struct argument) { "InstanceID", PARAM_DIR_IN, TRANSPORT_VAR_AAT_INSTANCE_ID },
        & (struct argument) { "NrTracks", PARAM_DIR_OUT, TRANSPORT_VAR_NR_TRACKS },
        & (struct argument) { "MediaDuration", PARAM_DIR_OUT, TRANSPORT_VAR_CUR_MEDIA_DUR },
        & (struct argument) { "CurrentURI", PARAM_DIR_OUT, TRANSPORT_VAR_AV_URI },
        & (struct argument) { "CurrentURIMetaData", PARAM_DIR_OUT, TRANSPORT_VAR_AV_URI_META },
        & (struct argument) { "NextURI", PARAM_DIR_OUT, TRANSPORT_VAR_NEXT_AV_URI },
        & (struct argument) { "NextURIMetaData", PARAM_DIR_OUT, TRANSPORT_VAR_NEXT_AV_URI_META },
        & (struct argument) { "PlayMedium", PARAM_DIR_OUT, TRANSPORT_VAR_PLAY_MEDIUM },
        & (struct argument) { "RecordMedium", PARAM_DIR_OUT, TRANSPORT_VAR_REC_MEDIUM },
        & (struct argument) { "WriteStatus", PARAM_DIR_OUT, TRANSPORT_VAR_REC_MEDIUM_WR_STATUS },
        NULL
};

static struct argument *arguments_gettransportinfo[] = {
        & (struct argument) { "InstanceID", PARAM_DIR_IN, TRANSPORT_VAR_AAT_INSTANCE_ID },
        & (struct argument) { "CurrentTransportState", PARAM_DIR_OUT, TRANSPORT_VAR_TRANSPORT_STATE },
        & (struct argument) { "CurrentTransportStatus", PARAM_DIR_OUT, TRANSPORT_VAR_TRANSPORT_STATUS },
        & (struct argument) { "CurrentSpeed", PARAM_DIR_OUT, TRANSPORT_VAR_TRANSPORT_PLAY_SPEED },
        NULL
};

static struct argument *arguments_getpositioninfo[] = {
        & (struct argument) { "InstanceID", PARAM_DIR_IN, TRANSPORT_VAR_AAT_INSTANCE_ID },
        & (struct argument) { "Track", PARAM_DIR_OUT, TRANSPORT_VAR_CUR_TRACK },
        & (struct argument) { "TrackDuration", PARAM_DIR_OUT, TRANSPORT_VAR_CUR_TRACK_DUR },
        & (struct argument) { "TrackMetaData", PARAM_DIR_OUT, TRANSPORT_VAR_CUR_TRACK_META },
        & (struct argument) { "TrackURI", PARAM_DIR_OUT, TRANSPORT_VAR_CUR_TRACK_URI },
        & (struct argument) { "RelTime", PARAM_DIR_OUT, TRANSPORT_VAR_REL_TIME_POS },
        & (struct argument) { "AbsTime", PARAM_DIR_OUT, TRANSPORT_VAR_ABS_TIME_POS },
        & (struct argument) { "RelCount", PARAM_DIR_OUT, TRANSPORT_VAR_REL_CTR_POS },
        & (struct argument) { "AbsCount", PARAM_DIR_OUT, TRANSPORT_VAR_ABS_CTR_POS },
        NULL
};

static struct argument *arguments_getdevicecapabilities[] = {
        & (struct argument) { "InstanceID", PARAM_DIR_IN, TRANSPORT_VAR_AAT_INSTANCE_ID },
        & (struct argument) { "PlayMedia", PARAM_DIR_OUT, TRANSPORT_VAR_PLAY_MEDIA },
        & (struct argument) { "RecMedia", PARAM_DIR_OUT, TRANSPORT_VAR_REC_MEDIA },
        & (struct argument) { "RecQualityModes", PARAM_DIR_OUT, TRANSPORT_VAR_POS_REC_QUAL_MODE },
	NULL
};

static struct argument *arguments_gettransportsettings[] = {
        & (struct argument) { "InstanceID", PARAM_DIR_IN, TRANSPORT_VAR_AAT_INSTANCE_ID },
        & (struct argument) { "PlayMode", PARAM_DIR_OUT, TRANSPORT_VAR_CUR_PLAY_MODE },
        & (struct argument) { "RecQualityMode", PARAM_DIR_OUT, TRANSPORT_VAR_CUR_REC_QUAL_MODE },
	NULL
};

static struct argument *arguments_stop[] = {
        & (struct argument) { "InstanceID", PARAM_DIR_IN, TRANSPORT_VAR_AAT_INSTANCE_ID },
	NULL
};
static struct argument *arguments_play[] = {
        & (struct argument) { "InstanceID", PARAM_DIR_IN, TRANSPORT_VAR_AAT_INSTANCE_ID },
        & (struct argument) { "Speed", PARAM_DIR_IN, TRANSPORT_VAR_TRANSPORT_PLAY_SPEED },
	NULL
};
static struct argument *arguments_pause[] = {
        & (struct argument) { "InstanceID", PARAM_DIR_IN, TRANSPORT_VAR_AAT_INSTANCE_ID },
	NULL
};
//static struct argument *arguments_record[] = {
//        & (struct argument) { "InstanceID", PARAM_DIR_IN, TRANSPORT_VAR_AAT_INSTANCE_ID },
//	NULL
//};

static struct argument *arguments_seek[] = {
        & (struct argument) { "InstanceID", PARAM_DIR_IN, TRANSPORT_VAR_AAT_INSTANCE_ID },
        & (struct argument) { "Unit", PARAM_DIR_IN, TRANSPORT_VAR_AAT_SEEK_MODE },
        & (struct argument) { "Target", PARAM_DIR_IN, TRANSPORT_VAR_AAT_SEEK_TARGET },
	NULL
};
static struct argument *arguments_next[] = {
        & (struct argument) { "InstanceID", PARAM_DIR_IN, TRANSPORT_VAR_AAT_INSTANCE_ID },
	NULL
};
static struct argument *arguments_previous[] = {
        & (struct argument) { "InstanceID", PARAM_DIR_IN, TRANSPORT_VAR_AAT_INSTANCE_ID },
	NULL
};
static struct argument *arguments_setplaymode[] = {
        & (struct argument) { "InstanceID", PARAM_DIR_IN, TRANSPORT_VAR_AAT_INSTANCE_ID },
        & (struct argument) { "NewPlayMode", PARAM_DIR_IN, TRANSPORT_VAR_CUR_PLAY_MODE },
	NULL
};
//static struct argument *arguments_setrecordqualitymode[] = {
//        & (struct argument) { "InstanceID", PARAM_DIR_IN, TRANSPORT_VAR_AAT_INSTANCE_ID },
//        & (struct argument) { "NewRecordQualityMode", PARAM_DIR_IN, TRANSPORT_VAR_CUR_REC_QUAL_MODE },
//	NULL
//};
static struct argument *arguments_getcurrenttransportactions[] = {
        & (struct argument) { "InstanceID", PARAM_DIR_IN, TRANSPORT_VAR_AAT_INSTANCE_ID },
        & (struct argument) { "Actions", PARAM_DIR_OUT, TRANSPORT_VAR_CUR_TRANSPORT_ACTIONS },
	NULL
};


static struct argument **argument_list[] = {
	[TRANSPORT_CMD_SETAVTRANSPORTURI] =         arguments_setavtransporturi,
	[TRANSPORT_CMD_GETDEVICECAPABILITIES] =     arguments_getdevicecapabilities,
	[TRANSPORT_CMD_GETMEDIAINFO] =              arguments_getmediainfo,
	[TRANSPORT_CMD_SETNEXTAVTRANSPORTURI] =     arguments_setnextavtransporturi,
	[TRANSPORT_CMD_GETTRANSPORTINFO] =          arguments_gettransportinfo,
	[TRANSPORT_CMD_GETPOSITIONINFO] =           arguments_getpositioninfo,
	[TRANSPORT_CMD_GETTRANSPORTSETTINGS] =      arguments_gettransportsettings,
	[TRANSPORT_CMD_STOP] =                      arguments_stop,
	[TRANSPORT_CMD_PLAY] =                      arguments_play,
	[TRANSPORT_CMD_PAUSE] =                     arguments_pause,
	//[TRANSPORT_CMD_RECORD] =                    arguments_record,
	[TRANSPORT_CMD_SEEK] =                      arguments_seek,
	[TRANSPORT_CMD_NEXT] =                      arguments_next,
	[TRANSPORT_CMD_PREVIOUS] =                  arguments_previous,
	[TRANSPORT_CMD_SETPLAYMODE] =               arguments_setplaymode,
	//[TRANSPORT_CMD_SETRECORDQUALITYMODE] =      arguments_setrecordqualitymode,
	[TRANSPORT_CMD_GETCURRENTTRANSPORTACTIONS] = arguments_getcurrenttransportactions,
	[TRANSPORT_CMD_UNKNOWN] =	NULL
};

/* protects transport_values, and service-specific state */

static ithread_mutex_t transport_mutex;

enum _transport_state {
	TRANSPORT_STOPPED,
	TRANSPORT_PLAYING,
	TRANSPORT_TRANSITIONING,	/* optional */
	TRANSPORT_PAUSED_PLAYBACK,	/* optional */
	TRANSPORT_PAUSED_RECORDING,	/* optional */
	TRANSPORT_RECORDING,	/* optional */
	TRANSPORT_NO_MEDIA_PRESENT	/* optional */
};

static enum _transport_state transport_state = TRANSPORT_STOPPED;



static int get_media_info(struct action_event *event)
{
	char *value;
	int rc;
	ENTER();

	value = upnp_get_string(event, "InstanceID");
	if (value == NULL) {
		rc = -1;
		goto out;
	}
	//printf("%s: InstanceID='%s'\n", __FUNCTION__, value);
	free(value);

	//struct service *service = event->service;

	//get_play_time(&service->variable_values[TRANSPORT_VAR_CUR_MEDIA_DUR],&service->variable_values[TRANSPORT_VAR_CUR_MEDIA_DUR]);
	//printf(" \n------get_position_info  playtime=%s\n",service->variable_values[TRANSPORT_VAR_CUR_MEDIA_DUR]);
	//struct player_sta *sta = get_player_sta();
 	//service->variable_values[TRANSPORT_VAR_REL_TIME_POS] =strdup(sta->real_time);
	//service->variable_values[TRANSPORT_VAR_CUR_TRACK_DUR] =strdup(sta->dur_time);

	rc = upnp_append_variable(event, TRANSPORT_VAR_NR_TRACKS,
				  "NrTracks");
	if (rc)
		goto out;

	rc = upnp_append_variable(event, TRANSPORT_VAR_CUR_MEDIA_DUR,
				  "MediaDuration");
	if (rc)
		goto out;

	rc = upnp_append_variable(event, TRANSPORT_VAR_AV_URI,
				  "CurrentURI");
	if (rc)
		goto out;

	rc = upnp_append_variable(event, TRANSPORT_VAR_AV_URI_META,
				  "CurrentURIMetaData");
	if (rc)
		goto out;

	rc = upnp_append_variable(event, TRANSPORT_VAR_NEXT_AV_URI,
				  "NextURI");
	if (rc)
		goto out;

	rc = upnp_append_variable(event, TRANSPORT_VAR_NEXT_AV_URI_META,
				  "NextURIMetaData");
	if (rc)
		goto out;

	rc = upnp_append_variable(event, TRANSPORT_VAR_REC_MEDIA,
				  "PlayMedium");
	if (rc)
		goto out;

	rc = upnp_append_variable(event, TRANSPORT_VAR_REC_MEDIUM,
				  "RecordMedium");
	if (rc)
		goto out;

	rc = upnp_append_variable(event,
				  TRANSPORT_VAR_REC_MEDIUM_WR_STATUS,
				  "WriteStatus");
	if (rc)
		goto out;

      out:
	return rc;
}


static void notify_lastchange(struct action_event *event, char *value)
{
	const char *varnames[] = {
		"LastChange",
		NULL
	};
	char *varvalues[] = {
		NULL, NULL
	};

	//printf("Event: '%s'\n", value);
	varvalues[0] = value;


	transport_values[TRANSPORT_VAR_LAST_CHANGE] = value;
	UpnpNotify(device_handle, event->request->DevUDN,
		   event->request->ServiceID, varnames,
		   (const char **) varvalues, 1);
}

/* warning - does not lock service mutex */
static void change_varok(struct action_event *event, int varnum,
		       char *new_value)
{
	char *buf;
	int ulRet = 0;
	char *tmp;

	ENTER();
	
	if ((varnum < 0) || (varnum >= TRANSPORT_VAR_UNKNOWN)) {
		LEAVE();
		return;
	}
	if (new_value == NULL) {
		LEAVE();
		return;
	}
	
	free(transport_values[varnum]);
	transport_values[varnum] = NULL;
	
	transport_values[varnum] = strdup(new_value);
	if (NULL == transport_values[varnum])
	{
		debug_printf(MSG_ERROR, "strdup\n");
		return;
	}
	/*
	asprintf(&buf,
		 "<Event xmlns = \"urn:schemas-upnp-org:metadata-1-0/AVT/\"><InstanceID val=\"0\"><%s val=\"%s\"/></InstanceID></Event>",
		 transport_variables[varnum], xmlescape(transport_values[varnum], 1));
		 */
	tmp = xmlescape(transport_values[varnum], 1);
	ulRet = asprintf(&buf,
		 "&lt;Event xmlns = &quot;urn:schemas-upnp-org:metadata-1-0/AVT/&quot;&gt;&lt;InstanceID val=&quot;0&quot;&gt;&lt;%s val=&quot;%s&quot;/&gt;&lt;/InstanceID&gt;&lt;/Event&gt;",
		 transport_variables[varnum], tmp);
	free(tmp);
	
	if (ulRet < 0)
	{
	    debug_printf(MSG_ERROR, "asprintf\n");
		return;
	}
	
	notify_lastchange(event, buf);
	//printf("----ok-2-\n");
	free(buf);
	
	LEAVE();

	return;
}


/* warning - does not lock service mutex */
static void change_var_trans(struct action_event *event,int varcount, 
                                  int varnum[],char *new_value[])
{
	char temp[NOTIFY_BUF_LEN];
	int len = 0;
	int i = 0;
	char *tmp = NULL;
	
	ENTER();

	if (NULL == event)
	{
	    debug_printf(MSG_ERROR, "NULL p\n");
		return;
	}

	if (varcount > 2)
	{
	    debug_printf(MSG_ERROR, "varcount too large:%d\n", varcount);
		return;
	}
	
	//check
	for(i = 0;i<varcount;i++){
		if ((varnum[i] < 0) || (varnum[i] >= TRANSPORT_VAR_UNKNOWN)) {
			LEAVE();
			return;
		}
		if (new_value[i] == NULL) {
			LEAVE();
			return;
		}
	}
	
	strcpy(g_str_transport_buf,
		"&lt;Event xmlns = &quot;urn:schemas-upnp-org:metadata-1-0/AVT/&quot;&gt;\n&lt;InstanceID val=&quot;0&quot;&gt;\n");
	for(i = 0; i<varcount; i++)
	{
	    free(transport_values[varnum[i]]);
	    transport_values[varnum[i]]= NULL;
		
		transport_values[varnum[i]] = strdup(new_value[i]);
		if (NULL == transport_values[varnum[i]])
		{
			debug_printf(MSG_ERROR, "strdup\n");
			return;
		}

        tmp = xmlescape(transport_values[varnum[i]], 1);
		len = sprintf(temp, "&lt;%s val=&quot;%s&quot;/&gt;\n",
		        transport_variables[varnum[i]], tmp);
		free(tmp);

		strcat(g_str_transport_buf, temp);
	}
	strcat(g_str_transport_buf, "&lt;/InstanceID&gt;\n&lt;/Event&gt;");

	notify_lastchange(event, g_str_transport_buf);
	
	LEAVE();
	return;
}

static int obtain_instanceid(struct action_event *event, int *instance)
{
	char *value;
	int rc = 0;
	
	ENTER();

	value = upnp_get_string(event, "InstanceID");
	if (value == NULL) {
		upnp_set_error(event, UPNP_SOAP_E_INVALID_ARGS,
			       "Missing InstanceID");
		return -1;
	}
	//printf("%s: InstanceID='%s'\n", __FUNCTION__, value);
	free(value);

	// TODO - parse value, and store in *instance, if instance!=NULL

	LEAVE();

	return rc;
}

static int replay_with(char *src,char symbol,char *req_symbol){
	
	int i = 0;
	int index = 0;
	int len = strlen(src);
	char *buf = (char *)malloc(len*6);
	if(!buf){
		return -1;
	}
	strcpy(buf,"");
	for(i = 0;i<len;i++){
		if(src[i] == symbol){
			strcat(buf,req_symbol);
		}else{
			index = strlen(buf);
			buf[index] = src[i];
			buf[index+1] = 0;
		}
	}
	strcpy(src,buf);
	free(buf);
	return 0;
}



/* UPnP action handlers */

static int set_avtransport_uri(struct action_event *event)
{
	char *value;
	int rc = 0;
	
	ENTER();
	
	if (obtain_instanceid(event, NULL)) {
		LEAVE();
		return -1;
	}
	value = upnp_get_string(event, "CurrentURI");
	if (value == NULL) {
		LEAVE();
		return -1;
	}

	ithread_mutex_lock(&transport_mutex);

	//printf("%s: CurrentURI='%s'\n", __FUNCTION__, value);

	output_set_uri(value);


	change_varok(event, TRANSPORT_VAR_AV_URI, value);
	free(value);

	value = upnp_get_string(event, "CurrentURIMetaData");
	if (value == NULL) {
		rc = -1;
	} else {
		//printf("%s: CurrentURIMetaData='%s'\n", __FUNCTION__,
//		       value);
		//usleep(100*1000);

		char *nstr = (char *)malloc(strlen(value)*6);
		if(!nstr){
			debug_printf(MSG_ERROR, "++++++++++++++%s alloc memory error++++++++\n",__FUNCTION__);
			free(value);
			return -1;
		}
		strcpy(nstr,value);
		replay_with(nstr,'<',"&lt;");
		replay_with(nstr,'>',"&gt;");
		replay_with(nstr,'\"',"&quot;");
		usleep(200*1000);
		change_varok(event, TRANSPORT_VAR_AV_URI_META, nstr);
		
		free(nstr);
		
		//change_varok(event, TRANSPORT_VAR_AV_URI_META, value);
		free(value);
	}
	usleep(200*1000);
	(void)output_get_volume(); //add zhaofeihua
	usleep(100*1000);
	ithread_mutex_unlock(&transport_mutex);

	LEAVE();
	return rc;
}

static int set_next_avtransport_uri(struct action_event *event)
{
	char *value;

	ENTER();

	if (obtain_instanceid(event, NULL)) {
		LEAVE();
		return -1;
	}
	value = upnp_get_string(event, "NextURI");
	if (value == NULL) {
		LEAVE();
		return -1;
	}
	//printf("%s: NextURI='%s'\n", __FUNCTION__, value);
	free(value);
	value = upnp_get_string(event, "NextURIMetaData");
	if (value == NULL) {
		LEAVE();
		return -1;
	}
	//printf("%s: NextURIMetaData='%s'\n", __FUNCTION__, value);
	free(value);

	LEAVE();
	return 0;
}

static int get_transport_info(struct action_event *event)
{
	int rc;
	ENTER();

	if (obtain_instanceid(event, NULL)) {
		rc = -1;
		goto out;
	}

	rc = upnp_append_variable(event, TRANSPORT_VAR_TRANSPORT_STATE,
				  "CurrentTransportState");
	if (rc)
		goto out;

	rc = upnp_append_variable(event, TRANSPORT_VAR_TRANSPORT_STATUS,
				  "CurrentTransportStatus");
	if (rc)
		goto out;

	rc = upnp_append_variable(event,
				  TRANSPORT_VAR_TRANSPORT_PLAY_SPEED,
				  "CurrentSpeed");
	if (rc)
		goto out;

      out:
	LEAVE();
	return rc;
}

static int get_transport_settings(struct action_event *event)
{
	int rc = 0;
	ENTER();

	if (obtain_instanceid(event, NULL)) {
		rc = -1;
		goto out;
	}

      out:
	LEAVE();
	return rc;
}

#if 0
static void notify_cur_position(struct action_event *event){

	struct service *service = event->service;

	int names[] = {TRANSPORT_VAR_REL_TIME_POS,
		TRANSPORT_VAR_CUR_TRACK_DUR};
	char *values[] = {transport_values[TRANSPORT_VAR_REL_TIME_POS],
		transport_values[TRANSPORT_VAR_CUR_TRACK_DUR],NULL};
	struct player_sta *sta = get_player_sta();
	free(service->variable_values[TRANSPORT_VAR_REL_TIME_POS]);
	free(service->variable_values[TRANSPORT_VAR_CUR_TRACK_DUR]);
	//printf("notify_cur_position sta->real_time=%s sta->dur_time=%s\n",sta->real_time,sta->dur_time);
	service->variable_values[TRANSPORT_VAR_REL_TIME_POS] =strdup(sta->real_time);
	service->variable_values[TRANSPORT_VAR_CUR_TRACK_DUR] =strdup(sta->dur_time);
	//service->variable_values[TRANSPORT_VAR_REL_TIME_POS] =real_time;
	//service->variable_values[TRANSPORT_VAR_CUR_TRACK_DUR] =dur_time;


	
	change_var(event, 2,names, values);
}
#endif

//音频设备被占用
void notify_audio_ocuppied(){
	transport_state = TRANSPORT_STOPPED;
	
	//发送通知
	int names[] = {TRANSPORT_VAR_AV_URI};
	char *values[] = {"", NULL};
	ithread_mutex_lock(&transport_mutex);
	change_var_trans(&g_event.event, 1, names, values);
	ithread_mutex_unlock(&transport_mutex);
	//停止播放器
	set_player_run(false);
}


static int get_position_info(struct action_event *event)
{
	int rc;
	
	
	ENTER();


	//update info

	struct service *service = event->service;

	struct player_sta *sta = get_player_sta();
	ithread_mutex_lock(&transport_mutex);
	free(service->variable_values[TRANSPORT_VAR_REL_TIME_POS]);
	free(service->variable_values[TRANSPORT_VAR_CUR_TRACK_DUR]);
	//printf("get_position_info sta->real_time=%s sta->dur_time=%s\n",sta->real_time,sta->dur_time);
 	service->variable_values[TRANSPORT_VAR_REL_TIME_POS] =strdup(sta->real_time);
	service->variable_values[TRANSPORT_VAR_CUR_TRACK_DUR] =strdup(sta->dur_time);
	if( NULL!=gsuri){  //add by take
		free(service->variable_values[TRANSPORT_VAR_CUR_TRACK_URI]);
		service->variable_values[TRANSPORT_VAR_CUR_TRACK_URI] = strdup(gsuri);	
	}
	ithread_mutex_unlock(&transport_mutex);
	
	if (obtain_instanceid(event, NULL)) {
		rc = -1;
		goto out;
	}

	rc = upnp_append_variable(event, TRANSPORT_VAR_CUR_TRACK, "Track");
	if (rc)
		goto out;

	rc = upnp_append_variable(event, TRANSPORT_VAR_CUR_TRACK_DUR,
				  "TrackDuration");
	if (rc)
		goto out;

	rc = upnp_append_variable(event, TRANSPORT_VAR_CUR_TRACK_META,
				  "TrackMetaData");
	if (rc)
		goto out;

	rc = upnp_append_variable(event, TRANSPORT_VAR_CUR_TRACK_URI,
				  "TrackURI");
	if (rc)
		goto out;

	rc = upnp_append_variable(event, TRANSPORT_VAR_REL_TIME_POS,
				  "RelTime");
	if (rc)
		goto out;

	rc = upnp_append_variable(event, TRANSPORT_VAR_ABS_TIME_POS,
				  "AbsTime");
	if (rc)
		goto out;

	rc = upnp_append_variable(event, TRANSPORT_VAR_REL_CTR_POS,
				  "RelCount");
	if (rc)
		goto out;

	rc = upnp_append_variable(event, TRANSPORT_VAR_ABS_CTR_POS,
				  "AbsCount");
	if (rc)
		goto out;

      out:
	LEAVE();
	return rc;
}

static int get_device_caps(struct action_event *event)
{
	int rc = 0;
	ENTER();

	if (obtain_instanceid(event, NULL)) {
		rc = -1;
		goto out;
	}

      out:
	LEAVE();
	return rc;
}


static int get_cur_transport(struct action_event *event)
{
	int rc = 0;
	ENTER();

	if (obtain_instanceid(event, NULL)) {
		rc = -1;
		goto out;
	}
/*
	char *value;
	char cmd[256];
	value = upnp_get_string(event, "Actions");
	printf("++++++++++cur_transport=%s\n",value);
	free(value);
	return 0;
	*/

      out:
	LEAVE();
	return rc;
}



static int stop(struct action_event *event)
{
	ENTER();

	if (obtain_instanceid(event, NULL)) {
		return -1;
	}

	ithread_mutex_lock(&transport_mutex);
	switch (transport_state) {
	case TRANSPORT_STOPPED:
		break;
	case TRANSPORT_PLAYING:
	case TRANSPORT_TRANSITIONING:
	case TRANSPORT_PAUSED_RECORDING:
	case TRANSPORT_RECORDING:
	case TRANSPORT_PAUSED_PLAYBACK:
		output_stop();
		transport_state = TRANSPORT_STOPPED;
		int names[] = {TRANSPORT_VAR_TRANSPORT_STATE,TRANSPORT_VAR_CUR_TRANSPORT_ACTIONS};
		char *values[] = {"STOPPED","Play",NULL};
		change_var_trans(event, 2, names, values);
		// Set TransportPlaySpeed to '1'
		break;

	case TRANSPORT_NO_MEDIA_PRESENT:
		/* action not allowed in these states - error 701 */
		upnp_set_error(event, UPNP_TRANSPORT_E_TRANSITION_NA,
			       "Transition not allowed");

		break;
	}
	ithread_mutex_unlock(&transport_mutex);

	LEAVE();

	return 0;
}


static int play(struct action_event *event)
{
	int rc = 0;
	struct player_sta *sta;

	ENTER();

	if(g_event.init == false){
		memcpy(&g_event.event,event,sizeof(struct action_event));
		memcpy(&g_event.request,event->request,sizeof(struct Upnp_Action_Request));
		g_event.event.request = &g_event.request;
	}

	if (obtain_instanceid(event, NULL)) {
		LEAVE();
		return -1;
	}

	ithread_mutex_lock(&transport_mutex);
	switch (transport_state) {
	case TRANSPORT_PLAYING:
		// Set TransportPlaySpeed to '1'
		break;
	case TRANSPORT_STOPPED:{
		
		//restart
		if (output_play()) {
			debug_printf(MSG_ERROR, "[err]Playing failed\n");
			upnp_set_error(event, 704, "Playing failed");
			rc = -1;
		} else {
		    sta = get_player_sta();
			transport_state = TRANSPORT_PLAYING;

			/*add by chenjianhua, for the play not the client signed in*/
			int names[] = {TRANSPORT_VAR_TRANSPORT_STATE,
				           TRANSPORT_VAR_CUR_TRANSPORT_ACTIONS};
			char *values[] = {"PLAYING","Stop,Pause,Seek",NULL};
			
			sta->finish = false;
			change_var_trans(&g_event.event, 2, names, values);
			sta->change &= ~TRANSPORT_CHG_PLAY;
		}
		break;
	}
	case TRANSPORT_PAUSED_PLAYBACK:
		//pause
		if (output_pause2play()) {
			upnp_set_error(event, 704, "Playing failed");
			rc = -1;
		} else {
			transport_state = TRANSPORT_PLAYING;
			int names[] = {TRANSPORT_VAR_TRANSPORT_STATE,TRANSPORT_VAR_CUR_TRANSPORT_ACTIONS};
			char *values[] = {"PLAYING","Stop,Pause,Seek",NULL};
			change_var_trans(event, 2, names, values);
			//notify_cur_position(event);	
		}
		// Set TransportPlaySpeed to '1'
		break;
	case TRANSPORT_NO_MEDIA_PRESENT:
	case TRANSPORT_TRANSITIONING:
	case TRANSPORT_PAUSED_RECORDING:
	case TRANSPORT_RECORDING:
		/* action not allowed in these states - error 701 */
		upnp_set_error(event, UPNP_TRANSPORT_E_TRANSITION_NA,
			       "Transition not allowed");
		rc = -1;

		break;
	}
	ithread_mutex_unlock(&transport_mutex);

	LEAVE();

	return rc;
}

static int player_pause(struct action_event *event)
{
	int rc = 0;

	ENTER();	

	if (obtain_instanceid(event, NULL)) {
		LEAVE();
		return -1;
	}

	ithread_mutex_lock(&transport_mutex);
	switch (transport_state) {
	case TRANSPORT_PLAYING:	
	case TRANSPORT_PAUSED_RECORDING:
	case TRANSPORT_PAUSED_PLAYBACK:
		if (output_play2pause()) {
			upnp_set_error(event, 704, "Playing failed");
			rc = -1;
		} else {
			transport_state = TRANSPORT_PAUSED_PLAYBACK;
			int names[] = {TRANSPORT_VAR_TRANSPORT_STATE,TRANSPORT_VAR_CUR_TRANSPORT_ACTIONS};
			char *values[] = {"PAUSED_PLAYBACK","Play,Stop,Seek",NULL};
			change_var_trans(event, 2, names, values);
		}
		// Set TransportPlaySpeed to '1'
		break;

	case TRANSPORT_NO_MEDIA_PRESENT:
	case TRANSPORT_TRANSITIONING:

	case TRANSPORT_STOPPED:	
	case TRANSPORT_RECORDING:
		
		/* action not allowed in these states - error 701 */
		upnp_set_error(event, UPNP_TRANSPORT_E_TRANSITION_NA,
			       "Transition not allowed");
		rc = -1;

		break;
	}
	ithread_mutex_unlock(&transport_mutex);

	LEAVE();

	return rc;
}



static int seek(struct action_event *event)
{
	int rc = 0;
	ENTER();

	if (obtain_instanceid(event, NULL)) {
		rc = -1;
	}
	char *value;
	value = upnp_get_string(event, "Target");
	
	output_seek(value);

	LEAVE();

	return rc;
}

static int next(struct action_event *event)
{
	int rc = 0;

	ENTER();

	if (obtain_instanceid(event, NULL)) {
		rc = -1;
	}

	LEAVE();

	return rc;
}

static int previous(struct action_event *event)
{
	ENTER();

	if (obtain_instanceid(event, NULL)) {
		return -1;
	}

	LEAVE();

	return 0;
}


static struct action transport_actions[] = {
	[TRANSPORT_CMD_GETCURRENTTRANSPORTACTIONS] = {"GetCurrentTransportActions", get_cur_transport},	/* optional */
	[TRANSPORT_CMD_GETDEVICECAPABILITIES] =     {"GetDeviceCapabilities", get_device_caps},
	[TRANSPORT_CMD_GETMEDIAINFO] =              {"GetMediaInfo", get_media_info},
	[TRANSPORT_CMD_SETAVTRANSPORTURI] =         {"SetAVTransportURI", set_avtransport_uri},	/* RC9800i */
	[TRANSPORT_CMD_SETNEXTAVTRANSPORTURI] =     {"SetNextAVTransportURI", set_next_avtransport_uri},
	[TRANSPORT_CMD_GETTRANSPORTINFO] =          {"GetTransportInfo", get_transport_info},
	[TRANSPORT_CMD_GETPOSITIONINFO] =           {"GetPositionInfo", get_position_info},
	[TRANSPORT_CMD_GETTRANSPORTSETTINGS] =      {"GetTransportSettings", get_transport_settings},
	[TRANSPORT_CMD_STOP] =                      {"Stop", stop},
	[TRANSPORT_CMD_PLAY] =                      {"Play", play},
	[TRANSPORT_CMD_PAUSE] =                     {"Pause", player_pause},	/* optional */
	//[TRANSPORT_CMD_RECORD] =                    {"Record", NULL},	/* optional */
	[TRANSPORT_CMD_SEEK] =                      {"Seek", seek},
	[TRANSPORT_CMD_NEXT] =                      {"Next", next},
	[TRANSPORT_CMD_PREVIOUS] =                  {"Previous", previous},
	[TRANSPORT_CMD_SETPLAYMODE] =               {"SetPlayMode", NULL},	/* optional */
	//[TRANSPORT_CMD_SETRECORDQUALITYMODE] =      {"SetRecordQualityMode", NULL},	/* optional */
	[TRANSPORT_CMD_UNKNOWN] =                  {NULL, NULL}
};


struct service transport_service = {
        .service_name =         TRANSPORT_SERVICE,
        .type =                 TRANSPORT_TYPE,
	 	.scpd_url =		TRANSPORT_SCPD_URL,
        .control_url =		TRANSPORT_CONTROL_URL,
        .event_url =		TRANSPORT_EVENT_URL,
        .actions =              transport_actions,
        .action_arguments =     argument_list,
        .variable_names =       transport_variables,
        .variable_values =      transport_values,
        .variable_meta =        transport_var_meta,
        .variable_count =       TRANSPORT_VAR_UNKNOWN,
        .command_count =        TRANSPORT_CMD_UNKNOWN,
        .service_mutex =        &transport_mutex
};

void *poll_transport_sta(){
	struct player_sta *sta;
	
	while(1){
		sleep(1);
		sta = get_player_sta();
		if(sta->change&TRANSPORT_CHG_PLAY){	//开始播放
			int names[] = {TRANSPORT_VAR_TRANSPORT_STATE,TRANSPORT_VAR_CUR_TRANSPORT_ACTIONS};
			char *values[] = {"PLAYING", "Stop,Pause,Seek", NULL};
			ithread_mutex_lock(&transport_mutex);
			change_var_trans(&g_event.event, 2, names, values);
			ithread_mutex_unlock(&transport_mutex);
			sta->change &=~TRANSPORT_CHG_PLAY;
			continue;
		}
		if(sta->change&TRANSPORT_CHG_FINISH){	//开始结束
			transport_state = TRANSPORT_STOPPED;

			
			int names[] = {TRANSPORT_VAR_TRANSPORT_STATE,TRANSPORT_VAR_CUR_TRANSPORT_ACTIONS};
			char *values[] = {"STOPPED","Play,Pause,Seek",NULL};
			ithread_mutex_lock(&transport_mutex);
			change_var_trans(&g_event.event, 2,names,values);
			ithread_mutex_unlock(&transport_mutex);
			/*
			int names[] = {TRANSPORT_VAR_TRANSPORT_STATE};
			char *values[] = {"STOPPED",NULL};
			change_var(&g_event.event, 1,names,values);
			//
			notify_cur_position(&g_event.event);*/
			
			sta->change &=~TRANSPORT_CHG_FINISH;
			continue;
		}		

		if(sta->change&TRANSPORT_CHG_SEEK){	//寻道完成
			int names[] = {TRANSPORT_VAR_TRANSPORT_STATE,TRANSPORT_VAR_CUR_TRANSPORT_ACTIONS};
			printf("SEEK-notify:%s,%d\n", __FUNCTION__, __LINE__);
			char *values[] = {"PLAYING","Pause,Seek,Stop",NULL};
			ithread_mutex_lock(&transport_mutex);
			change_var_trans(&g_event.event, 2,names, values);
			ithread_mutex_unlock(&transport_mutex);
			//notify_cur_position(&g_event.event,sta->real_time,sta->dur_time);
			sta->change &=~TRANSPORT_CHG_SEEK;
			printf("SEEK-notify:%s,%d\n", __FUNCTION__, __LINE__);
			continue;
		}		
	}
	
	return NULL;
}


int transport_init(void)
{
    int err_no = 0;
    pthread_t id = 0; 
	//set_transport_service(&transport_service);
	memset(&g_event,0,sizeof(g_event));
	transport_values[TRANSPORT_VAR_TRANSPORT_STATE]  = strdup("STOPPED");
	transport_values[TRANSPORT_VAR_TRANSPORT_STATUS]  = strdup("OK");
	transport_values[TRANSPORT_VAR_PLAY_MEDIUM]  = strdup("UNKNOWN");
	transport_values[TRANSPORT_VAR_REC_MEDIUM]  = strdup("NOT_IMPLEMENTED");
	transport_values[TRANSPORT_VAR_PLAY_MEDIA]  = strdup("NETWORK,UNKNOWN");
	transport_values[TRANSPORT_VAR_REC_MEDIA]  = strdup("NOT_IMPLEMENTED");
	transport_values[TRANSPORT_VAR_CUR_PLAY_MODE]  = strdup("NORMAL");
	transport_values[TRANSPORT_VAR_TRANSPORT_PLAY_SPEED]  = strdup("1");
	transport_values[TRANSPORT_VAR_REC_MEDIUM_WR_STATUS]  = strdup("NOT_IMPLEMENTED");
	transport_values[TRANSPORT_VAR_CUR_REC_QUAL_MODE]  = strdup("NOT_IMPLEMENTED");
	transport_values[TRANSPORT_VAR_POS_REC_QUAL_MODE]  = strdup("NOT_IMPLEMENTED");
	transport_values[TRANSPORT_VAR_NR_TRACKS]  = strdup("0");
	transport_values[TRANSPORT_VAR_CUR_TRACK]  = strdup("0");
	transport_values[TRANSPORT_VAR_CUR_TRACK_DUR]  = strdup("00:00:00");
	transport_values[TRANSPORT_VAR_CUR_MEDIA_DUR]  = strdup("");
	transport_values[TRANSPORT_VAR_CUR_TRACK_META]  = strdup("");
	transport_values[TRANSPORT_VAR_CUR_TRACK_URI]  = strdup("");
	transport_values[TRANSPORT_VAR_AV_URI]  = strdup("");
	transport_values[TRANSPORT_VAR_AV_URI_META]  = strdup("");
	transport_values[TRANSPORT_VAR_NEXT_AV_URI]  = strdup("");
	transport_values[TRANSPORT_VAR_NEXT_AV_URI_META]  = strdup("");
	transport_values[TRANSPORT_VAR_REL_TIME_POS]  = strdup("00:00:00");
	transport_values[TRANSPORT_VAR_ABS_TIME_POS]  = strdup("NOT_IMPLEMENTED");
	transport_values[TRANSPORT_VAR_REL_CTR_POS]  = strdup("2147483647");
	transport_values[TRANSPORT_VAR_ABS_CTR_POS]  = strdup("2147483647");
	transport_values[TRANSPORT_VAR_LAST_CHANGE]  = strdup("&lt;Event xmlns=&quot;urn:schemas-upnp-org:metadata-1-0/AVT/&quot;/&gt;");
	transport_values[TRANSPORT_VAR_AAT_SEEK_MODE]  = strdup("TRACK_NR");
	transport_values[TRANSPORT_VAR_AAT_SEEK_TARGET]  = strdup("");
	transport_values[TRANSPORT_VAR_AAT_INSTANCE_ID]  = strdup("0");
	transport_values[TRANSPORT_VAR_CUR_TRANSPORT_ACTIONS]  = strdup("Play,Pause,Seek,Stop");

    g_str_transport_buf = (char *)malloc(NOTIFY_BUF_LEN*2);
	if (NULL == g_str_transport_buf)
	{
	    perror("malloc");
	    debug_printf(MSG_ERROR, "\n");
		return -1;
	}

	err_no = pthread_create(&id, NULL, poll_transport_sta, NULL);
	if (err_no != 0)
	{
	    debug_printf(MSG_ERROR, "can't create thread: %s\n", strerror(err_no));
		return err_no;
	}
	
	return err_no;
}


