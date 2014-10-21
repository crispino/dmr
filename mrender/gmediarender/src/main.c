/* main.c - Main program routines
 *
 * Copyright (C) 2005-2007   Ivo Clarysse
 *
 * This file is part of GMediaRender.
 *
 * GMediaRender is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GMediaRender is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GMediaRender; if not, write to the Free Software 
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, 
 * MA 02110-1301, USA.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h> 
#include <sys/types.h>
#include <getopt.h>
#include <fcntl.h>
#include <stdarg.h>
#include <errno.h>


//#include <glib.h>

#include <upnp/ithread.h>
#include <upnp/upnp.h>

#include "logging.h"
#include "output_gstreamer.h"
#include "upnp.h"
#include "upnp_device.h"
#include "upnp_renderer.h"
#include "debug.h"
#include "uuid.h"


#define PIPE_WR                                   "/tmp/dlna"
#define PIPE_RD                                   "/tmp/dlna_player"
#define PIPE_SIZE                                 1024


static int show_version = FALSE;
static int show_devicedesc = FALSE;
static int show_connmgr_scpd = FALSE;
static int show_control_scpd = FALSE;
static int show_transport_scpd = FALSE;
static char *ip_address = NULL;
static char *uuid = "GMediaRender-1_0-000-000-002";
//static char *friendly_name = PACKAGE_NAME;
//static char *friendly_name = "dlna";


static int pipe_wr = -1;

static int pipe_init(void)
{
	int err;
	unlink(PIPE_WR);
	unlink(PIPE_RD);

	err = mkfifo(PIPE_WR, O_CREAT |O_EXCL |O_NONBLOCK |O_RDWR|0666);
	if (err) {
		debug_printf(MSG_ERROR, "create pipe %s fail", PIPE_WR); 	
		return err;
	}	

	err = mkfifo(PIPE_RD, O_CREAT |O_EXCL |O_NONBLOCK |O_RDWR|0666);
	if (err) {
		debug_printf(MSG_ERROR, "create pipe %s fail", PIPE_RD); 	
		return err;
	}
	
	return 0;
}


static int pipe_open(void)
{
	pipe_wr = open(PIPE_WR, O_WRONLY);
	if (pipe_wr < 0) {
		debug_printf(MSG_ERROR, "open %s fail: %s", PIPE_WR, strerror(errno));
		return -1;
	}
	
	return 0;
}

static void pipe_deinit(void)
{
	if (pipe_wr > 0) {
		close(pipe_wr);
	}

	unlink(PIPE_WR);
	unlink(PIPE_RD);
}

void pipe_write(const char *fmt, ...)
{
	char buf[PIPE_SIZE] = {0};
	va_list list;
	int ret = -1;

	va_start(list, fmt);
	vsprintf(buf, fmt, list);
	va_end(list);

	ret = write(pipe_wr, buf, strlen(buf));
	while (ret == -1 && errno == EINTR) {
		ret = write(pipe_wr, buf, strlen(buf));
		sleep(1);
	}	
}


#if 0
/* Generic GMediaRender options */
static GOptionEntry option_entries[] = {
	{ "version", 0, 0, G_OPTION_ARG_NONE, &show_version,
	  "Output version information and exit", NULL },
	{ "ip-address", 'I', 0, G_OPTION_ARG_STRING, &ip_address,
	  "IP address on which to listen", NULL },
	{ "uuid", 'u', 0, G_OPTION_ARG_STRING, &uuid,
	  "UUID to advertise", NULL },
	{ "friendly-name", 'f', 0, G_OPTION_ARG_STRING, &friendly_name,
	  "Friendly name to advertise", NULL },
	{ "dump-devicedesc", 0, 0, G_OPTION_ARG_NONE, &show_devicedesc,
	  "Dump device descriptor XML and exit", NULL },
	{ "dump-connmgr-scpd", 0, 0, G_OPTION_ARG_NONE, &show_connmgr_scpd,
	  "Dump Connection Manager service description XML and exit", NULL },
	{ "dump-control-scpd", 0, 0, G_OPTION_ARG_NONE, &show_control_scpd,
	  "Dump Rendering Control service description XML and exit", NULL },
	{ "dump-transport-scpd", 0, 0, G_OPTION_ARG_NONE, &show_transport_scpd,
	  "Dump A/V Transport service description XML and exit", NULL },
	{ NULL }
};
#endif

