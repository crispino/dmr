#ifndef CODEC_H
#define CODEC_H

typedef struct audio_codec {	
    void (*help)(void);
    char *name;
    
    int (*init)(void);
    void (*deinit)(void);
	int (*load)(char *file);
	void (*unload)(void);
    int (*decode)(void);

    int (*get_sample_fmt_bits)(void);
    int (*get_sample_rate)(void);
    int (*get_channel)(void); 	
	int (*get_audio_seconds)(void);
	double (*get_audio_clock)(void);
	int (*seek)(int sec);
	int (*volume)(int vol);

	void (*set_log_level)(int lvl);
} audio_codec_t;

audio_codec_t *audio_get_codec(char *name);
void audio_ls_codecs(void);

#endif
