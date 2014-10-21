/* output_gstreamer.h - Definitions for GStreamer output module
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

#ifndef _OUTPUT_GSTREAMER_H
#define _OUTPUT_GSTREAMER_H
//transport
#define TRANSPORT_CHG_PLAY (0x01<<0)
#define TRANSPORT_CHG_FINISH (0x01<<1)
#define TRANSPORT_CHG_SEEK (0x01<<2)
//control
#define CONTROL_CHG_VOLUME (0x01<<16)

#define bool unsigned char
#define true 1
#define false 0



struct player_sta{
	bool run;
	bool play;			//播放是否开始
	bool finish;			//播放是否停止
	bool seek;			//跳转是否完成
	char  real_time[32];			//播放时间
	char  dur_time[32];			//歌曲时间
	char  volume[32];			//音量
	unsigned int change;	//变化标识	
};


#if 0
int output_gstreamer_add_options(GOptionContext *ctx);
#endif
int output_gstreamer_init(void);
void output_set_uri(const char *uri);
int output_play(void);
int output_stop(void);
int output_pause2play(void);
int output_play2pause(void);
int output_seek(char *target);
int output_loop(void);
void set_player_run(bool run);
struct player_sta *get_player_sta();


extern char *gsuri;


int output_get_volume(void);


#endif /*  _OUTPUT_GSTREAMER_H */