static void do_show_version(void)
{
	puts( PACKAGE_STRING "\n"
        	"This is free software. "
		"You may redistribute copies of it under the terms of\n"
		"the GNU General Public License "
		"<http://www.gnu.org/licenses/gpl.html>.\n"
		"There is NO WARRANTY, to the extent permitted by law."
	);
}
#if 0
static int process_cmdline(int argc, char **argv)
{
	int result = -1;
	GOptionContext *ctx;
	GError *err = NULL;
	int rc;

	ctx = g_option_context_new("- GMediaRender");
	g_option_context_add_main_entries(ctx, option_entries, NULL);

	rc = output_gstreamer_add_options(ctx);
	if (rc != 0) {
		goto out;
	}

	if (!g_option_context_parse (ctx, &argc, &argv, &err)) {
		g_print ("Failed to initialize: %s\n", err->message);
		g_error_free (err);
		goto out;
	}


	result = 0;

out:
	LEAVE();
	return result;
}
#endif


void close_player(){
	
	debug_printf(MSG_INFO, "exit.......");
	
	pipe_write("Q\n");
	sleep(1);	
	system("pkill dmplay");

	pipe_deinit();
	
	exit(0);
	
	//printf("notify_audio_ocuppied\n");
	//notify_audio_ocuppied();
}
extern void notify_audio_ocuppied();
void player_close(){
	//printf("----------------get comstance sigal------------\n");
	//发广播包，断开连接
	notify_audio_ocuppied();
}

void  sigsegv_handle(int signo)
{
	//psingal(signo, "Catch signal");
	debug_printf(MSG_ERROR, "sigsegv_handle in\n");
	signal(SIGSEGV, sigsegv_handle);
}

void sigchld_handle(int signo)
{
	pid_t   pid; 
	int     stat; 

	while((pid = waitpid(-1, &stat, WNOHANG)) > 0)
	{ 
	   debug_printf(MSG_INFO, "child %d terminated\n", pid); 
	} 
	
    signal(SIGCHLD, sigchld_handle);
}

void  sigbus_handle(int signo)
{
	//psingal(signo, "Catch signal");
	signal(SIGBUS, sigbus_handle);
}

