#include <stdio.h>
#include <sys/time.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "rlog.h"

#define CONFIG_DEBUG_FILE
#define CONFIG_ROTATE_FILE
#define CONFIG_LOCAL_TIME

#ifdef CONFIG_DEBUG_FILE
#define MAX_FILE_PATH_LENGTH           128
static FILE *out_file = NULL;
char file_path[MAX_FILE_PATH_LENGTH];
#ifdef CONFIG_ROTATE_FILE
#define MAX_BACKUP_FILE_NUM            4                          /* default: 4 */
#define MAX_FILE_SIZE                  (1 * 1024 * 1024)          /* default: 1M */
unsigned int file_size = 512 * 1024;                              /* default: 512K */
int backup_file_num = 2;                                          /* default: 2 */
#endif
#endif
int debug_level = MSG_INFO;
int debug_timestamp = 1;

#if defined(CONFIG_DEBUG_FILE) && defined(CONFIG_ROTATE_FILE)
static int file_is_full(void)
{
	struct stat st;
	
	if (stat(file_path, &st) || st.st_size >= file_size) {
		return 1;
	}
	
	return 0;	
}

static int shift_file(const char *base_name, int num)
{
	char log_file[MAX_FILE_PATH_LENGTH];
	char log_file_old[MAX_FILE_PATH_LENGTH];
	char *src = log_file_old;
	char *dst = log_file;
	int i, ret = 0;
	char *tmp = NULL;

	snprintf(log_file, MAX_FILE_PATH_LENGTH - 1, "%s.%d", base_name, num - 1);
	if (remove(log_file)) {
		ret |= (1 << (num - 1));
	}

	for (i = num - 2; i >= 0; i--) {
		snprintf(src, MAX_FILE_PATH_LENGTH - 1, "%s.%d", base_name, i);
		if (rename(src, dst)) {			
			ret |= (1 << i);
		}
		
		tmp = dst;
		dst = src;
		src = tmp;
	}

	rename(base_name, dst);
	return ret;
}

int set_max_file_size(unsigned int size)
{
	return ((size <= MAX_FILE_SIZE) ? (file_size = size) : 0);
}

int get_max_file_size(void)
{
	return file_size;
}

int set_backup_file_num(int num)
{
	return ((num < MAX_BACKUP_FILE_NUM && num >= 0) ? (backup_file_num = num) : 0);
}

int get_backup_file_num(void)
{
	return backup_file_num;
}

#endif

static void debug_print_timestamp(void)
{
	if (!debug_timestamp) {
		return;
	}
	
	struct timeval tv;
#ifdef CONFIG_LOCAL_TIME
	time_t t = time(NULL);
	struct tm *lt = localtime(&t);
#endif /* CONFIG_LOCAL_TIME */

	gettimeofday(&tv, NULL);

#ifdef CONFIG_DEBUG_FILE
	if (out_file) {
#ifdef CONFIG_LOCAL_TIME
		fprintf(out_file, "%04d.%02d.%02d %02d:%02d:%02d.%06u: ", lt->tm_year + 1900, lt->tm_mon + 1,
			lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec, tv.tv_usec);
#else
		fprintf(out_file, "%ld.%06u: ", tv.tv_sec, tv.tv_usec);
#endif
	} else {
#endif /* CONFIG_DEBUG_FILE */
#ifdef CONFIG_LOCAL_TIME
		printf("%04d.%02d.%02d %02d:%02d:%02d.%06u: ", lt->tm_year + 1900, lt->tm_mon + 1,
			lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec, tv.tv_usec);
#else
		printf("%ld.%06u: ", tv.tv_sec, tv.tv_usec);
#endif
#ifdef CONFIG_DEBUG_FILE
	}	
#endif /* CONFIG_DEBUG_FILE */
}

int debug_open_file(const char *path)
{
#ifdef CONFIG_DEBUG_FILE
	if (!path || strlen(path) > MAX_FILE_PATH_LENGTH - 1) {
		return 0;
	}
	strcpy(file_path, path);
	out_file = fopen(file_path, "a");
	if (out_file == NULL) {
		debug_printf(MSG_ERROR, "debug_open_file: Failed to open "
			"output file, using standard output");
		return -1;
	}
#endif /* CONFIG_DEBUG_FILE */
	return 0;
}

void debug_close_file(void)
{
#ifdef CONFIG_DEBUG_FILE
	if (!out_file) {
		return;
	}
	fclose(out_file);
	out_file = NULL;
#endif /* CONFIG_DEBUG_FILE */
}

void debug_printf(int level, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	if (level >= debug_level) {
#if defined(CONFIG_DEBUG_FILE) && defined(CONFIG_ROTATE_FILE)
		if (out_file && file_is_full()) {			
			debug_close_file();
			shift_file(file_path, backup_file_num);
			debug_open_file(file_path);
		}
#endif /* CONFIG_ROTATE_FILE */		
		debug_print_timestamp();
#ifdef CONFIG_DEBUG_FILE
		if (out_file) {
			vfprintf(out_file, fmt, ap);
			fprintf(out_file, "\n");
			fflush(out_file);
		} else {
#endif /* CONFIG_DEBUG_FILE */
			vprintf(fmt, ap);
			printf("\n");
#ifdef CONFIG_DEBUG_FILE
		}
#endif /* CONFIG_DEBUG_FILE */
	}
	va_end(ap);
}

int set_debug_level(int level)
{
	return ((level >= 0) ? (debug_level = level) : 0);
}

int get_debug_level(void)
{
	return debug_level;
}
