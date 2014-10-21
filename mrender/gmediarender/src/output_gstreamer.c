/* output_gstreamer.c - Output module for GStreamer
 *
 * Copyright (C) 2005-2007   Ivo Clarysse
 *
 * Adapted to gstreamer-0.10 2006 David Siorpaes
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#if 0
#include <net/if.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <netdb.h>
#endif


//#include <gst/gst.h>

//#define ENABLE_TRACING

#include "logging.h"
#include "upnp_connmgr.h"
#include "output_gstreamer.h"

#include "debug.h"


extern void pipe_write(const char *fmt, ...);


int player_sta_listen() ;

struct player_sta sta;

void send_player_cmd(char *cmd){
	char s_cmd[1024] = {0};
	
	sprintf(s_cmd, "%s\n", cmd);
	pipe_write(s_cmd);
}

#if 0
static void scan_caps(const GstCaps * caps)
{
	guint i;

	g_return_if_fail(caps != NULL);

	if (gst_caps_is_any(caps)) {
		return;
	}
	if (gst_caps_is_empty(caps)) {
		return;
	}

	for (i = 0; i < gst_caps_get_size(caps); i++) {
		GstStructure *structure = gst_caps_get_structure(caps, i);
		register_mime_type(gst_structure_get_name(structure));
	}

}
#endif
#if 0
static void scan_pad_templates_info(GstElement * element,
				    GstElementFactory * factory)
{
	const GList *pads;
	GstPadTemplate *padtemplate;
	GstPad *pad;
	GstElementClass *class;

	class = GST_ELEMENT_GET_CLASS(element);

	if (!class->numpadtemplates) {
		return;
	}

	pads = class->padtemplates;
	while (pads) {
		padtemplate = (GstPadTemplate *) (pads->data);
		pad = (GstPad *) (pads->data);
		pads = g_list_next(pads);

		if ((padtemplate->direction == GST_PAD_SINK) &&
		    ((padtemplate->presence == GST_PAD_ALWAYS) ||
		     (padtemplate->presence == GST_PAD_SOMETIMES) ||
		     (padtemplate->presence == GST_PAD_REQUEST)) &&
		    (padtemplate->caps)) {
			scan_caps(padtemplate->caps);
		}
	}

}
#endif
#if 0
static void scan_mime_list(void)
{
	GList *plugins;
	GstRegistry *registry = gst_registry_get_default();

	ENTER();

	plugins = gst_default_registry_get_plugin_list();

	while (plugins) {
		GList *features;
		GstPlugin *plugin;

		plugin = (GstPlugin *) (plugins->data);
		plugins = g_list_next(plugins);

		features =
		    gst_registry_get_feature_list_by_plugin(registry,
							    gst_plugin_get_name
							    (plugin));

		while (features) {
			GstPluginFeature *feature;

			feature = GST_PLUGIN_FEATURE(features->data);

			if (GST_IS_ELEMENT_FACTORY(feature)) {
				GstElementFactory *factory;
				GstElement *element;
				factory = GST_ELEMENT_FACTORY(feature);
				element =
				    gst_element_factory_create(factory,
							       NULL);
				if (element) {
					scan_pad_templates_info(element,
								factory);
				}
			}

			features = g_list_next(features);
		}
	}

	LEAVE();
}
#endif
#if 0
static GstElement *play;
#endif
char *gsuri = NULL;

void output_set_uri(const char *uri)
{
	ENTER();
	debug_printf(MSG_INFO, "%s: setting uri to '%s'\n", __FUNCTION__, uri);
	if (gsuri != NULL)
	{
		free(gsuri);
		gsuri = NULL;
	}
	gsuri = strdup(uri);
	
	LEAVE();
}

int output_play(void)
{
#if 0
	int result = -1;
	ENTER();
	if (gst_element_set_state(play, GST_STATE_READY) ==
	    GST_STATE_CHANGE_FAILURE) {
		printf("setting play state failed\n");
                goto out;
	}
	g_object_set(G_OBJECT(play), "uri", gsuri, NULL);
	if (gst_element_set_state(play, GST_STATE_PLAYING) ==
	    GST_STATE_CHANGE_FAILURE) {
		printf("setting play state failed\n");
		goto out;
	} 
	result = 0;
out:
	LEAVE();
	return result;
#else
/*
	char buf[1024];

	ENTER();
	printf("%s: >>> play %s\n", __FUNCTION__, gsuri);

    system("killall mpg123");
    memset(buf, 0x0, sizeof(buf));
    sprintf(buf, "mpg123 %s &", gsuri);
  	system(buf);

	LEAVE();

	return 0;
	*/
	char buf[512];

	ENTER();
	debug_printf(MSG_INFO, "%s: >>> play %s\n", __FUNCTION__, gsuri);
	

	if(sta.run){
		send_player_cmd("STOP");	
	}else{
		set_player_run(true);
	}
	memset(buf, 0x0, sizeof(buf));
	sprintf(buf, "LOAD %s", gsuri);
	debug_printf(MSG_INFO, "send play command to player in(output_play)\n");
	send_player_cmd(buf);	
	strcpy(sta.real_time,"00:00:00");
	
	
	LEAVE();

	return 0;	
