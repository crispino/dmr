#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <libavformat/avformat.h>
#include <libavcodec/audioconvert.h>
#include "codec.h"
#include "debug.h"

#define FFMPEG_DECODE_INTERRUPT_CB_TIME                   15

typedef struct audio_state {
	AVFormatContext *ic;
	AVCodecContext *avctx;
	AVStream *audio_st;
	int audio_stream;
	int interrupt;            /* flag: decode_interrupt_cb */
	time_t now_time;
    int sample_fmt;
    int sample_rate;
    int channel;
	int audio_seconds;
	double audio_clock;
	AVPacket audio_pkt_temp;
	AVPacket audio_pkt;	
    enum SampleFormat audio_src_fmt;
	AVAudioConvert *reformat_ctx;
    DECLARE_ALIGNED(16, uint8_t, audio_buf1)[(AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2];
    DECLARE_ALIGNED(16, uint8_t, audio_buf2)[(AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2];
    DECLARE_ALIGNED(16, uint8_t, audio_buf_mono)[AVCODEC_MAX_AUDIO_FRAME_SIZE * 3];
    uint8_t *audio_buf;
} audio_state_t;

static int show_status = 1;
static audio_state_t *as = NULL;             /* audio state */
static int audio_volume = 256;

static int decode_interrupt_cb(void)
{
	time_t new_time = time(NULL);
	
	if (as && as->interrupt && (new_time - as->now_time) > FFMPEG_DECODE_INTERRUPT_CB_TIME) {
		return 1;	
	}
	return 0;
}

static uint8_t *mono_audio_buffer(uint8_t *buf, int len)
{
	int i, j = 0, half = len / 2;	
	uint8_t *cbuf = as->audio_buf_mono;	
	uint16_t *tbuf = (uint16_t *)as->audio_buf_mono;	
	uint16_t *sbuf = (uint16_t *)buf;
	
	if (!buf) {
		return NULL;
	}
	
	for (i = 0; i < half; i++) {
		tbuf[j++] = sbuf[i];
		tbuf[j++] = sbuf[i];		
	}

	return cbuf;
}

static void preprocess_audio_volume(uint8_t *buf, int len)
{
	int i, v, half = len / 2;	
	int16_t *volp = (int16_t *)buf;
	
	if (!buf) {
		return;
	}
	
	for (i = 0; i < half; i++) {
		v = ((*volp) * audio_volume + 128) >> 8;
		if (v < -32768) {
			v = -32768;
		}
		if (v >  32767) {
			v = 32767;
		}
		*volp++ = v;
	}
}

static int init(void)
{
	as = malloc(sizeof(audio_state_t));
	if (!as) {
		debug_printf(MSG_ERROR, "ffmpeg init fail");	
		return -1;
	}
	memset(as, 0, sizeof(audio_state_t));
	av_register_all();
	return 0;
}

static void deinit(void)
{
	if (as) {
		free(as);
		as = NULL;
	}
}

static void unload(void)
{
	if (as->audio_stream >= 0) {
		avcodec_close(as->avctx);
		as->avctx = NULL;
	}
	if (as->ic) {
		av_close_input_file(as->ic);
		as->ic = NULL;
	}
    url_set_interrupt_cb(NULL);
}

static int load(char *file)
{
	AVCodec *codec;
	int err, i, ret;
	
	if (!file) {
		return -1;
	}

	as->audio_clock = 0;
	as->audio_stream = -1;
	as->ic = NULL;
	
	/* disable interrupt */
	as->interrupt = 0;
	url_set_interrupt_cb(decode_interrupt_cb);
	as->audio_src_fmt= SAMPLE_FMT_S16;

	as->ic = avformat_alloc_context();	
    err = av_open_input_file(&as->ic, file, NULL, 0, NULL);
    if (err < 0) {
        debug_printf(MSG_ERROR, "av_open_input_file: open %s fail", file);
        ret = -1;
        goto fail;
    }
	
    err = av_find_stream_info(as->ic);
    if (err < 0) {
        debug_printf(MSG_ERROR, "%s: could not find codec parameters", file);
        ret = -1;
        goto fail;
    }

    if (show_status) {
        dump_format(as->ic, 0, file, 0);
    }

	as->audio_seconds = as->ic->duration / AV_TIME_BASE;

	for (i = 0; i < as->ic->nb_streams; i++) {		
        AVStream *st= as->ic->streams[i];
        AVCodecContext *avctx = st->codec;
		if (avctx->codec_type == AVMEDIA_TYPE_AUDIO) {
			as->audio_stream = i;
			as->audio_st = st;
			as->avctx = avctx;            
            as->sample_fmt = avctx->sample_fmt;
            as->sample_rate = avctx->sample_rate;
            as->channel = avctx->channels;
			break;
		}
	}

	if (as->audio_stream == -1) {
        debug_printf(MSG_ERROR, "Cannot find a audio stream");
		ret = -1;
		goto fail;
	}
	
	codec = avcodec_find_decoder(as->avctx->codec_id);
	if (!codec || avcodec_open(as->avctx, codec) < 0) {
        debug_printf(MSG_ERROR, "Cannot find/open codec");
		ret = -1;
		goto fail;
	}

	/* enable interrupt */
	as->interrupt = 1;
	return 0;
fail:
	unload();
	return ret;
}

static int decode(void)
{
    AVPacket *pkt_temp = &as->audio_pkt_temp;
	AVPacket *pkt = &as->audio_pkt;
	AVCodecContext *dec = as->avctx;
	int ret;
	int n, len, data_size;
    double pts;
	
	as->now_time = time(NULL);
	ret = av_read_frame(as->ic, pkt);
	if (ret < 0) {		
		debug_printf(MSG_INFO, "av_read_frame ret = %d", ret);
		if (url_feof((as->ic->pb))) {
			debug_printf(MSG_INFO, "av_read_frame read eof");
		} else if (url_ferror(as->ic->pb)) {
			debug_printf(MSG_INFO, "av_read_frame fail");
		}		
		return 0;
	}

	if (pkt->stream_index != as->audio_stream) {
		/* skip the frame */
		debug_printf(MSG_INFO, "stream_index error");
		return 1;
	}

	pkt_temp->data = pkt->data;
	pkt_temp->size = pkt->size;

    /* if update the audio clock with the pts */
    if (pkt->pts != AV_NOPTS_VALUE) {
        as->audio_clock = av_q2d(as->audio_st->time_base)*pkt->pts;
    }

	while (pkt_temp->size > 0) {
		data_size = sizeof(as->audio_buf1);		
		as->now_time = time(NULL);
		len = avcodec_decode_audio3(dec, (int16_t *)as->audio_buf1, &data_size, pkt_temp);
		if (len < 0) {
			/* if error, we skip the frame */
			debug_printf(MSG_INFO, "skip the frame");			
			pkt->size = 0;
			break;
		}

		pkt_temp->data += len;
		pkt_temp->size -= len;
		if (data_size <= 0) {
			continue;
		}

		as->audio_buf = as->audio_buf1;

	    if (dec->sample_fmt != as->audio_src_fmt) {
			if (as->reformat_ctx) {
				av_audio_convert_free(as->reformat_ctx);
			}
            as->reformat_ctx= av_audio_convert_alloc(SAMPLE_FMT_S16, 1,
            									dec->sample_fmt, 1, NULL, 0);
            if (!as->reformat_ctx) {
                debug_printf(MSG_ERROR, "Cannot convert %s sample format to %s sample format",
                avcodec_get_sample_fmt_name(dec->sample_fmt),
                avcodec_get_sample_fmt_name(SAMPLE_FMT_S16));
                break;
            }
			           
			if (as->reformat_ctx) {
				const void *ibuf[6] = {as->audio_buf1};
				void *obuf[6] = {as->audio_buf2};
				int istride[6] = {av_get_bits_per_sample_format(dec->sample_fmt) / 8};
				int ostride[6] = {2};
				int len = data_size / istride[0];
				if (av_audio_convert(as->reformat_ctx, obuf, ostride, ibuf, istride, len) < 0) {
					debug_printf(MSG_ERROR, "av_audio_convert() failed\n");
					break;
				}
				as->audio_buf = as->audio_buf2;
				/* FIXME: existing code assume that data_size equals framesize*channels*2
						  remove this legacy cruft */
				data_size = len * 2;
			}          
		}
		
		/* if no pts, then compute it */
		pts = as->audio_clock;
		n = 2 * dec->channels;
		as->audio_clock += (double)data_size /
			(double)(n * dec->sample_rate);

		/* deal with mono */
		if (as->channel == 1) {
			as->audio_buf = mono_audio_buffer(as->audio_buf, data_size);
			data_size <<= 1;
		}

		/* preprocess audio (volume) */
		if (audio_volume != 256) {
			preprocess_audio_volume(as->audio_buf, data_size);
		}
		
		/* write pcm to DAC chip */
		config.output->play(as->audio_buf, data_size, 1);
	}

end:
	/* free the current packet */
	if (pkt->data) {
		av_free_packet(pkt);
	}
	
	return 1;
}

static int get_sample_fmt_bits(void)
{
   return 0;//avcodec_get_sample_fmt_bits(as->sample_fmt);
}

static int get_sample_rate(void)
{
   return as->sample_rate;
}

static int get_channel(void)
{
    return as->channel;
}

static void set_log_level(int lvl)
{
	av_log_set_level(lvl);
}

static double get_audio_clock(void)
{
	return as->audio_clock;
}

static int get_audio_seconds(void)
{
	return as->audio_seconds;
}

static int seek(int sec)
{
	if (as->ic && sec <= as->audio_seconds) {
		return av_seek_frame(as->ic, -1 , sec * AV_TIME_BASE, AVSEEK_FLAG_ANY);
	}
	return -1;
}

static int volume(int vol)
{
	audio_volume = vol * 256 / 100;	
	return 0;
}

audio_codec_t audio_ffmpeg = {
	.name = "ffmpeg",
	.init = &init,
	.deinit = &deinit,
	.load = &load,
	.unload = &unload,
	.decode = &decode,
	.get_sample_fmt_bits = &get_sample_fmt_bits,
	.get_sample_rate = &get_sample_rate,
	.get_channel = &get_channel,
	.get_audio_seconds = &get_audio_seconds,
	.get_audio_clock = &get_audio_clock,
	.set_log_level = &set_log_level,
	.seek = &seek,
	.volume = &volume,
	.help = NULL
};
