#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdarg.h>

#include "debug.h"
#include "audio.h"
#include "common.h"

#define PIPE_FILE_WR                                   "/tmp/dlna_player"
#define PIPE_FILE_RD                                   "/tmp/dlna"
#define PIPE_BUF_SIZE                                  1024

#define MODE_STOPPED                                   0
#define MODE_PLAYING                                   1
#define MODE_PAUSED                                    2

#define DEFAULT_VOLUME                                 100

static void usage(char *prog_name)
{
	printf("Usage: %s [options...]\n", prog_name);

	printf("\n");
	printf("Options:\n");
    printf("    -h, --help                 show this help\n");
    printf("    -i, --input=FILE           input audio file\n");
    printf("    -o, --output=FILE          output audio file/device\n");	
    printf("    -d, --daemon               fork (daemonise). The PID of the child process is\n");
    printf("                               written to stdout, unless a pidfile is used.\n");
    printf("    -P, --pidfile=FILE         write daemon's pid to FILE on startup.\n");
    printf("                               Has no effect if -d is not specified\n");
    printf("    -l, --log=FILE             redirect dmplay's standard output to FILE\n");
    printf("                               If --error is not specified, it also redirects\n");
    printf("                               error output to FILE\n");
	printf("    -v, --level=NUM            set debug level for log\n");
	printf("    -n, --num=NUM              set max file num for log\n");
	printf("    -s, --size=SIZE            set max file size for log\n");

	printf("    -c, --codec=NAME           set codec for using\n");
	printf("    -r, --codec_debug=NUM      set codec debug level\n");

	printf("\n");
	audio_ls_outputs();
}

static int parse_options(int argc, char **argv)
{
	int opt;
	static struct option long_options[] = {
		{"help", 0, NULL, 'h'},
		{"daemon", 0, NULL, 'd'},
		{"pidfile", 1, NULL, 'P'},
		
		/* dlog reference */
		{"log", 1, NULL, 'l'},
		{"level", 1, NULL, 'v'},
		{"num", 1, NULL, 'n'},
		{"size", 1, NULL, 's'},
		
		{"output", 1, NULL, 'o'},
		{"codec", 1, NULL, 'c'},
		{"codec_debug", 1, NULL, 'r'},			/* codec debug level */
		{"input", 1, NULL, 'i'},	            /* audio file */
		{NULL, 0, NULL, 0}

	};
	const char *short_options = "hdv:P:l:n:s:i:o:c:r:";

	
    /* prevent unrecognised arguments from being shunted to the audio driver */
	setenv("POSIXLY_CORRECT", "", 1);
	
	while ((opt = getopt_long(argc, argv, short_options, long_options, NULL)) > 0) {
		switch (opt) {
			case 'h':
				usage(argv[0]);
				exit(1);
			case 'd':
				config.daemon = 1;
				break;
			case 'v':
				config.debug_level = atoi(optarg);
				break;
			case 'P':
				config.pid_file = optarg;
				break;
			case 'l':
				config.log_file = optarg;
				break;
			case 'n':
				config.file_num = atoi(optarg);
				break;
			case 's':
				config.file_size = atoi(optarg);
				break;
			case 'i':
				config.input_file = optarg;
				break;
			case 'o':
				config.output_name = optarg;
				break;
			case 'c':
				config.codec_name = optarg;
				break;
			case 'r':
				config.codec_debug = atoi(optarg);
				break;
			default:
				break;
		}
	}

	return optind;
}

static void daemon_exit(void)
{
	config.pid_file && unlink(config.pid_file);
}

static void daemon_init(void)
{
	pid_t pid = fork();
	if (pid < 0) {
		printf("failed to fork!\n");
		exit(1);
	}

	if (pid) {     /* parent */
		exit(0);
	} else {       /* child */
		int fd = -1;
		
		if (setsid() == -1) {
			printf("setsid: %s\n", strerror(errno));
			exit(1);
		}

		fd = open(config.pid_file,  O_WRONLY | O_CREAT, S_IWUSR);
		if (fd < 0) {
			printf("Could not open pidfile\n");
			exit(1);
		}
		
		dprintf(fd, "%d\n", getpid());
		close(fd);		
	} 
}

