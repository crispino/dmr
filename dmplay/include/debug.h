#ifndef DEBUG_H
#define DEBUG_H

#include "common.h"

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

void dlog_init(audioplay_cfg_t cfg);
void dlog_deinit(void);
void show_dlog_parameters(void);

#endif