#endif
}

int output_stop(void)
{
#if 0
	if (gst_element_set_state(play, GST_STATE_READY) ==
	    GST_STATE_CHANGE_FAILURE) {
		return -1;
	} else {
		return 0;
	}
#endif
/*
    system("killall mpg123");
      return 0;
*/
	debug_printf(MSG_INFO, "send STOP command to player in(output_stop)\n");
	send_player_cmd("STOP");	
	
    return 0;
}

int output_play2pause(void)
{
#if 0
	if (gst_element_set_state(play, GST_STATE_PAUSED) ==
	    GST_STATE_CHANGE_FAILURE) {
		return -1;
	} else {
		return 0;
	}
#endif

	debug_printf(MSG_INFO, "send PAUSE command to player in(output_pause)\n");
	send_player_cmd("PAUSE");	

    return 0;
}

int output_pause2play(void)
{
	debug_printf(MSG_INFO, "send PAUSE command to player in(output_pause)\n");
	send_player_cmd("PLAY");	

    return 0;
}


static int time2sec(char *timestr){
	int h,m,s;
	sscanf(timestr,"%d:%d:%d",&h,&m,&s);
	return h*60*60+m*60+s;
}

int output_seek(char *target)
{
#if 0
	if (gst_element_set_state(play, GST_STATE_PAUSED) ==
	    GST_STATE_CHANGE_FAILURE) {
		return -1;
	} else {
		return 0;
	}
#endif
	
	/*if(time2sec(target) < time2sec(sta.real_time)){		//往回拖，则要重新播放再寻址
		//start 
		output_play();
	}*/
	char cmd[256];
	sprintf(cmd,"JUMP %ds",time2sec(target));
	
	debug_printf(MSG_INFO, "send seek command to player in(output_seek)\n");
	send_player_cmd(cmd);
     
	strcpy(sta.real_time, target); //修改当前播放进度，以防终端的播放器有回跳过程

    return 0;
}



int output_get_volume(void)
{
#if 0
	if (gst_element_set_state(play, GST_STATE_PAUSED) ==
	    GST_STATE_CHANGE_FAILURE) {
		return -1;
	} else {
		return 0;
	}
#endif
	
	debug_printf(MSG_INFO, 
	      "send get_volume command to player in(output_get_volume)\n");
	send_player_cmd("GETVOL");	

    return 0;
}

void set_player_run(bool run){
	
//	if(!sta.run &&run){ //打开播放器
		//output_get_volume(); //add zhaofeihua
//	}
	if(sta.run&&!run){
		//关闭播放器
		send_player_cmd("STOP");
	}
	sta.run = run ;
	

}