void play_audio_file(char *file)
{
	int channel, bits, rate;
	int err;

	if (!file) {
		return;
	}	
    
	err = config.codec->load(file);
	if (err) {
		return;		
	}

	err = config.output->start();
	if (err) {
		return;
	}
	
	/* gain audio params */
	channel = config.codec->get_channel();
	/* bits: fixed to 16 */
	bits = 16;    //config.codec->get_sample_fmt_bits();
	rate = config.codec->get_sample_rate();

    debug_printf(MSG_DEBUG, "channel: %d", channel);
    debug_printf(MSG_DEBUG, "bits: %d", bits);
    debug_printf(MSG_DEBUG, "rate: %d", rate);

	/* set i2s params */	
	config.output->channel(channel);
	config.output->sample_fmt_bits(bits);
	config.output->sample_rate(rate);

	/* decode audio */
	while (config.codec->decode());

	config.output->stop();	
	config.codec->unload();	
}

static int pipe_rd = -1;
static int pipe_wr = -1;

static int pipe_init(void)
{
	int err;
	unlink(PIPE_FILE_WR);
	unlink(PIPE_FILE_RD);

	err = mkfifo(PIPE_FILE_WR, O_CREAT | O_EXCL);
	if (err) {
		debug_printf(MSG_ERROR, "create %s pipe fail", PIPE_FILE_WR); 	
		return err;
	}

	err = mkfifo(PIPE_FILE_RD, O_CREAT | O_EXCL);
	if (err) {
		debug_printf(MSG_ERROR, "create %s pipe fail", PIPE_FILE_RD); 	
		return err;
	}
	
	return 0;
}

static int pipe_open(void)
{
	pipe_rd = open(PIPE_FILE_RD, O_RDONLY);
	if (pipe_rd < 0) {
		debug_printf(MSG_ERROR, "open %s fail", PIPE_FILE_RD);
		return -1;
	}
	
	pipe_wr = open(PIPE_FILE_WR, O_WRONLY);
	if (pipe_wr < 0) {
		debug_printf(MSG_ERROR, "open %s fail", PIPE_FILE_WR);
		return -1;
	}
	
	return 0;
}

static void pipe_deinit(void)
{
	if (pipe_wr > 0) {
		close(pipe_wr);
	}
	if (pipe_rd > 0) {
		close(pipe_rd);
	}
	unlink(PIPE_FILE_WR);
	unlink(PIPE_FILE_RD);
}

static void pipe_write_msg(const char *fmt, ...)
{
	char buf[PIPE_BUF_SIZE];
	va_list list;

	va_start(list, fmt);
	vsprintf(buf, fmt, list);
	va_end(list);

	write(pipe_wr, buf, strlen(buf));
}	


static int read_digit_from_file(const char *name, int *num)
{
	FILE *fp = NULL;
	int value;

	if (!name) {
		return -1;
	}
	
	fp = fopen(name, "r");
	if (!fp) {
		return -1;
	}

	if (fscanf(fp, "%d", &value) != 1) {
		fclose(fp);
		return -1;
	}

	fclose(fp);
	*num = value;
	return 0;
}

