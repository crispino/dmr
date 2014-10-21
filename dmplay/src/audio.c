#include <stdio.h>
#include "debug.h"
#include "audio.h"

#ifdef CONFIG_I2S
extern audio_output_t audio_i2s;
#endif

static audio_output_t *outputs[] = {
#ifdef CONFIG_I2S
	&audio_i2s,
#endif
	NULL
};

audio_output_t *audio_get_output(char *name) 
{
    audio_output_t **out;

    /* default to the first */
    if (!name) {
        return outputs[0];
    }

    for (out = outputs; *out; out++) {
        if (!strcasecmp(name, (*out)->name)) {
            return *out;
		}
    }

    return NULL;
}


void audio_ls_outputs(void) 
{
    audio_output_t **out;

	debug_printf(MSG_INFO, "Available audio outputs:");

    for (out = outputs; *out; out++) {
		debug_printf(MSG_INFO, "    %s%s", (*out)->name, out == outputs ? " (default)" : "");		
    }

    for (out = outputs; *out; out++) {
        debug_printf(MSG_INFO, "");
        debug_printf(MSG_INFO, "Options for output %s:", (*out)->name);
		if ((*out)->help) {
			(*out)->help();
		}
    }
}