int output_loop()
{
#if 0
	GMainLoop *loop;

	/* Create a main loop that runs the default GLib main context */
	loop = g_main_loop_new(NULL, FALSE);

	g_main_loop_run(loop);
#endif
	while(1)
	{
		sleep(1);
		player_sta_listen() ;
	}
	return 0;
}
#if 0
static const char *gststate_get_name(GstState state)
{
	switch(state) {
	case GST_STATE_VOID_PENDING:
		return "VOID_PENDING";
	case GST_STATE_NULL:
		return "NULL";
	case GST_STATE_READY:
		return "READY";
	case GST_STATE_PAUSED:
		return "PAUSED";
	case GST_STATE_PLAYING:
		return "PLAYING";
	default:
		return "Unknown";
	}
}
#endif
#if 0
static gboolean my_bus_callback(GstBus * bus, GstMessage * msg,
				gpointer data)
{
	//GMainLoop *loop = (GMainLoop *) data;
	GstMessageType msgType;
	GstObject *msgSrc;
	gchar *msgSrcName;

	msgType = GST_MESSAGE_TYPE(msg);
	msgSrc = GST_MESSAGE_SRC(msg);
	msgSrcName = GST_OBJECT_NAME(msgSrc);

	switch (msgType) {
	case GST_MESSAGE_EOS:
		g_print("GStreamer: %s: End-of-stream\n", msgSrcName);
		break;
	case GST_MESSAGE_ERROR:{
			gchar *debug;
			GError *err;

			gst_message_parse_error(msg, &err, &debug);
			g_free(debug);

			g_print("GStreamer: %s: Error: %s\n", msgSrcName, err->message);
			g_error_free(err);

			break;
		}
	case GST_MESSAGE_STATE_CHANGED:{
			GstState oldstate, newstate, pending;
			gst_message_parse_state_changed(msg, &oldstate, &newstate, &pending);
			g_print("GStreamer: %s: State change: OLD: '%s', NEW: '%s', PENDING: '%s'\n",
			        msgSrcName,
			        gststate_get_name(oldstate),
			        gststate_get_name(newstate),
			        gststate_get_name(pending));
			break;
		}
	default:
		g_print("GStreamer: %s: unhandled message type %d (%s)\n",
		        msgSrcName, msgType, gst_message_type_get_name(msgType));
		break;
	}

	return TRUE;
}
#endif
#if 0
static gchar *audiosink = NULL;
static gchar *videosink = NULL;

/* Options specific to output_gstreamer */
static GOptionEntry option_entries[] = {
        { "gstout-audiosink", 0, 0, G_OPTION_ARG_STRING, &audiosink,
          "GStreamer audio sink to use "
	  "(autoaudiosink, alsasink, osssink, esdsink, ...)",
	  NULL },
        { "gstout-videosink", 0, 0, G_OPTION_ARG_STRING, &videosink,
          "GStreamer video sink to use "
	  "(autovideosink, xvimagesink, ximagesink, ...)",
	  NULL },
        { NULL }
};
#endif

#if 0
int output_gstreamer_add_options(GOptionContext *ctx)
{
	GOptionGroup *option_group;
	ENTER();
	option_group = g_option_group_new("gstout", "GStreamer Output Options",
	                                  "Show GStreamer Output Options",
	                                  NULL, NULL);
	g_option_group_add_entries(option_group, option_entries);

	g_option_context_add_group (ctx, option_group);
	
	g_option_context_add_group (ctx, gst_init_get_option_group ());
	LEAVE();
	return 0;
}
#endif


#define FIFO_RET "/tmp/dlna_player"  //路径



static void time_format(long secs,char *str){
	sprintf(str,"%02d:%02d:%02d",(int) secs/(60*60),(int)((secs%(60*60))/60),(int)(secs%60));
	
}

/*
int get_result_array(char *result,char *array){
	if(!strchr(result,'\n')){
		
	}
}
*/

bool get_value_by_name(const char *str, const char *name, char *value)
{
	char *temp;
	char find_str[50];
	char *start;
	char *end;
	int size;
	
	if(str == NULL || name == NULL || value == NULL)
	{
		return false;
	}
	strncpy(find_str, name, 50-1);
	strcat(find_str, "=");
	
	temp = strstr(str, find_str);
	if (!temp)
	{
		return false;
	}
	start = temp + strlen(find_str);
	
	end = strchr(temp, ';');
	if (!end)
	{
	    debug_printf(MSG_ERROR, "not find ;\n");
	    return false;
	}
	
	size = end - start;
	if (size >= 255)
	{
	    debug_printf(MSG_ERROR, "value size too large:%d\n", size);
		return false;
	}
	memcpy(value, start, size);
	value[size] = '\0';

	return true;
}