static int write_digit_to_file(const char *name, int num)
{
	FILE *fp = NULL;

	if (!name) {
		return -1;
	}
	
	fp = fopen(name, "w");
	if (!fp) {
		return -1;
	}

	fprintf(fp, "%d", num);
	fclose(fp);
	return 0;
}

	
static void run_loop(void)
{
	static int mode = MODE_STOPPED;
	struct timeval tv;
	int alive = 1;
	fd_set fds;
	int n;
	int audio_seconds, cur_time, last_time;
	int channel, bits, rate;
	static int vol = DEFAULT_VOLUME;
	int silence = 0;	
	
	if (pipe_open()) {
		return;
	}

	do {		
		FD_ZERO(&fds);
		FD_SET(pipe_rd, &fds);

		if (mode == MODE_PLAYING) {
			/* set nonblock */
			tv.tv_sec = tv.tv_usec = 0;
			n = select(32, &fds, NULL, NULL, &tv);
			if (n == 0) {
				if (!config.codec->decode()) {
					mode = MODE_STOPPED;
					pipe_write_msg("realtime=%d;durtime=%d;\n", audio_seconds, audio_seconds);
					config.output->stop();
					config.codec->unload(); 
					continue;
				}
				
				/* Send progress notice per 1 second */
				cur_time = config.codec->get_audio_clock();
				
				if (cur_time - last_time >= 1) {
					pipe_write_msg("realtime=%d;durtime=%d;\n", cur_time, audio_seconds);
					last_time = cur_time;
				}				
			}
		} else {
			/* wait for command */
			while (1) {				
				n = select(32, &fds, NULL, NULL, NULL);
				if (n > 0) {					
					break;
				}
			}
		}

		if (n < 0 && errno != EINTR) {			
			debug_printf(MSG_ERROR, "Error waiting for command: %s", strerror(errno));
			return;
		}
		
		/* read & process commands */
		if (n > 0 && FD_ISSET(pipe_rd, &fds)) {

			short int len = 1;         /* length of buffer */
			char *cmd, *arg;           /* variables for parsing, */
			char *comstr = NULL;       /* gcc thinks that this could be used uninitialited... */ 
			char buf[PIPE_BUF_SIZE];
			short int counter;
			char *next_comstr = buf;   /* have it initialized for first command */

			/* read as much as possible, maybe multiple commands */
			/* When there is nothing to read (EOF) or even an error, it is the end */
			len = read(pipe_rd, buf, PIPE_BUF_SIZE);
			if (len == 0) {
				//debug_printf(MSG_INFO, "fifo ended: reopening");
				close(pipe_rd);
				pipe_rd = open(PIPE_FILE_RD, O_RDONLY | O_NONBLOCK);
				if (pipe_rd < 0) {
					debug_printf(MSG_ERROR, "open of fifo failed: %s", strerror(errno));
					break;
				}				
				continue;				
			}
			if (len < 0) {
				debug_printf(MSG_ERROR, "command read error: %s", strerror(errno));
				break;
			}

		    //debug_printf(MSG_DEBUG, "read %i bytes of commands: %s", len, buf);
			
			/* one command on a line - separation by \n -> C strings in a row */
			for(counter = 0; counter < len; ++counter) {
				/* line end is command end */
				if ((buf[counter] == '\n') || (buf[counter] == '\r')) {
					buf[counter] = 0; /* now it's a properly ending C string */
					comstr = next_comstr;

					/* skip the additional line ender of \r\n or \n\r */
					if((counter < (len - 1)) && ((buf[counter+1] == '\n') || (buf[counter+1] == '\r'))) {
						buf[++counter] = 0;
					}

					/* next "real" char is first of next command */
					next_comstr = buf + counter+1;

					/* directly process the command now */
					debug_printf(MSG_DEBUG, "interpreting command: %s", comstr);
					if (strlen(comstr) == 0) {
						continue;
					}

					/* STOP */
					if (!strcasecmp(comstr, "S") || !strcasecmp(comstr, "STOP")) {
						if (mode != MODE_STOPPED) {							
							config.output->stop();	
							config.codec->unload();
							mode = MODE_STOPPED;
						}

						continue;
					}

					/* PLAY/PAUSE */
					if (!strcasecmp(comstr, "P")) {						
						if (mode != MODE_STOPPED) {	
							if (mode == MODE_PLAYING) {
								mode = MODE_PAUSED;
							} else {
								mode = MODE_PLAYING;
							}
						} 
						
						continue;
					}	
					
					/* PAUSE */
					if (!strcasecmp(comstr, "PAUSE")) {						
						if (mode != MODE_STOPPED) {						
							mode = MODE_PAUSED;
						} 
						
						continue;
					}

					/* PLAY */
					if (!strcasecmp(comstr, "PLAY")) {						
						if (mode != MODE_STOPPED) {						
							mode = MODE_PLAYING;
						} 
						
						continue;
					}

					/* SILENCE */
					if (!strcasecmp(comstr, "SILENCE")) {						
						if (silence) {					
							config.output->volume(vol);							
							silence = 0;
						} else {
							config.output->volume(0);
							silence = 1;
						}
						continue;
					}
					
					/* QUIT */
					if (!strcasecmp(comstr, "Q") || !strcasecmp(comstr, "QUIT")){
						alive = 0;
						config.output->stop();	
						config.codec->unload();
						continue;
					}

					if (!strcasecmp(comstr, "GETVOL")) {
						pipe_write_msg("volume=%d;\n", vol); 
						continue;
					}

					/* commands with arguments */
					cmd = NULL;
					arg = NULL;
					cmd = strtok(comstr," \t"); /* get the main command */
					arg = strtok(NULL,""); /* get the args */
					if (cmd && strlen(cmd) && arg && strlen(arg)) {
						/* JUMP */
						if (!strcasecmp(cmd, "J") || !strcasecmp(cmd, "JUMP")) {
							char *spos;
							double secs;
							
							spos = arg;							
							if (spos[strlen(spos)-1] == 's' && sscanf(arg, "%lf", &secs) == 1) {
								config.codec->seek((int)secs);
								last_time = (int)secs;
							}
							
							pipe_write_msg("seek=true;\n");
							continue;
						}

						if (!strcasecmp(cmd, "V") || !strcasecmp(cmd, "VOLUME")) {
							vol = atoi(arg);
#ifdef CONFIG_SOFTWARE_VOLUME
							config.codec->volume(vol);
#else
							config.output->volume(vol);
#endif
	
							pipe_write_msg("volume=%d;\n",vol);
							continue;
						}

						/* LOAD - actually play */
						if (!strcasecmp(cmd, "L") || !strcasecmp(cmd, "LOAD")) {
							last_time = 0;
							if (config.codec->load(arg)) {
								continue;
							}							
							config.output->start();	
							audio_seconds = config.codec->get_audio_seconds();
							/* gain audio params */
							channel = config.codec->get_channel();							
							/* bits: fixed to 16 */
							bits = 16; //config.codec->get_sample_fmt_bits();
							rate = config.codec->get_sample_rate();
							debug_printf(MSG_DEBUG, "channel: %d", channel);
							debug_printf(MSG_DEBUG, "bits: %d", bits);
							debug_printf(MSG_DEBUG, "rate: %d", rate);
							/* set i2s params */	
							config.output->channel(channel);
							config.output->sample_fmt_bits(bits);
							config.output->sample_rate(rate);
							mode = MODE_PLAYING;							
							pipe_write_msg("play=true;\n");
							continue; 
						}												
					}					
				}
			}
		}
	} while (alive);	
}

