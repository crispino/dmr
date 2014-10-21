/*
 * Faad decoder
 * Copyright (c) 2003 Zdenek Kabelac
 * Copyright (c) 2004 Thomas Raivio
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * @file
 * AAC decoder.
 *
 * still a bit unfinished - but it plays something
 */

#include "avcodec.h"
//#include "faad.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* MACROS FOR BACKWARDS COMPATIBILITY */
/* structs */
#define faacDecHandle                  NeAACDecHandle
#define faacDecConfiguration           NeAACDecConfiguration
#define faacDecConfigurationPtr        NeAACDecConfigurationPtr
#define faacDecFrameInfo               NeAACDecFrameInfo
/* functions */
#define faacDecGetErrorMessage         NeAACDecGetErrorMessage
#define faacDecSetConfiguration        NeAACDecSetConfiguration
#define faacDecGetCurrentConfiguration NeAACDecGetCurrentConfiguration
#define faacDecInit                    NeAACDecInit
#define faacDecInit2                   NeAACDecInit2
#define faacDecInitDRM                 NeAACDecInitDRM
#define faacDecPostSeekReset           NeAACDecPostSeekReset
#define faacDecOpen                    NeAACDecOpen
#define faacDecClose                   NeAACDecClose
#define faacDecDecode                  NeAACDecDecode
#define AudioSpecificConfig            NeAACDecAudioSpecificConfig

#ifdef _WIN32
  #pragma pack(push, 8)
  #ifndef NEAACDECAPI
    #define NEAACDECAPI __cdecl
  #endif
#else
  #ifndef NEAACDECAPI
    #define NEAACDECAPI
  #endif
#endif

#define FAAD2_VERSION "2.7"

/* object types for AAC */
#define MAIN       1
#define LC         2
#define SSR        3
#define LTP        4
#define HE_AAC     5
#define ER_LC     17
#define ER_LTP    19
#define LD        23
#define DRM_ER_LC 27 /* special object type for DRM */

/* header types */
#define RAW        0
#define ADIF       1
#define ADTS       2
#define LATM       3

/* SBR signalling */
#define NO_SBR           0
#define SBR_UPSAMPLED    1
#define SBR_DOWNSAMPLED  2
#define NO_SBR_UPSAMPLED 3

/* library output formats */
#define FAAD_FMT_16BIT  1
#define FAAD_FMT_24BIT  2
#define FAAD_FMT_32BIT  3
#define FAAD_FMT_FLOAT  4
#define FAAD_FMT_FIXED  FAAD_FMT_FLOAT
#define FAAD_FMT_DOUBLE 5

/* Capabilities */
#define LC_DEC_CAP           (1<<0) /* Can decode LC */
#define MAIN_DEC_CAP         (1<<1) /* Can decode MAIN */
#define LTP_DEC_CAP          (1<<2) /* Can decode LTP */
#define LD_DEC_CAP           (1<<3) /* Can decode LD */
#define ERROR_RESILIENCE_CAP (1<<4) /* Can decode ER */
#define FIXED_POINT_CAP      (1<<5) /* Fixed point */

/* Channel definitions */
#define FRONT_CHANNEL_CENTER (1)
#define FRONT_CHANNEL_LEFT   (2)
#define FRONT_CHANNEL_RIGHT  (3)
#define SIDE_CHANNEL_LEFT    (4)
#define SIDE_CHANNEL_RIGHT   (5)
#define BACK_CHANNEL_LEFT    (6)
#define BACK_CHANNEL_RIGHT   (7)
#define BACK_CHANNEL_CENTER  (8)
#define LFE_CHANNEL          (9)
#define UNKNOWN_CHANNEL      (0)

/* DRM channel definitions */
#define DRMCH_MONO          1
#define DRMCH_STEREO        2
#define DRMCH_SBR_MONO      3
#define DRMCH_SBR_STEREO    4
#define DRMCH_SBR_PS_STEREO 5

/* A decode call can eat up to FAAD_MIN_STREAMSIZE bytes per decoded channel,
   so at least so much bytes per channel should be available in this stream */
#define FAAD_MIN_STREAMSIZE 768 /* 6144 bits/channel */

typedef void *NeAACDecHandle;

typedef struct mp4AudioSpecificConfig
{
    /* Audio Specific Info */
    unsigned char objectTypeIndex;
    unsigned char samplingFrequencyIndex;
    unsigned long samplingFrequency;
    unsigned char channelsConfiguration;

    /* GA Specific Info */
    unsigned char frameLengthFlag;
    unsigned char dependsOnCoreCoder;
    unsigned short coreCoderDelay;
    unsigned char extensionFlag;
    unsigned char aacSectionDataResilienceFlag;
    unsigned char aacScalefactorDataResilienceFlag;
    unsigned char aacSpectralDataResilienceFlag;
    unsigned char epConfig;

    char sbr_present_flag;
    char forceUpSampling;
    char downSampledSBR;
} mp4AudioSpecificConfig;

