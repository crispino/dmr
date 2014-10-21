#include <stdio.h>
#include <string.h>
#include "debug.h"

#define DEFAULT_LOG_FILE              "/tmp/run/gmediarender.log"
#define DEFAULT_DEBUG_LEVEL           MSG_INFO
#define DEFAULT_LOG_FILE_NUM          2
#define DEFAULT_LOG_FILE_SIZE         (128 * 1024)


log_cfg_t config;

void log_config_init(void)
{
	memset(&config, 0, sizeof(config));

	config.log_file = DEFAULT_LOG_FILE;
	config.debug_level = DEFAULT_DEBUG_LEVEL;
	config.file_num = DEFAULT_LOG_FILE_NUM;
	config.file_size = DEFAULT_LOG_FILE_SIZE;
}

void dlog_init(log_cfg_t cfg)
{
	debug_open_file(cfg.log_file);
	set_debug_level(cfg.debug_level);	
	set_max_file_size(cfg.file_size);
	set_backup_file_num(cfg.file_num);
}

void dlog_deinit(void)
{
	debug_close_file();
}

void show_dlog_parameters(void)
{
	printf("Show dlog configure:\n");
	printf("    debug level     %d\n", get_debug_level());
	printf("    log num         %d\n", get_backup_file_num());
	printf("    log size        %d bytes\n", get_max_file_size());
}

