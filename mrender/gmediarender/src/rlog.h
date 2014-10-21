#ifndef DLOG_H
#define DLOG_H

enum { 
	MSG_DEBUG,
	MSG_INFO,
	MSG_WARNING,
	MSG_ERROR
};

/* Operation File */
int debug_open_file(const char *path);
void debug_close_file(void);
void debug_printf(int level, const char *fmt, ...);

/* Properties of File */
int set_max_file_size(unsigned int size);
int get_max_file_size(void);
int set_backup_file_num(int num);
int get_backup_file_num(void);
int set_debug_level(int level);
int get_debug_level(void);

#endif /* DLOG_H */