typedef struct NeAACDecConfiguration
{
    unsigned char defObjectType;
    unsigned long defSampleRate;
    unsigned char outputFormat;
    unsigned char downMatrix;
    unsigned char useOldADTSFormat;
    unsigned char dontUpSampleImplicitSBR;
} NeAACDecConfiguration, *NeAACDecConfigurationPtr;

typedef struct NeAACDecFrameInfo
{
    unsigned long bytesconsumed;
    unsigned long samples;
    unsigned char channels;
    unsigned char error;
    unsigned long samplerate;

    /* SBR: 0: off, 1: on; upsample, 2: on; downsampled, 3: off; upsampled */
    unsigned char sbr;

    /* MPEG-4 ObjectType */
    unsigned char object_type;

    /* AAC header type; MP4 will be signalled as RAW also */
    unsigned char header_type;

    /* multichannel configuration */
    unsigned char num_front_channels;
    unsigned char num_side_channels;
    unsigned char num_back_channels;
    unsigned char num_lfe_channels;
    unsigned char channel_position[64];

    /* PS: 0: off, 1: on */
    unsigned char ps;
} NeAACDecFrameInfo;

char* NEAACDECAPI NeAACDecGetErrorMessage(unsigned char errcode);

unsigned long NEAACDECAPI NeAACDecGetCapabilities(void);

NeAACDecHandle NEAACDECAPI NeAACDecOpen(void);

NeAACDecConfigurationPtr NEAACDECAPI NeAACDecGetCurrentConfiguration(NeAACDecHandle hDecoder);

unsigned char NEAACDECAPI NeAACDecSetConfiguration(NeAACDecHandle hDecoder,
                                                   NeAACDecConfigurationPtr config);

/* Init the library based on info from the AAC file (ADTS/ADIF) */
long NEAACDECAPI NeAACDecInit(NeAACDecHandle hDecoder,
                              unsigned char *buffer,
                              unsigned long buffer_size,
                              unsigned long *samplerate,
                              unsigned char *channels);

/* Init the library using a DecoderSpecificInfo */
char NEAACDECAPI NeAACDecInit2(NeAACDecHandle hDecoder,
                               unsigned char *pBuffer,
                               unsigned long SizeOfDecoderSpecificInfo,
                               unsigned long *samplerate,
                               unsigned char *channels);

/* Init the library for DRM */
char NEAACDECAPI NeAACDecInitDRM(NeAACDecHandle *hDecoder, unsigned long samplerate,
                                 unsigned char channels);

void NEAACDECAPI NeAACDecPostSeekReset(NeAACDecHandle hDecoder, long frame);

void NEAACDECAPI NeAACDecClose(NeAACDecHandle hDecoder);

void* NEAACDECAPI NeAACDecDecode(NeAACDecHandle hDecoder,
                                 NeAACDecFrameInfo *hInfo,
                                 unsigned char *buffer,
                                 unsigned long buffer_size);

void* NEAACDECAPI NeAACDecDecode2(NeAACDecHandle hDecoder,
                                  NeAACDecFrameInfo *hInfo,
                                  unsigned char *buffer,
                                  unsigned long buffer_size,
                                  void **sample_buffer,
                                  unsigned long sample_buffer_size);

char NEAACDECAPI NeAACDecAudioSpecificConfig(unsigned char *pBuffer,
                                             unsigned long buffer_size,
                                             mp4AudioSpecificConfig *mp4ASC);

#ifdef _WIN32
  #pragma pack(pop)
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */



#ifndef FAADAPI
#define FAADAPI
#endif

/*
 * when CONFIG_LIBFAADBIN is true libfaad will be opened at runtime
 */
//#undef CONFIG_LIBFAADBIN
//#define CONFIG_LIBFAADBIN 0
//#define CONFIG_LIBFAADBIN 1

#if CONFIG_LIBFAADBIN
#include <dlfcn.h>
static const char* const libfaadname = "libfaad.so";
#else
#define dlopen(a)
#define dlclose(a)
#endif

