#include <stdio.h>
#include "debug.h"

void dlog_init(audioplay_cfg_t cfg)
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
