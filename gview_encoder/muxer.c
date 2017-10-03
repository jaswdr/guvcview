/*******************************************************************************#
#           guvcview              http://guvcview.sourceforge.net               #
#                                                                               #
#           Paulo Assis <pj.assis@gmail.com>                                    #
#           Nobuhiro Iwamatsu <iwamatsu@nigauri.org>                            #
#                             Add UYVY color support(Macbook iSight)            #
#           Flemming Frandsen <dren.dk@gmail.com>                               #
#                             Add VU meter OSD                                  #
#                                                                               #
# This program is free software; you can redistribute it and/or modify          #
# it under the terms of the GNU General Public License as published by          #
# the Free Software Foundation; either version 2 of the License, or             #
# (at your option) any later version.                                           #
#                                                                               #
# This program is distributed in the hope that it will be useful,               #
# but WITHOUT ANY WARRANTY; without even the implied warranty of                #
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                 #
# GNU General Public License for more details.                                  #
#                                                                               #
# You should have received a copy of the GNU General Public License             #
# along with this program; if not, write to the Free Software                   #
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA     #
#                                                                               #
********************************************************************************/

/*******************************************************************************#
#                                                                               #
#  Encoder library                                                                #
#                                                                               #
********************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <sys/statfs.h>
/* support for internationalization - i18n */
#include <locale.h>
#include <libintl.h>

#include "../config.h"
#include "gviewencoder.h"
#include "encoder.h"
#include "stream_io.h"
#include "matroska.h"
#include "avi.h"
#include "gview.h"

extern int verbosity;

static mkv_context_t *mkv_ctx = NULL;
static avi_context_t *avi_ctx = NULL;

static stream_io_t *video_stream = NULL;
static stream_io_t *audio_stream = NULL;

/*file mutex*/
static __MUTEX_TYPE mutex = __STATIC_MUTEX_INIT;
#define __PMUTEX &mutex

/*
 * mux a video frame
 * args:
 *   encoder_ctx - pointer to encoder context
 *
 * asserts:
 *   encoder_ctx is not null;
 *
 * returns: error code
 */
int encoder_write_video_data(encoder_context_t *encoder_ctx)
{
	/*assertions*/
	assert(encoder_ctx);

	encoder_video_context_t *enc_video_ctx = encoder_ctx->enc_video_ctx;
	assert(enc_video_ctx);

	if(enc_video_ctx->outbuf_coded_size <= 0)
		return -1;

	enc_video_ctx->framecount++;

	int ret =0;
	int block_align = 1;

	encoder_codec_data_t *video_codec_data = (encoder_codec_data_t *) enc_video_ctx->codec_data;

	if(video_codec_data)
		block_align = video_codec_data->codec_context->block_align;

	__LOCK_MUTEX( __PMUTEX );
	switch (encoder_ctx->muxer_id)
	{
		case ENCODER_MUX_AVI:
			ret = avi_write_packet(
					avi_ctx,
					0,
					enc_video_ctx->outbuf,
					enc_video_ctx->outbuf_coded_size,
					enc_video_ctx->dts,
					block_align,
					enc_video_ctx->flags);
			break;

		case ENCODER_MUX_MKV:
		case ENCODER_MUX_WEBM:
			ret = mkv_write_packet(
					mkv_ctx,
					0,
					enc_video_ctx->outbuf,
					enc_video_ctx->outbuf_coded_size,
					enc_video_ctx->duration,
					enc_video_ctx->pts,
					enc_video_ctx->flags);
			break;

		default:

			break;
	}
	__UNLOCK_MUTEX( __PMUTEX );

	return (ret);
}

/*
 * mux a audio frame
 * args:
 *   encoder_ctx - pointer to encoder context
 *
 * asserts:
 *   encoder_ctx is not null;
 *
 * returns: error code
 */
int encoder_write_audio_data(encoder_context_t *encoder_ctx)
{
	/*assertions*/
	assert(encoder_ctx != NULL);

	encoder_audio_context_t *enc_audio_ctx = encoder_ctx->enc_audio_ctx;

	if(!enc_audio_ctx || encoder_ctx->audio_channels <= 0)
		return -1;

	if(enc_audio_ctx->outbuf_coded_size <= 0)
		return -1;
	
	if(verbosity > 3)
		printf("ENCODER: writing %i bytes of audio data\n", enc_audio_ctx->outbuf_coded_size);

	int ret =0;
	int block_align = 1;

	encoder_codec_data_t *audio_codec_data = (encoder_codec_data_t *) enc_audio_ctx->codec_data;

	if(audio_codec_data)
		block_align = audio_codec_data->codec_context->block_align;

	__LOCK_MUTEX( __PMUTEX );
	switch (encoder_ctx->muxer_id)
	{
		case ENCODER_MUX_AVI:
			ret = avi_write_packet(
					avi_ctx,
					1,
					enc_audio_ctx->outbuf,
					enc_audio_ctx->outbuf_coded_size,
					enc_audio_ctx->dts,
					block_align,
					enc_audio_ctx->flags);
			break;

		case ENCODER_MUX_MKV:
		case ENCODER_MUX_WEBM:
			ret = mkv_write_packet(
					mkv_ctx,
					1,
					enc_audio_ctx->outbuf,
					enc_audio_ctx->outbuf_coded_size,
					enc_audio_ctx->duration,
					enc_audio_ctx->pts,
					enc_audio_ctx->flags);
			break;

		default:

			break;
	}
	__UNLOCK_MUTEX( __PMUTEX );

	return (ret);
}