typedef struct {
    void* handle;               /* dlopen handle */
    void* faac_handle;          /* FAAD library handle */
    int sample_size;
    int init;

    /* faad calls */
    faacDecHandle FAADAPI (*faacDecOpen)(void);
    faacDecConfigurationPtr FAADAPI (*faacDecGetCurrentConfiguration)(faacDecHandle hDecoder);
#ifndef FAAD2_VERSION
    int FAADAPI (*faacDecSetConfiguration)(faacDecHandle hDecoder,
                                           faacDecConfigurationPtr config);
    int FAADAPI (*faacDecInit)(faacDecHandle hDecoder,
                               unsigned char *buffer,
                               unsigned long *samplerate,
                               unsigned long *channels);
    int FAADAPI (*faacDecInit2)(faacDecHandle hDecoder, unsigned char *pBuffer,
                                unsigned long SizeOfDecoderSpecificInfo,
                                unsigned long *samplerate, unsigned long *channels);
    int FAADAPI (*faacDecDecode)(faacDecHandle hDecoder,
                                 unsigned char *buffer,
                                 unsigned long *bytesconsumed,
                                 short *sample_buffer,
                                 unsigned long *samples);
#else
    unsigned char FAADAPI (*faacDecSetConfiguration)(faacDecHandle hDecoder,
                                                     faacDecConfigurationPtr config);
    long FAADAPI (*faacDecInit)(faacDecHandle hDecoder,
                                unsigned char *buffer,
                                unsigned long buffer_size,
                                unsigned long *samplerate,
                                unsigned char *channels);
    char FAADAPI (*faacDecInit2)(faacDecHandle hDecoder, unsigned char *pBuffer,
                                 unsigned long SizeOfDecoderSpecificInfo,
                                 unsigned long *samplerate, unsigned char *channels);
    void *FAADAPI (*faacDecDecode)(faacDecHandle hDecoder,
                                   faacDecFrameInfo *hInfo,
                                   unsigned char *buffer,
                                   unsigned long buffer_size);
    char* FAADAPI (*faacDecGetErrorMessage)(unsigned char errcode);
#endif

    void FAADAPI (*faacDecClose)(faacDecHandle hDecoder);


} FAACContext;

static const unsigned long faac_srates[] =
{
    96000, 88200, 64000, 48000, 44100, 32000,
    24000, 22050, 16000, 12000, 11025, 8000
};

static void channel_setup(AVCodecContext *avctx)
{
#ifdef FAAD2_VERSION
    FAACContext *s = avctx->priv_data;
    if (avctx->request_channels > 0 && avctx->request_channels == 2 &&
        avctx->request_channels < avctx->channels) {
        faacDecConfigurationPtr faac_cfg;
        avctx->channels = 2;
        faac_cfg = s->faacDecGetCurrentConfiguration(s->faac_handle);
        faac_cfg->downMatrix = 1;
        s->faacDecSetConfiguration(s->faac_handle, faac_cfg);
    }
#endif
}

static av_cold int faac_init_mp4(AVCodecContext *avctx)
{
    FAACContext *s = avctx->priv_data;
    unsigned long samplerate;
#ifndef FAAD2_VERSION
    unsigned long channels;
#else
    unsigned char channels;
#endif
    int r = 0;

    if (avctx->extradata){
        r = s->faacDecInit2(s->faac_handle, (uint8_t*) avctx->extradata,
                            avctx->extradata_size,
                            &samplerate, &channels);
        if (r < 0){
            av_log(avctx, AV_LOG_ERROR,
                   "faacDecInit2 failed r:%d   sr:%ld  ch:%ld  s:%d\n",
                   r, samplerate, (long)channels, avctx->extradata_size);
        } else {
            avctx->sample_rate = samplerate;
            avctx->channels = channels;
            channel_setup(avctx);
            s->init = 1;
        }
    }

    return r;
}

static int faac_decode_frame(AVCodecContext *avctx,
                             void *data, int *data_size,
                             AVPacket *avpkt)
{
    const uint8_t *buf = avpkt->data;
    int buf_size = avpkt->size;
    FAACContext *s = avctx->priv_data;
#ifndef FAAD2_VERSION
    unsigned long bytesconsumed;
    short *sample_buffer = NULL;
    unsigned long samples;
    int out;
#else
    faacDecFrameInfo frame_info;
    void *out;
#endif
    if(buf_size == 0)
        return 0;
#ifndef FAAD2_VERSION
    out = s->faacDecDecode(s->faac_handle,
                           (unsigned char*)buf,
                           &bytesconsumed,
                           data,
                           &samples);
    samples *= s->sample_size;
    if (data_size)
        *data_size = samples;
    return (buf_size < (int)bytesconsumed)
        ? buf_size : (int)bytesconsumed;
#else

    if(!s->init){
        unsigned long srate;
        unsigned char channels;
        int r = s->faacDecInit(s->faac_handle, buf, buf_size, &srate, &channels);
        if(r < 0){
            av_log(avctx, AV_LOG_ERROR, "libfaad: codec init failed.\n");
            return -1;
        }
        avctx->sample_rate = srate;
        avctx->channels = channels;
        channel_setup(avctx);
        s->init = 1;
    }

    out = s->faacDecDecode(s->faac_handle, &frame_info, (unsigned char*)buf, (unsigned long)buf_size);

    if (frame_info.error > 0) {
        av_log(avctx, AV_LOG_ERROR, "libfaad: frame decoding failed: %s\n",
               s->faacDecGetErrorMessage(frame_info.error));
        return -1;
    }
    if (!avctx->frame_size)
        avctx->frame_size = frame_info.samples/avctx->channels;
    frame_info.samples *= s->sample_size;
    memcpy(data, out, frame_info.samples); // CHECKME - can we cheat this one

    if (data_size)
        *data_size = frame_info.samples;

    return (buf_size < (int)frame_info.bytesconsumed)
        ? buf_size : (int)frame_info.bytesconsumed;
#endif
}

