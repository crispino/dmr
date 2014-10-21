#include <stdio.h>
#include <string.h>
#include "common.h"
#include "rlog.h"

#define DEFAULT_PID_FILE              "/tmp/run/dmplay.pid"
#define DEFAULT_PROGRAM_COUNT_FILE    "/tmp/run/dmplay.count"
#define DEFAULT_LOG_FILE              "/tmp/run/dmplay.log"
#define DEFAULT_DEBUG_LEVEL           MSG_DEBUG
#define DEFAULT_LOG_FILE_NUM          2
#define DEFAULT_LOG_FILE_SIZE         (128 * 1024)
#define DEFAULT_OUTPUT_NAME           "i2s"
#define DEFAULT_CODEC_NAME            "ffmpeg"
#define DEFAULT_CODEC_LEVEL           40

audioplay_cfg_t config;

void default_config_init(void)
{
	memset(&config, 0, sizeof(config));

	config.pid_file = DEFAULT_PID_FILE;
	config.log_file = DEFAULT_LOG_FILE;
	config.debug_level = DEFAULT_DEBUG_LEVEL;
	config.file_num = DEFAULT_LOG_FILE_NUM;
	config.file_size = DEFAULT_LOG_FILE_SIZE;
	config.output_name = DEFAULT_OUTPUT_NAME;		
	config.codec_name = DEFAULT_CODEC_NAME;
	config.codec_debug = DEFAULT_CODEC_LEVEL;
	config.prog_count_file = DEFAULT_PROGRAM_COUNT_FILE;
}

void print_config(void)
{
	debug_printf(MSG_INFO, "Load audio    :     %s", config.input_file);
	debug_printf(MSG_INFO, "Daemon        :     %d", config.daemon);
	debug_printf(MSG_INFO, "Pid File      :     %s", config.pid_file);
	debug_printf(MSG_INFO, "Count File    :     %s", config.prog_count_file);
	debug_printf(MSG_INFO, "output        :     %s", config.output_name);
	debug_printf(MSG_INFO, "codec         :     %s", config.codec_name);
	debug_printf(MSG_INFO, "codec debug   :     %d", config.codec_debug);
	debug_printf(MSG_INFO, "Logs Properies:");
	debug_printf(MSG_INFO, "    name            %s", config.log_file);
	debug_printf(MSG_INFO, "    debug level     %d", config.debug_level);
	debug_printf(MSG_INFO, "    num             %d", config.file_num);
	debug_printf(MSG_INFO, "    size            %d bytes", config.file_size);
}