/*
 * initialization of the file muxer
 * args:
 *   encoder_ctx - pointer to encoder context
 *   filename - video filename
 *
 * asserts:
 *   encoder_ctx is not null
 *   encoder_ctx->enc_video_ctx is not null
 *
 * returns: none
 */
void encoder_muxer_init(encoder_context_t *encoder_ctx, const char *filename)
{
	/*assertions*/
	assert(encoder_ctx != NULL);
	assert(encoder_ctx->enc_video_ctx != NULL);

	encoder_codec_data_t *video_codec_data = (encoder_codec_data_t *) encoder_ctx->enc_video_ctx->codec_data;

	int video_codec_id = AV_CODEC_ID_NONE;

	if(encoder_ctx->video_codec_ind == 0) /*no codec_context*/
	{
		switch(encoder_ctx->input_format)
		{
			case V4L2_PIX_FMT_H264:
				video_codec_id = AV_CODEC_ID_H264;
				break;
		}
	}
	else if(video_codec_data)
	{
		video_codec_id = video_codec_data->codec_context->codec_id;
	}

	if(verbosity > 1)
		printf("ENCODER: initializing muxer(%i)\n", encoder_ctx->muxer_id);

	switch (encoder_ctx->muxer_id)
	{
		case ENCODER_MUX_AVI:
			if(avi_ctx != NULL)
			{
				avi_destroy_context(avi_ctx);
				avi_ctx = NULL;
			}
			avi_ctx = avi_create_context(filename);

			/*add video stream*/
			video_stream = avi_add_video_stream(
				avi_ctx,
				encoder_ctx->video_width,
				encoder_ctx->video_height,
				encoder_ctx->fps_den,
				encoder_ctx->fps_num,
				video_codec_id);

			if(video_codec_id == AV_CODEC_ID_THEORA && video_codec_data)
			{
				video_stream->extra_data = (uint8_t *) video_codec_data->codec_context->extradata;
				video_stream->extra_data_size = video_codec_data->codec_context->extradata_size;
			}

			/*add audio stream*/
			if(encoder_ctx->enc_audio_ctx != NULL &&
				encoder_ctx->audio_channels > 0)
			{
				encoder_codec_data_t *audio_codec_data = (encoder_codec_data_t *) encoder_ctx->enc_audio_ctx->codec_data;
				if(audio_codec_data)
				{
					int acodec_ind = get_audio_codec_list_index(audio_codec_data->codec_context->codec_id);
					/*sample size - only used for PCM*/
					int32_t a_bits = encoder_get_audio_bits(acodec_ind);
					/*bit rate (compressed formats)*/
					int32_t b_rate = encoder_get_audio_bit_rate(acodec_ind);

					audio_stream = avi_add_audio_stream(
						avi_ctx,
						encoder_ctx->audio_channels,
						encoder_ctx->audio_samprate,
						a_bits,
						b_rate,
						audio_codec_data->codec_context->codec_id,
						encoder_ctx->enc_audio_ctx->avi_4cc);

					if(audio_codec_data->codec_context->codec_id == AV_CODEC_ID_VORBIS)
					{
						audio_stream->extra_data = (uint8_t *) audio_codec_data->codec_context->extradata;
						audio_stream->extra_data_size = audio_codec_data->codec_context->extradata_size;
					}
				}
			}

			/* add first riff header */
			avi_add_new_riff(avi_ctx);

			break;

		default:
		case ENCODER_MUX_MKV:
		case ENCODER_MUX_WEBM:
			if(mkv_ctx != NULL)
			{
				mkv_destroy_context(mkv_ctx);
				mkv_ctx = NULL;
			}
			mkv_ctx = mkv_create_context(filename, encoder_ctx->muxer_id);

			/*add video stream*/
			video_stream = mkv_add_video_stream(
				mkv_ctx,
				encoder_ctx->video_width,
				encoder_ctx->video_height,
				encoder_ctx->fps_den,
				encoder_ctx->fps_num,
				video_codec_id);

			video_stream->extra_data_size = encoder_set_video_mkvCodecPriv(encoder_ctx);

			if(video_stream->extra_data_size > 0)
			{
				video_stream->extra_data = (uint8_t *) encoder_get_video_mkvCodecPriv(encoder_ctx->video_codec_ind);
				if(encoder_ctx->input_format == V4L2_PIX_FMT_H264)
					video_stream->h264_process = 1; //we need to process NALU marker
			}

			/*add audio stream*/
			if(encoder_ctx->enc_audio_ctx != NULL &&
				encoder_ctx->audio_channels > 0)
			{
				encoder_codec_data_t *audio_codec_data = (encoder_codec_data_t *) encoder_ctx->enc_audio_ctx->codec_data;
				if(audio_codec_data)
				{
					mkv_ctx->audio_frame_size = audio_codec_data->codec_context->frame_size;

					/*sample size - only used for PCM*/
					int32_t a_bits = encoder_get_audio_bits(encoder_ctx->audio_codec_ind);
					/*bit rate (compressed formats)*/
					int32_t b_rate = encoder_get_audio_bit_rate(encoder_ctx->audio_codec_ind);

					audio_stream = mkv_add_audio_stream(
						mkv_ctx,
						encoder_ctx->audio_channels,
						encoder_ctx->audio_samprate,
						a_bits,
						b_rate,
						audio_codec_data->codec_context->codec_id,
						encoder_ctx->enc_audio_ctx->avi_4cc);

					audio_stream->extra_data_size = encoder_set_audio_mkvCodecPriv(encoder_ctx);

					if(audio_stream->extra_data_size > 0)
						audio_stream->extra_data = encoder_get_audio_mkvCodecPriv(encoder_ctx->audio_codec_ind);
				}
			}

			/* write the file header */
			mkv_write_header(mkv_ctx);

			break;

	}
}