static av_cold int faac_decode_end(AVCodecContext *avctx)
{
    FAACContext *s = avctx->priv_data;

    s->faacDecClose(s->faac_handle);

    dlclose(s->handle);
    return 0;
}

static av_cold int faac_decode_init(AVCodecContext *avctx)
{
    FAACContext *s = avctx->priv_data;
    faacDecConfigurationPtr faac_cfg;

#if CONFIG_LIBFAADBIN
    const char* err = 0;

    s->handle = dlopen(libfaadname, RTLD_LAZY);
    if (!s->handle)
    {
        av_log(avctx, AV_LOG_ERROR, "FAAD library: %s could not be opened! \n%s\n",
               libfaadname, dlerror());
        return -1;
    }

#define dfaac(a) do {                                                   \
        const char* n = AV_STRINGIFY(faacDec ## a);                     \
        if (!err && !(s->faacDec ## a = dlsym(s->handle, n))) {         \
            err = n;                                                    \
        }                                                               \
    } while(0)
#else  /* !CONFIG_LIBFAADBIN */
#define dfaac(a)     s->faacDec ## a = faacDec ## a
#endif /* CONFIG_LIBFAADBIN */

    // resolve all needed function calls
    dfaac(Open);
    dfaac(Close);
    dfaac(GetCurrentConfiguration);
    dfaac(SetConfiguration);
    dfaac(Init);
    dfaac(Init2);
    dfaac(Decode);
#ifdef FAAD2_VERSION
    dfaac(GetErrorMessage);
#endif

#undef dfaac

#if CONFIG_LIBFAADBIN
    if (err) {
        dlclose(s->handle);
        av_log(avctx, AV_LOG_ERROR, "FAAD library: cannot resolve %s in %s!\n",
               err, libfaadname);
        return -1;
    }
#endif

    s->faac_handle = s->faacDecOpen();
    if (!s->faac_handle) {
        av_log(avctx, AV_LOG_ERROR, "FAAD library: cannot create handler!\n");
        faac_decode_end(avctx);
        return -1;
    }


    faac_cfg = s->faacDecGetCurrentConfiguration(s->faac_handle);

    if (faac_cfg) {
        switch (avctx->bits_per_coded_sample) {
        case 8: av_log(avctx, AV_LOG_ERROR, "FAADlib unsupported bps %d\n", avctx->bits_per_coded_sample); break;
        default:
        case 16:
#ifdef FAAD2_VERSION
            faac_cfg->outputFormat = FAAD_FMT_16BIT;
#endif
            s->sample_size = 2;
            break;
        case 24:
#ifdef FAAD2_VERSION
            faac_cfg->outputFormat = FAAD_FMT_24BIT;
#endif
            s->sample_size = 3;
            break;
        case 32:
#ifdef FAAD2_VERSION
            faac_cfg->outputFormat = FAAD_FMT_32BIT;
#endif
            s->sample_size = 4;
            break;
        }

        faac_cfg->defSampleRate = (!avctx->sample_rate) ? 44100 : avctx->sample_rate;
        faac_cfg->defObjectType = LC;
    }

    s->faacDecSetConfiguration(s->faac_handle, faac_cfg);

    faac_init_mp4(avctx);

    if(!s->init && avctx->channels > 0)
        channel_setup(avctx);

    avctx->sample_fmt = SAMPLE_FMT_S16;
    return 0;
}

AVCodec ff_aac_fixed_decoder = {
    "aac_fixed",
    AVMEDIA_TYPE_AUDIO,
    CODEC_ID_AAC,
    sizeof(FAACContext),
    faac_decode_init,
    NULL,
    faac_decode_end,
    faac_decode_frame,
    .long_name = NULL_IF_CONFIG_SMALL("Fixed-point decoder for AAC (Advanced Audio Codec)"),
};
