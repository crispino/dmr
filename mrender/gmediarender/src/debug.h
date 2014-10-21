#ifndef DEBUG_H
#define DEBUG_H

#define CONFIG_RLOG

typedef struct log_cfg {
	int debug_level;
	int file_num;
	int file_size;
	char *log_file;
} log_cfg_t;


#ifdef CONFIG_RLOG
#include "rlog.h"
#else
#define debug_open_file(path) 
#define debug_close_file() 
#define debug_printf(level, fmt, ...) 
#define set_max_file_size(size) 
#define get_max_file_size() 
#define set_backup_file_num(num) 
#define get_backup_file_num()
#define set_debug_level(level) 
#define get_debug_level()
#endif

void log_config_init(void);
void dlog_init(log_cfg_t cfg);
void dlog_deinit(void);
void show_dlog_parameters(void);

extern log_cfg_t config;

#endif