void init_daemon()
{
	pid_t pt = -1;

	pt = fork();
	if(pt == -1)
	{
		debug_printf(MSG_ERROR, "Fork:%s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	else if(pt > 0)
	{
		exit(EXIT_SUCCESS);
	}

	if(setsid() == -1)
	{
		debug_printf(MSG_ERROR, "Setsid:%s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
}

void boot_only()
{		
	FILE * fp = NULL;		
	char buff[4];		
	int rc = -1;			
	
	char *command = "ps | grep gmediarender| awk '{if($5==\"gmediarender\")print $0}'|wc -l";		
	fp = popen(command, "r");		
	if (fp == NULL)		
	{				
		debug_printf(MSG_ERROR, "popen:%s\n", strerror(errno));				
		exit(EXIT_FAILURE);		
	}	
	
	memset(buff, '\0', 4);				
	rc = fread(buff,	4, 1, fp);		
	if (rc < 0)		
	{			
		debug_printf(MSG_ERROR, "fread:%s\n", strerror(errno));			
		pclose(fp);			
		exit(EXIT_FAILURE);		
	}		
	else		
	{			
		if(atoi(buff) > 10)			
		{				
			debug_printf(MSG_ERROR, "Current program gmediarender is running\n");				
			pclose(fp);				
			exit(EXIT_FAILURE);			
		}		
	}		
	pclose(fp);
	
}
char g_aucBuf[128] = {0};

static void usage(char *prog_name)
{
	printf("Usage: %s [options...]\n", prog_name);

	printf("\n");
	printf("Options:\n");
    printf("    -h, --help                 show this help\n");
    printf("    -l, --log=FILE             redirect dmplay's standard output to FILE\n");
    printf("                               If --error is not specified, it also redirects\n");
    printf("                               error output to FILE\n");
	printf("    -v, --level=NUM            set debug level for log\n");
	printf("    -n, --num=NUM              set max file num for log\n");
	printf("    -s, --size=SIZE            set max file size for log\n");

	printf("\n");
}


static int parse_options(int argc, char **argv)
{
	int opt;
	static struct option long_options[] = {
		{"help", 0, NULL, 'h'},
		{"log", 1, NULL, 'l'},
		{"level", 1, NULL, 'v'},
		{"num", 1, NULL, 'n'},
		{"size", 1, NULL, 's'},

		{NULL, 0, NULL, 0}
	};
	const char *short_options = "hv:l:n:s:";

	setenv("POSIXLY_CORRECT", "", 1);
	
	while ((opt = getopt_long(argc, argv, short_options, long_options, NULL)) > 0) {
		switch (opt) {
			case 'h':
				usage(argv[0]);
				exit(1);
			case 'v':
				config.debug_level = atoi(optarg);
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
			
			default:
				break;
		}
	}

	return optind;
}


int main(int argc, char **argv)
{
	boot_only();
	int rc;
	int result = EXIT_FAILURE;
	struct device *upnp_renderer;
	char buf[1024];
	char *friendly_name = NULL;
	FILE *fp = NULL;
	FILE *fpStream = NULL;
	int i = 0;
	char device_uuid[128] = {0};
	
	init_daemon();

	log_config_init();
	rc = parse_options(argc, argv);
	dlog_init(config);
	 
	debug_printf(MSG_INFO, "gmediarender, build : %s %s\n", __DATE__, __TIME__);

	rc = pipe_init();
	if (rc == -1) {
		debug_printf(MSG_ERROR, "pipe_init failed.\n");
		goto out;
	}

	fp = popen("cat /etc/hostname", "r");
	if(fp == NULL)
	{
		debug_printf(MSG_ERROR, "Popen:%s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	    
	friendly_name = fgets(buf, sizeof(buf), fp);
	if(friendly_name == NULL)
	{
		friendly_name = "pisen";
	}
	pclose(fp);
	//signal(SIGINT,close_player); //zhaofeihua
	signal(SIGUSR1,player_close); //zhaofeihua
	
	signal(SIGINT, SIG_IGN);		
	signal(SIGALRM, SIG_IGN);		
	signal(SIGPIPE, SIG_IGN);
	signal(SIGHUP, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGABRT, SIG_IGN);
	//signal(SIGSEGV, sigsegv_handle);
	signal(SIGCHLD, sigchld_handle);
	//signal(SIGBUS, sigbus_handle);
	
	
#if 0
	if (!g_thread_supported()) {
		g_thread_init(NULL);
	}

	rc = process_cmdline(argc, argv);
	if (rc != 0) {
		goto out;
	}
#endif
	if (show_version) {
		do_show_version();
		exit(EXIT_SUCCESS);
	}
	if (show_connmgr_scpd) {
		upnp_renderer_dump_connmgr_scpd();
		exit(EXIT_SUCCESS);
	}
	if (show_control_scpd) {
		upnp_renderer_dump_control_scpd();
		exit(EXIT_SUCCESS);
	}
	if (show_transport_scpd) {
		upnp_renderer_dump_transport_scpd();
		exit(EXIT_SUCCESS);
	}

	device_uuid_create(device_uuid);	
	debug_printf(MSG_INFO, "device_uuid: [%s]\n", device_uuid);
	upnp_renderer = upnp_renderer_new(friendly_name, device_uuid);
	if (upnp_renderer == NULL) {
		goto out;
	}

	if (show_devicedesc) {
		fputs(upnp_get_device_desc(upnp_renderer), stdout);
		exit(EXIT_SUCCESS);
	}

	rc = output_gstreamer_init();
	if (rc != 0) {
		goto out;
	}

	fpStream = popen("ifconfig br0 | awk '/inet/{print $2}'", "r");
 	if (NULL == fpStream)
	{
	    debug_printf(MSG_ERROR, "popen [ifconfig br0] fail\n");
		goto out;
	}
	
	fgets(g_aucBuf, sizeof(g_aucBuf), fpStream);
	pclose(fpStream);
	if (strlen(g_aucBuf) > 10)
	{
	    i = 5;
		while (g_aucBuf[i])
		{
		    if (((g_aucBuf[i] > '9') || (g_aucBuf[i] < '0')) && (g_aucBuf[i] != '.'))
		    {
		        g_aucBuf[i] = 0;
				break;
		    }
			i++;
		}
		ip_address = &g_aucBuf[5];
	}
	
	debug_printf(MSG_INFO, "ip: [%s]\n", ip_address);
	
	rc = pipe_open();
	if (rc == -1) {
		debug_printf(MSG_ERROR, "pipe_open failed.\n");
		goto out;
	}
	
    //sleep(1);
	rc = upnp_device_init(upnp_renderer, ip_address);
	if (rc != 0) {
		goto out;
	}

	debug_printf(MSG_INFO, "Ready for rendering..\n");

	output_loop();

	atexit(close_player);

	result = EXIT_SUCCESS;

out:
	return result;
}