int player_sta_listen() 
{
	int fd_ret = 0;
	char play_info[1024];
	int nread = 0;
	char parse_info[128];
	int nparse_start = 0;
	int nparse_end = 0;
	int ireal_time;
	int idur_time;
	char value1[256];
	char value2[256];

	fd_ret = open(FIFO_RET, O_RDONLY);
	if (fd_ret == -1)
	{
		if (errno == ENXIO)
		{
			debug_printf(MSG_ERROR, "open error; no reading process\n");
		}

		return -1;
	}

	while(1)
	{
		if (nread <= nparse_end)
		{
			memset(play_info, 0, sizeof(play_info));			
			nread = read(fd_ret, play_info, sizeof(play_info));
			if (nread > 0) 
			{
				play_info[nread] = '\0';
			}
			nparse_end = 0;
		}	

		memset(parse_info, 0, sizeof(parse_info));
		nparse_start = nparse_end;
		while (play_info[nparse_end] != '\0' &&
			   play_info[nparse_end] != '\n')
		{
			nparse_end++;
		}
		memcpy(parse_info, play_info+nparse_start, nparse_end-nparse_start);
		parse_info[nparse_end-nparse_start] = '\0';
		nparse_end++;   /* skip '\n' */                                                 

		if (nread > 0)
		{
			debug_printf(MSG_INFO, "parse:%s\n", parse_info);
			
			//播放状态
			if (get_value_by_name(parse_info, "realtime", value1) 
				&& get_value_by_name(parse_info, "durtime", value2))
			{	
				//sscanf(play_info,"%s %d %d",cmd,&ireal_time,&idur_time);
				ireal_time = atoi(value1);
				idur_time = atoi(value2);
				time_format(ireal_time, sta.real_time);
				time_format(idur_time, sta.dur_time);
				
				if ((idur_time - ireal_time) <= 1)
				{
					strncpy(sta.real_time, sta.dur_time, 32);
					sta.finish = true;
				 	sta.change |= TRANSPORT_CHG_FINISH;
					
				 	output_stop();
				}
			}

			// 音量
			if (get_value_by_name(parse_info, "volume", sta.volume))
			{	
				//sscanf(play_info,"%s %s",cmd,sta.volume);
				sta.change |= CONTROL_CHG_VOLUME;
				debug_printf(MSG_INFO, 
					"receive get_volume command response from player\n");
			}

			// 跳转完成
			if (get_value_by_name(parse_info, "seek", value1))
			{
				sta.seek = true;
				sta.change |= TRANSPORT_CHG_SEEK;		
				debug_printf(MSG_INFO, 
					"receive seek command response from player\n");
			}

			// 跳转完成
			if (get_value_by_name(parse_info, "play", value1))
			{
				if (!strcmp(value1, "true"))
				{
					sta.play = true;
					sta.change |= TRANSPORT_CHG_PLAY;	
					debug_printf(MSG_INFO, 
						"receive play command response from player\n");
				}
				else
				{
					debug_printf(MSG_INFO, "play=%s\n", value1);
				}	
			}
		}
		else 
		{
			if ((nread == -1) && (errno == EINTR))
			{
				continue;
			}
			else
			{
				close(fd_ret);
				debug_printf(MSG_ERROR, "read %s error: %s\n", FIFO_RET, strerror(errno));
				return 0;
			}
		}
	}
	
	close(fd_ret);
	
	return 0;
}

struct player_sta *get_player_sta(){
	return &sta;
}

int output_gstreamer_init(void)
{
#if 0
	GstBus *bus;
#endif
	ENTER();
#if 0
	scan_mime_list();

	play = gst_element_factory_make("playbin", "play");

	bus = gst_pipeline_get_bus(GST_PIPELINE(play));
	gst_bus_add_watch(bus, my_bus_callback, NULL);
	gst_object_unref(bus);

	if(audiosink != NULL){
		GstElement *sink = NULL;
		printf("Setting audio sink to %s\n", audiosink);
		sink = gst_element_factory_make (audiosink, "sink");
		g_object_set (G_OBJECT (play), "audio-sink", sink, NULL);
	}
	if(videosink != NULL){
		GstElement *sink = NULL;
		printf("Setting video sink to %s\n", videosink);
		sink = gst_element_factory_make (videosink, "sink");
		g_object_set (G_OBJECT (play), "video-sink", sink, NULL);
	}

	if (gst_element_set_state(play, GST_STATE_READY) ==
	    GST_STATE_CHANGE_FAILURE) {
		fprintf(stderr,
			"Error: pipeline doesn't want to get ready\n");
	}
#endif

	register_mime_type("audio/mpeg");
	register_mime_type("audio/ape");
	register_mime_type("audio/flac");
	register_mime_type("audio/aac");
	register_mime_type("audio/mp4a-latm");
	register_mime_type("audio/wav");
	register_mime_type("audio/x-ms-wma");
	register_mime_type("audio/ogg");

	char buf[128];
	
    memset(&sta, 0x0, sizeof(sta));
   // sprintf(buf, "mpg123 -R --fifo %s&",PLAYER_CMD_PIPE);
   sprintf(buf, "dmplay &");
   system(buf);
   sleep(4);	
/*
	sleep(3);
	pthread_t id = 0; 
	pthread_create(&id,NULL,player_sta_listen,NULL);
	
	*/
	LEAVE();

	return 0;
}



