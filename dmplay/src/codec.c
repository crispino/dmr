#include <stdio.h>

#include "codec.h"
#include "debug.h"

#ifdef CONFIG_FFMPEG
extern audio_codec_t audio_ffmpeg;
#endif

static audio_codec_t *codecs[] = {
#ifdef CONFIG_FFMPEG
	&audio_ffmpeg,
#endif
	NULL
};

audio_codec_t *audio_get_codec(char *name) 
{
    audio_codec_t **codec;

    /* default to the first */
    if (!name) {
        return codecs[0];
    }

    for (codec = codecs; *codec; codec++) {
        if (!strcasecmp(name, (*codec)->name)) {
            return *codec;
		}
    }

    return NULL;
}


void audio_ls_codecs(void)
{
    audio_codec_t **codec;

	debug_printf(MSG_INFO, "Available audio codecs:");

    for (codec = codecs; *codec; codec++) {
		debug_printf(MSG_INFO, "    %s%s", (*codec)->name, codec == codecs ? " (default)" : "");		
    }

    for (codec = codecs; *codec; codec++) {
        debug_printf(MSG_INFO, "");
        debug_printf(MSG_INFO, "Options for codec %s:", (*codec)->name);
		if ((*codec)->help) {
			(*codec)->help();
		}
    }
}