/*
 * close the file muxer
 * args:
 *   encoder_ctx - pointer to encoder context
 *
 * asserts:
 *   none
 *
 * returns: none
 */
void encoder_muxer_close(encoder_context_t *encoder_ctx)
{
	switch (encoder_ctx->muxer_id)
	{
		case ENCODER_MUX_AVI:
			if (avi_ctx)
			{
				/*last frame pts*/
				float tottime = (float) ((int64_t) (encoder_ctx->enc_video_ctx->pts) / 1000000); // convert to miliseconds

				if (verbosity > 0)
					printf("ENCODER: (avi) time = %f\n", tottime);

				if (tottime > 0)
				{
					/*try to find the real frame rate*/
					avi_ctx->fps = (double) (encoder_ctx->enc_video_ctx->framecount * 1000) / tottime;
				}

				if (verbosity > 0)
					printf("ENCODER: (avi) %"PRId64" frames in %f ms [ %f fps]\n",
						encoder_ctx->enc_video_ctx->framecount, tottime, avi_ctx->fps);

				//close sound ??

				avi_close(avi_ctx);

				avi_destroy_context(avi_ctx);
				avi_ctx = NULL;
			}
			break;

		default:
		case ENCODER_MUX_MKV:
		case ENCODER_MUX_WEBM:
			if(mkv_ctx != NULL)
			{
				mkv_close(mkv_ctx);

				mkv_destroy_context(mkv_ctx);
				mkv_ctx = NULL;
			}
			break;
	}
}

/*
 * function to determine if enought free space is available
 * args:
 *   treshold: limit treshold in Kbytes (min. free space)
 *
 * asserts:
 *   none
 *
 * returns: 1 if still enough free space left on disk
 *          0 otherwise
 */
int encoder_disk_supervisor(int treshold, const char *path)
{
    /*
     * treshold:
     * 51200  = 50Mb
     * 102400 = 100Mb
     * 358400 = 300Mb
     * 512000 = 500Mb
     */
    int percent = 0;
    uint64_t free_kbytes=0;
    uint64_t total_kbytes=0;
    struct statfs buf;


    statfs(path, &buf);

    total_kbytes= buf.f_blocks * (buf.f_bsize/1024);
    free_kbytes= buf.f_bavail * (buf.f_bsize/1024);

    if(total_kbytes > 0)
        percent = (int) ((1.0f-((float)free_kbytes/(float)total_kbytes))*100.0f);
    else
    {
        fprintf(stderr, "ENCODER: couldn't get disk stats for %s\n", path);
        return (1); /* don't invalidate video capture*/
    }

    if(verbosity > 0)
        printf("ENCODER: (%s) %lluK bytes free on a total of %lluK (used: %d %%) treshold=%iK\n",
            path, (unsigned long long) free_kbytes,
            (unsigned long long) total_kbytes, percent, treshold);

    if(free_kbytes < treshold)
    {
        fprintf(stderr,"ENCODER: Not enough free disk space (%lluKb) left on disk, need > %ik \n",
            (unsigned long long) free_kbytes, treshold);
        return(0); /* not enough free space left on disk   */
    }

    return (1); /* still have enough free space on disk */
}

