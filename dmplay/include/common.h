#ifndef COMMON_H
#define COMMON_H

#include "audio.h"
#include "codec.h"

typedef struct audioplay_cfg {
	/* Properties of dlog */
	int debug_level;
	int file_num;
	int file_size;
	char *log_file;
	char *prog_count_file;

    char *output_name;
	char *codec_name;
	int codec_debug;
    audio_output_t *output;
	audio_codec_t *codec;
	char *pid_file;
	char *input_file;       /* audio file */
	int daemon;
} audioplay_cfg_t;

void default_config_init(void);
void print_config(void);

extern audioplay_cfg_t config;

#endif