int main(int argc, char **argv)
{
	int audio_arg;
	int ret;
	int count = 0;

	debug_printf(MSG_INFO, "[1] %s start...", argv[0]);
	
	/* set defaults */
	default_config_init();
	/* parse arguments into config */
	audio_arg = parse_options(argc, argv);
	print_config();

	if (config.daemon) {
		daemon_init();
	}
	
	dlog_init(config);	

	config.output = audio_get_output(config.output_name);
	if (!config.output) {
		audio_ls_outputs();
		debug_printf(MSG_ERROR, "Invalid audio output specified!");
		exit(1);
	}

	config.codec = audio_get_codec(config.codec_name);
	if (!config.codec) {
		audio_ls_codecs();
		debug_printf(MSG_ERROR, "Invalid audio codec specified!");
		exit(1);
	}

	if (config.output->init()) {
		goto end1;
	}
	if (config.codec->init()) {
		goto end2;
	}
	
	config.codec->set_log_level(config.codec_debug);

	if (config.input_file) {		
		play_audio_file(config.input_file);
		goto end3;
	}

	read_digit_from_file(config.prog_count_file, &count);
	count++;
	write_digit_to_file(config.prog_count_file, count);
	
	debug_printf(MSG_INFO, "[2] %s run loop...[count=%d]", argv[0], count);
	/* receive and handle msg */
	run_loop();
	
	debug_printf(MSG_INFO, "[3] %s exit...", argv[0]);
	/* resource reclaim */
	pipe_deinit();
	daemon_exit();
end3:
	config.codec->deinit();
end2:	
	config.output->deinit();
end1:

	return 0;
}
