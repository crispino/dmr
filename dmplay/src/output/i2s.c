#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "debug.h"
#include "audio.h"

#define I2S_DEVICE                            "/dev/i2s"
#define I2S_OPEN_INTERVAL                     (100 * 1000)
#define MAX_I2S_DATA_LEN                      18432

#define I2S_SAMPLE_RATE                       _IOW('N',  0x21, int)
#define I2S_SAMPLE_BITS                       _IOW('N',  0x22, int)
#define I2S_CHANNEL                           _IOW('N',  0x23, int)
#define I2S_VOLUME                            _IOW('N',  0x20, int)


static int fd = -1;
static int is_open = 0;

static int init(void)
{
	//debug_printf(MSG_DEBUG, "i2s audio init");
    return 0;
}

static void deinit(void) 
{
	//debug_printf(MSG_DEBUG, "i2s audio deinit");
}

static int start(void)
{
	if (is_open) {		
		return 0;
	}

	fd = open(I2S_DEVICE, O_WRONLY);
	if (fd < 0) {
		usleep(I2S_OPEN_INTERVAL);
		fd = open(I2S_DEVICE, O_WRONLY);
		if (fd < 0) {			
			debug_printf(MSG_ERROR, "Cannot open %s", I2S_DEVICE);			
			return -1;
		}
	}

	is_open = 1;
	return 0;
}

static void swap_endian(uint16_t *s)
{
	*s = ((*s >> 8) & 0x00FF) | ((*s << 8) & 0xFF00);
}

static void play(uint8_t* buf, int len, int endian) 
{
	uint8_t *tbuf = buf;
	uint16_t *ps = (uint16_t *)buf;
	
	if (!is_open || !buf || len <= 0){
		return;
	}

	/* swap endian */
	if (endian) {
		int i, half = len / 2;
		for (i = 0; i < half; i++) {
			swap_endian(ps++);
		}
	}

	/* Loop write data */
	while (len > MAX_I2S_DATA_LEN) {
		write(fd, tbuf, MAX_I2S_DATA_LEN);
		tbuf += MAX_I2S_DATA_LEN;
		len -= MAX_I2S_DATA_LEN;
	}
	write(fd, tbuf, len);
}

static void stop(void) 
{
	//debug_printf(MSG_DEBUG, "i2s audio stopped");	
	if (is_open) {
		close(fd);
		is_open = 0;
	}
}

static void help(void)
{
	debug_printf(MSG_DEBUG, "There are no options for i2s audio.");
}

static void set_sample_rate(int rate)
{
	if (!is_open || rate < 0) {
		return;
	}
	
	ioctl(fd, I2S_SAMPLE_RATE, rate);
}

static void set_sample_fmt_bits(int bits)
{
	if (!is_open) {
		return;
	}

	if (bits != 8 && bits != 16 && bits != 24 && bits != 32) {
		return;
	}
	
	ioctl(fd, I2S_SAMPLE_BITS, bits);
}

static void set_channel(int ch)
{
	if (!is_open || ch < 0 || ch > 8) {
		return;
	}
	
	ioctl(fd, I2S_CHANNEL, ch);
}

static void set_volume(double vol)
{
	int volume = (int)vol;
	if (!is_open || volume < 0 || volume > 100) {
		return;
	}

	ioctl(fd, I2S_VOLUME, volume);
}

audio_output_t audio_i2s = {
	.name = "i2s",
	.help = &help,
	.init = &init,
	.deinit = &deinit,
	.start = &start,
	.stop = &stop,
	.play = &play,
	.sample_rate = &set_sample_rate,
	.sample_fmt_bits = &set_sample_fmt_bits,
	.channel = &set_channel,
	.volume = &set_volume
};
