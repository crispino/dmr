#ifndef AUDIO_H
#define AUDIO_H

#include<stdint.h>

typedef struct audio_output {
    void (*help)(void);
    char *name;

    int (*init)(void);
    void (*deinit)(void);

    int (*start)(void);
    void (*play)(uint8_t *buf, int len, int endian);
    void (*stop)(void);

	void (*sample_rate)(int rate);	
	void (*sample_fmt_bits)(int bits);		
	void (*channel)(int ch);
    void (*volume)(double vol);
} audio_output_t;

audio_output_t *audio_get_output(char *name);
void audio_ls_outputs(void);

#endif
