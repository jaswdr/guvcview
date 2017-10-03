/*******************************************************************************#
#           guvcview              http://guvcview.sourceforge.net               #
#                                                                               #
#           Paulo Assis <pj.assis@gmail.com>                                    #
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

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <math.h>
/* support for internationalization - i18n */
#include <locale.h>
#include <libintl.h>

#include "gviewv4l2core.h"
#include "gviewrender.h"
#include "gviewencoder.h"
#include "gview.h"
#include "video_capture.h"
#include "options.h"
#include "config.h"
#include "core_io.h"
#include "gui.h"
#include "../config.h"

/*flags*/
extern int debug_level;

extern __MUTEX_TYPE capture_mutex;
extern __COND_TYPE capture_cond;

static int render = RENDER_SDL; /*render API*/
static int quit = 0; /*terminate flag*/
static int save_image = 0; /*save image flag*/
static int save_video = 0; /*save video flag*/

static uint64_t my_photo_timer = 0; /*timer count*/

static uint64_t my_video_timer = 0; /*timer count*/
static uint64_t my_video_begin_time = 0; /*first video frame ts*/

static int restart = 0; /*restart flag*/

static char render_caption[30]; /*render window caption*/

static uint32_t my_render_mask = REND_FX_YUV_NOFILT; /*render fx filter mask*/

static uint32_t my_audio_mask = AUDIO_FX_NONE; /*audio fx filter mask*/

/*continues focus*/
static int do_soft_autofocus = 0;
/*single time focus (can happen during continues focus)*/
static int do_soft_focus = 0;

/*pointer to audio context data*/
static audio_context_t *my_audio_ctx = NULL;

/*pointer to v4l2 device handler*/
static v4l2_dev_t *my_vd = NULL;

static __THREAD_TYPE encoder_thread;

static int my_encoder_status = 0;

static char status_message[80];

/*
 * set render flag
 * args:
 *    value - flag value
 *
 * asserts:
 *    none
 *
 * returns: none
 */
void set_render_flag(int value)
{
	render = value;
}

/*
 * get render fx mask
 * args:
 *    none
 *
 * asserts:
 *    none
 *
 * returns: render fx mask
 */
uint32_t get_render_fx_mask()
{
	return my_render_mask;
}

/*
 * set render fx mask
 * args:
 *    new_mask - fx mask value
 *
 * asserts:
 *    none
 *
 * returns: none
 */
void set_render_fx_mask(uint32_t new_mask)
{
	my_render_mask = new_mask;
	/* update config */
	config_t *my_config = config_get();
	my_config->video_fx = my_render_mask;
}

/*
 * get audio fx mask
 * args:
 *    none
 *
 * asserts:
 *    none
 *
 * returns: audio fx mask
 */
uint32_t get_audio_fx_mask()
{
	return my_audio_mask;
}

/*
 * set audio fx mask
 * args:
 *    new_mask - new audio fx filter mask
 *
 * asserts:
 *    none
 *
 * returns: none
 */
void set_audio_fx_mask(uint32_t new_mask)
{
	my_audio_mask = new_mask;
	/* update config */
	config_t *my_config = config_get();
	my_config->audio_fx = my_audio_mask;
}

/*
 * set software autofocus flag
 * args:
 *    value - flag value
 *
 * asserts:
 *    none
 *
 * returns: none
 */
void set_soft_autofocus(int value)
{
	do_soft_autofocus = value;
}

/*
 * sets the save video flag
 * args:
 *    value - save_video flag value
 *
 * asserts:
 *    none
 *
 * returns: none
 */
void video_capture_save_video(int value)
{
	save_video = value;
	
	if(debug_level > 1)
		printf("GUVCVIEW: save video flag changed to %i\n", save_video);
}

/*
 * gets the save video flag
 * args:
 *    none
 *
 * asserts:
 *    none
 *
 * returns: save_video flag
 */
int video_capture_get_save_video()
{
	return save_video;
}

/*
 * sets the save image flag
 * args:
 *    none
 *
 * asserts:
 *    none
 *
 * returns: none
 */
void video_capture_save_image()
{
	save_image = 1;
}

/*
 * get encoder started flag
 * args:
 *    none
 *
 * asserts:
 *    none
 *
 * returns: encoder started flag (1 -started; 0 -not started)
 */
int get_encoder_status()
{
	return my_encoder_status;
}

/*
 * stops the photo timed capture
 * args:
 *    none
 *
 * asserts:
 *    none
 *
 * returns: none
 */
void stop_photo_timer()
{
	my_photo_timer = 0;
	gui_set_image_capture_button_label(_("Cap. Image (I)"));
}

/*
 * checks if photo timed capture is on
 * args:
 *    none
 *
 * asserts:
 *    none
 *
 * returns: 1 if on; 0 if off
 */
int check_photo_timer()
{
	return ( (my_photo_timer > 0) ? 1 : 0 );
}

/*
 * reset video timer
 * args:
 *   none
 *
 * asserts:
 *   none
 *
 * returns: none
 */
void reset_video_timer()
{
	my_video_timer = 0;
	my_video_begin_time = 0;
}

/*
 * stops the video timed capture
 * args:
 *    none
 *
 * asserts:
 *    none
 *
 * returns: none
 */
static void stop_video_timer()
{
	/*
	 * if we are saving video stop it
	 * this also calls reset_video_timer
	 */
	if(video_capture_get_save_video())
		gui_click_video_capture_button();

	/*make sure the timer is reset*/
	reset_video_timer();
}

/*
 * checks if video timed capture is on
 * args:
 *    none
 *
 * asserts:
 *    none
 *
 * returns: 1 if on; 0 if off
 */
int check_video_timer()
{
	return ( (my_video_timer > 0) ? 1 : 0 );
}

/*
 * set software focus flag
 * args:
 *    value - flag value
 *
 * asserts:
 *    none
 *
 * returns: none
 */
void set_soft_focus(int value)
{
	v4l2core_soft_autofocus_set_focus();

	do_soft_focus = value;
}
/*
 * request format update
 * args:
 *    none
 *
 * asserts:
 *    none
 *
 * returns: none
 */
void request_format_update()
{
	restart = 1;
}

/*
 * quit callback
 * args:
 *    data - pointer to user data
 *
 * asserts:
 *    none
 *
 * returns: error code
 */
int quit_callback(void *data)
{
	/*make sure we only call gui_close once*/
	if(!quit)
		gui_close();

	quit = 1;

	return 0;
}

/************ RENDER callbacks *******************/
/*
 * key I pressed callback
 * args:
 *    data - pointer to user data
 *
 * asserts:
 *    none
 *
 * returns: error code
 */
int key_I_callback(void *data)
{
	gui_click_image_capture_button();
	
	if(debug_level > 1)
		printf("GUVCVIEW: I key pressed\n");

	return 0;
}

/*
 * key V pressed callback
 * args:
 *    data - pointer to user data
 *
 * asserts:
 *    none
 *
 * returns: error code
 */
int key_V_callback(void *data)
{
	gui_click_video_capture_button(data);
	
	if(debug_level > 1)
		printf("GUVCVIEW: V key pressed\n");

	return 0;
}

/*
 * key DOWN pressed callback
 * args:
 *    data - pointer to user data
 *
 * asserts:
 *    none
 *
 * returns: error code
 */
int key_DOWN_callback(void *data)
{
	if(v4l2core_has_pantilt_id(my_vd))
    {
		int id = V4L2_CID_TILT_RELATIVE;
		int value = v4l2core_get_tilt_step(my_vd);

		v4l2_ctrl_t *control = v4l2core_get_control_by_id(my_vd, id);

		if(control)
		{
			control->value =  value;

			if(v4l2core_set_control_value_by_id(my_vd, id))
				fprintf(stderr, "GUVCVIEW: error setting pan/tilt value\n");

			return 0;
		}
	}

	return -1;
}

/*
 * key UP pressed callback
 * args:
 *    data - pointer to user data
 *
 * asserts:
 *    none
 *
 * returns: error code
 */
int key_UP_callback(void *data)
{
	if(v4l2core_has_pantilt_id(my_vd))
    {
		int id = V4L2_CID_TILT_RELATIVE;
		int value = - v4l2core_get_tilt_step(my_vd);

		v4l2_ctrl_t *control = v4l2core_get_control_by_id(my_vd, id);

		if(control)
		{
			control->value =  value;

			if(v4l2core_set_control_value_by_id(my_vd, id))
				fprintf(stderr, "GUVCVIEW: error setting pan/tilt value\n");

			return 0;
		}
	}

	return -1;
}

/*
 * key LEFT pressed callback
 * args:
 *    data - pointer to user data
 *
 * asserts:
 *    none
 *
 * returns: error code
 */
int key_LEFT_callback(void *data)
{
	if(v4l2core_has_pantilt_id(my_vd))
    {
		int id = V4L2_CID_PAN_RELATIVE;
		int value = v4l2core_get_pan_step(my_vd);

		v4l2_ctrl_t *control = v4l2core_get_control_by_id(my_vd, id);

		if(control)
		{
			control->value =  value;

			if(v4l2core_set_control_value_by_id(my_vd, id))
				fprintf(stderr, "GUVCVIEW: error setting pan/tilt value\n");

			return 0;
		}
	}

	return -1;
}

/*
 * key RIGHT pressed callback
 * args:
 *    data - pointer to user data
 *
 * asserts:
 *    none
 *
 * returns: error code
 */
int key_RIGHT_callback(void *data)
{
	if(v4l2core_has_pantilt_id(my_vd))
    {
		int id = V4L2_CID_PAN_RELATIVE;
		int value = - v4l2core_get_pan_step(my_vd);

		v4l2_ctrl_t *control = v4l2core_get_control_by_id(my_vd, id);

		if(control)
		{
			control->value =  value;

			if(v4l2core_set_control_value_by_id(my_vd, id))
				fprintf(stderr, "GUVCVIEW: error setting pan/tilt value\n");

			return 0;
		}
	}

	return -1;
}

/*
 * create a v4l2 device handler
 * args:
 *    device - device name
 *
 * asserts:
 *    none
 *
 * returns: pointer to v4l2 device handler (or null on error)
 */
v4l2_dev_t *create_v4l2_device_handler(const char *device)
{
	my_vd = v4l2core_init_dev(device);
	
	return my_vd;
}

/*
 * close the v4l2 device handler
 * args:
 *    none
 *
 * asserts:
 *    none
 *
 * returns: none
 */
void close_v4l2_device_handler()
{
	/*closes the video device*/
	v4l2core_close_dev(my_vd);

	my_vd = NULL;
}

/*
 * get the v4l2 device handler
 * args:
 *    none
 *
 * asserts:
 *    none
 *
 * returns: pointer to v4l2 device handler
 */
v4l2_dev_t *get_v4l2_device_handler()
{
	return my_vd;
}

/*
 * create an audio context
 * args:
 *    api - audio api
 *    device - api device index (-1 use default)
 *
 * asserts:
 *    none
 *
 * returns: pointer to audio context data
 */
audio_context_t *create_audio_context(int api, int device)
{
	
	close_audio_context();

	my_audio_ctx = audio_init(api, device);

	if(my_audio_ctx == NULL)
		fprintf(stderr, "GUVCVIEW: couldn't allocate audio context\n");

	return my_audio_ctx;
}

/*
 * get audio context
 * args:
 *    none
 *
 * asserts:
 *    none
 *
 * returns: pointer to audio context data (or NULL if no audio)
 */
audio_context_t *get_audio_context()
{
	if(!my_audio_ctx)
		return NULL;

	/*force a valid number of channels*/
	if(audio_get_channels(my_audio_ctx) > 2)
		audio_set_channels(my_audio_ctx, 2);

	return my_audio_ctx;
}

/*
 * close the audio context
 * args:
 *    none
 *
 * asserts:
 *    none
 *
 * returns: none
 */
void close_audio_context()
{
	if(my_audio_ctx != NULL)
		audio_close(my_audio_ctx);

	my_audio_ctx = NULL;
}

/*
 * audio processing loop (should run in a separate thread)
 * args:
 *    data - pointer to user data
 *
 * asserts:
 *   none
 *
 * returns: pointer to return code
 */
static void *audio_processing_loop(void *data)
{
	encoder_context_t *encoder_ctx = (encoder_context_t *) data;

	if(debug_level > 1)
		printf("GUVCVIEW: audio thread (tid: %u)\n",
			(unsigned int) syscall (SYS_gettid));

	audio_context_t *audio_ctx = get_audio_context();
	if(!audio_ctx)
	{
		fprintf(stderr, "GUVCVIEW: no audio context: skiping audio processing\n");
		return ((void *) -1);
	}
	audio_buff_t *audio_buff = NULL;

	/*start audio capture*/
	int frame_size = encoder_get_audio_frame_size(encoder_ctx);

	//if(frame_size < 1024)
	//	frame_size = 1024;

	audio_set_cap_buffer_size(audio_ctx, 
		frame_size * audio_get_channels(audio_ctx));
	audio_start(audio_ctx);
	/*
	 * alloc the buffer after audio_start
	 * otherwise capture_buff_size may not
	 * be correct
	 * allocated data is big enough for float samples (32 bit)
	 * although it may contain int16 samples (16 bit)
	 */
	audio_buff = audio_get_buffer(audio_ctx);

	int sample_type = encoder_get_audio_sample_fmt(encoder_ctx);
	
	uint32_t osd_mask = render_get_osd_mask();

	/*enable vu meter OSD display*/
	if(audio_get_channels(audio_ctx) > 1)
		osd_mask |= REND_OSD_VUMETER_STEREO;
	else
		osd_mask |= REND_OSD_VUMETER_MONO;

	render_set_osd_mask(osd_mask);

	while(video_capture_get_save_video())
	{
		int ret = audio_get_next_buffer(audio_ctx, audio_buff,
				sample_type, my_audio_mask);

		if(ret > 0)
		{
			/* 
			 * no buffers to process
			 * sleep a couple of milisec
			 */
			 struct timespec req = {
				.tv_sec = 0,
				.tv_nsec = 1000000};/*nanosec*/
			 nanosleep(&req, NULL);
		}
		else if(ret == 0)
		{
			encoder_ctx->enc_audio_ctx->pts = audio_buff->timestamp;

			/*OSD vu meter level*/
			render_set_vu_level(audio_buff->level_meter);

			encoder_process_audio_buffer(encoder_ctx, audio_buff->data);
		}
		
	}

	/*flush any delayed audio frames*/
	encoder_flush_audio_buffer(encoder_ctx);

	/*reset vu meter*/
	audio_buff->level_meter[0] = 0;
	audio_buff->level_meter[1] = 0;
	render_set_vu_level(audio_buff->level_meter);

	/*disable OSD vumeter*/
	osd_mask &= ~REND_OSD_VUMETER_STEREO;
	osd_mask &= ~REND_OSD_VUMETER_MONO;

	render_set_osd_mask(osd_mask);

	audio_stop(audio_ctx);
	audio_delete_buffer(audio_buff);

	return ((void *) 0);
}

/*
 * encoder loop (should run in a separate thread)
 * args:
 *    data - pointer to user data
 *
 * asserts:
 *   none
 *
 * returns: pointer to return code
 */
static void *encoder_loop(void *data)
{
	my_encoder_status = 1;
	
	if(debug_level > 1)
		printf("GUVCVIEW: encoder thread (tid: %u)\n",
			(unsigned int) syscall (SYS_gettid));

	/*get the audio context*/
	audio_context_t *audio_ctx = get_audio_context();

	__THREAD_TYPE encoder_audio_thread;

	int channels = 0;
	int samprate = 0;

	if(audio_ctx)
	{
		channels = audio_get_channels(audio_ctx);
		samprate = audio_get_samprate(audio_ctx);
	}

	if(debug_level > 0)
		printf("GUVCVIEW: audio [channels= %i; samprate= %i] \n",
			channels, samprate);

	/*create the encoder context*/
	encoder_context_t *encoder_ctx = encoder_init(
		v4l2core_get_requested_frame_format(my_vd),
		get_video_codec_ind(),
		get_audio_codec_ind(),
		get_video_muxer(),
		v4l2core_get_frame_width(my_vd),
		v4l2core_get_frame_height(my_vd),
		v4l2core_get_fps_num(my_vd),
		v4l2core_get_fps_denom(my_vd),
		channels,
		samprate);

	/*store external SPS and PPS data if needed*/
	if(encoder_ctx->video_codec_ind == 0 && /*raw - direct input*/
		v4l2core_get_requested_frame_format(my_vd) == V4L2_PIX_FMT_H264)
	{
		/*request a IDR (key) frame*/
		v4l2core_h264_request_idr(my_vd);

		if(debug_level > 0)
			printf("GUVCVIEW: storing external pps and sps data in encoder context\n");
		encoder_ctx->h264_pps_size = v4l2core_get_h264_pps_size(my_vd);
		if(encoder_ctx->h264_pps_size > 0)
		{
			encoder_ctx->h264_pps = calloc(encoder_ctx->h264_pps_size, sizeof(uint8_t));
			if(encoder_ctx->h264_pps == NULL)
			{
				fprintf(stderr,"GUVCVIEW: FATAL memory allocation failure (encoder_loop): %s\n", strerror(errno));
				exit(-1);
			}
			memcpy(encoder_ctx->h264_pps, v4l2core_get_h264_pps(my_vd), encoder_ctx->h264_pps_size);
		}

		encoder_ctx->h264_sps_size = v4l2core_get_h264_sps_size(my_vd);
		if(encoder_ctx->h264_sps_size > 0)
		{
			encoder_ctx->h264_sps = calloc(encoder_ctx->h264_sps_size, sizeof(uint8_t));
			if(encoder_ctx->h264_sps == NULL)
			{
				fprintf(stderr,"GUVCVIEW: FATAL memory allocation failure (encoder_loop): %s\n", strerror(errno));
				exit(-1);
			}
			memcpy(encoder_ctx->h264_sps, v4l2core_get_h264_sps(my_vd), encoder_ctx->h264_sps_size);
		}
	}

	uint32_t current_framerate = 0;
	if(v4l2core_get_requested_frame_format(my_vd) == V4L2_PIX_FMT_H264)
	{
		/* store framerate since it may change due to scheduler*/
		current_framerate = v4l2core_get_h264_frame_rate_config(my_vd);
	}

	char *video_filename = NULL;
	/*get_video_[name|path] always return a non NULL value*/
	char *name = strdup(get_video_name());
	char *path = strdup(get_video_path());

	if(get_video_sufix_flag())
	{
		char *new_name = add_file_suffix(path, name);
		free(name); /*free old name*/
		name = new_name; /*replace with suffixed name*/
	}
	int pathsize = strlen(path);
	if(path[pathsize] != '/')
		video_filename = smart_cat(path, '/', name);
	else
		video_filename = smart_cat(path, 0, name);

	snprintf(status_message, 79, _("saving video to %s"), video_filename);
	gui_status_message(status_message);

	/*muxer initialization*/
	encoder_muxer_init(encoder_ctx, video_filename);

	/*start video capture*/
	video_capture_save_video(1);

	int treshold = 102400; /*100 Mbytes*/
	int64_t last_check_pts = 0; /*last pts when disk supervisor called*/

	/*start audio processing thread*/
	if(encoder_ctx->enc_audio_ctx != NULL && audio_get_channels(audio_ctx) > 0)
	{
		if(debug_level > 1)
			printf("GUVCVIEW: starting encoder audio thread\n");
		
		int ret = __THREAD_CREATE(&encoder_audio_thread, audio_processing_loop, (void *) encoder_ctx);
		
		if(ret)
			fprintf(stderr, "GUVCVIEW: encoder audio thread creation failed (%i)\n", ret);
		else if(debug_level > 2)
			printf("GUVCVIEW: created audio encoder thread with tid: %u\n", 
				(unsigned int) encoder_audio_thread);
	}

	while(video_capture_get_save_video())
	{
		/*process the video buffer*/
		if(encoder_process_next_video_buffer(encoder_ctx) > 0)
		{
			/* 
			 * no buffers to process
			 * sleep a couple of milisec
			 */
			 struct timespec req = {
				.tv_sec = 0,
				.tv_nsec = 1000000};/*nanosec*/
			 nanosleep(&req, NULL);
			 
		}	

		/*disk supervisor*/
		if(encoder_ctx->enc_video_ctx->pts - last_check_pts > 2 * NSEC_PER_SEC)
		{
			last_check_pts = encoder_ctx->enc_video_ctx->pts;

			if(!encoder_disk_supervisor(treshold, path))
			{
				/*stop capture*/
				gui_set_video_capture_button_status(0);
			}
		}
	}
	
	if(debug_level > 1)
		printf("GUVCVIEW: video capture terminated - flushing video buffers\n");
	/*flush the video buffer*/
	encoder_flush_video_buffer(encoder_ctx);
	if(debug_level > 1)
		printf("GUVCVIEW: flushing video buffers - done\n");

	/*make sure the audio processing thread has stopped*/
	if(encoder_ctx->enc_audio_ctx != NULL && audio_get_channels(audio_ctx) > 0)
	{
		if(debug_level > 1)
			printf("GUVCVIEW: join encoder audio thread\n");
		__THREAD_JOIN(encoder_audio_thread);
	}

	/*close the muxer*/
	encoder_muxer_close(encoder_ctx);

	/*close the encoder context (clean up)*/
	encoder_close(encoder_ctx);

	if(v4l2core_get_requested_frame_format(my_vd) == V4L2_PIX_FMT_H264)
	{
		/* restore framerate */
		v4l2core_set_h264_frame_rate_config(my_vd, current_framerate);
	}

	/*clean strings*/
	free(video_filename);
	free(path);
	free(name);

	my_encoder_status = 0;

	return ((void *) 0);
}

/*
 * capture loop (should run in a separate thread)
 * args:
 *    data - pointer to user data (options data)
 *
 * asserts:
 *    none
 *
 * returns: pointer to return code
 */
void *capture_loop(void *data)
{
	__LOCK_MUTEX(&capture_mutex);
	capture_loop_data_t *cl_data = (capture_loop_data_t *) data;
	options_t *my_options = (options_t *) cl_data->options;
	config_t *my_config = (config_t *) cl_data->config;

	uint64_t my_last_photo_time = 0; /*timer count*/
	int my_photo_npics = 0;/*no npics*/

	/*reset quit flag*/
	quit = 0;
	
	if(debug_level > 1)
		printf("GUVCVIEW: capture thread (tid: %u)\n", 
			(unsigned int) syscall (SYS_gettid));

	int ret = 0;
	
	int render_flags = 0;
	
	if (strcasecmp(my_options->render_flag, "full") == 0)
		render_flags = 1;
	else if (strcasecmp(my_options->render_flag, "max") == 0)
		render_flags = 2;
	
	render_set_verbosity(debug_level);

	render_set_crosshair_color(my_config->crosshair_color);
	
	if(render_init(
		render,
		v4l2core_get_frame_width(my_vd),
		v4l2core_get_frame_height(my_vd),
		render_flags) < 0)
		render = RENDER_NONE;
	else
	{
		render_set_event_callback(EV_QUIT, &quit_callback, NULL);
		render_set_event_callback(EV_KEY_V, &key_V_callback, NULL);
		render_set_event_callback(EV_KEY_I, &key_I_callback, NULL);
		render_set_event_callback(EV_KEY_UP, &key_UP_callback, NULL);
		render_set_event_callback(EV_KEY_DOWN, &key_DOWN_callback, NULL);
		render_set_event_callback(EV_KEY_LEFT, &key_LEFT_callback, NULL);
		render_set_event_callback(EV_KEY_RIGHT, &key_RIGHT_callback, NULL);
	}

	/*add a video capture timer*/
	if(my_options->video_timer > 0)
	{
		my_video_timer = NSEC_PER_SEC * my_options->video_timer;
		my_video_begin_time = v4l2core_time_get_timestamp(); /*timer count*/
		/*if are not saving video start it*/
		if(!get_encoder_status())
			start_encoder_thread();
	}

	/*add a photo capture timer*/
	if(my_options->photo_timer > 0)
	{
		my_photo_timer = NSEC_PER_SEC * my_options->photo_timer;
		my_last_photo_time = v4l2core_time_get_timestamp(my_vd); /*timer count*/
	}

	if(my_options->photo_npics > 0)
		my_photo_npics = my_options->photo_npics;

	v4l2core_start_stream(my_vd);

	v4l2_frame_buff_t *frame = NULL; //pointer to frame buffer

	__COND_SIGNAL(&capture_cond);
	__UNLOCK_MUTEX(&capture_mutex);

	while(!quit)
	{
		if(restart)
		{
			int current_width = v4l2core_get_frame_width(my_vd);
			int current_height = v4l2core_get_frame_height(my_vd);

			restart = 0; /*reset*/
			v4l2core_stop_stream(my_vd);

			v4l2core_clean_buffers(my_vd);

			/*try new format (values prepared by the request callback)*/
			ret = v4l2core_update_current_format(my_vd);
			/*try to set the video stream format on the device*/
			if(ret != E_OK)
			{
				fprintf(stderr, "GUCVIEW: could not set the defined stream format\n");
				fprintf(stderr, "GUCVIEW: trying first listed stream format\n");

				v4l2core_prepare_valid_format(my_vd);
				v4l2core_prepare_valid_resolution(my_vd);
				ret = v4l2core_update_current_format(my_vd);

				if(ret != E_OK)
				{
					fprintf(stderr, "GUCVIEW: also could not set the first listed stream format\n");

					gui_error("Guvcview error", "could not start a video stream in the device", 1);

					return ((void *) -1);
				}
			}

			if((current_width != v4l2core_get_frame_width(my_vd)) ||
				current_height != v4l2core_get_frame_height(my_vd))
			{
				if(debug_level > 1)
					printf("GUVCVIEW: resolution changed, reseting render\n");

				/*close render*/
				render_close();

				/*restart the render with new format*/
				if(render_init(
					render,
					v4l2core_get_frame_width(my_vd),
					v4l2core_get_frame_height(my_vd),
					render_flags) < 0)
					render = RENDER_NONE;
				else
				{
					render_set_event_callback(EV_QUIT, &quit_callback, NULL);
					render_set_event_callback(EV_KEY_V, &key_V_callback, NULL);
					render_set_event_callback(EV_KEY_I, &key_I_callback, NULL);
					render_set_event_callback(EV_KEY_UP, &key_UP_callback, NULL);
					render_set_event_callback(EV_KEY_DOWN, &key_DOWN_callback, NULL);
					render_set_event_callback(EV_KEY_LEFT, &key_LEFT_callback, NULL);
					render_set_event_callback(EV_KEY_RIGHT, &key_RIGHT_callback, NULL);
				}
			}

			if(debug_level > 0)
				printf("GUVCVIEW: reset to pixelformat=%x width=%i and height=%i\n",
					v4l2core_get_requested_frame_format(my_vd),
					v4l2core_get_frame_width(my_vd),
					v4l2core_get_frame_height(my_vd));

			v4l2core_start_stream(my_vd);

		}

		/*get the frame from v4l2 core*/
		frame = v4l2core_get_decoded_frame(my_vd);
		if( frame != NULL)
		{
			/*run software autofocus (must be called after frame was grabbed and decoded)*/
			if(do_soft_autofocus || do_soft_focus)
				do_soft_focus = v4l2core_soft_autofocus_run(my_vd, frame);

			/* apply fx effects to the frame
			 * do it before saving the frame
			 * (we want to store the effects)
			 */
			render_frame_fx(frame->yuv_frame, my_render_mask);

			/*check the timers*/
			if(check_photo_timer())
			{
				if((frame->timestamp - my_last_photo_time) > my_photo_timer)
				{
					save_image = 1;
					my_last_photo_time = frame->timestamp;

					if(my_options->photo_npics > 0)
					{
						if(my_photo_npics > 0)
							my_photo_npics--;
						else
						{
							save_image = 0;
							stop_photo_timer(); /*close timer*/
							if(!check_video_timer() && my_options->exit_on_term > 0)
								quit_callback(NULL); /*close app*/
						}
					}
				}
			}

			if(check_video_timer())
			{
				if((frame->timestamp - my_video_begin_time) > my_video_timer)
				{
					stop_video_timer();
					if(!check_photo_timer() && my_options->exit_on_term > 0)
						quit_callback(NULL); /*close app*/
				}
			}

			/*save the frame (photo)*/
			if(save_image)
			{
				char *img_filename = NULL;

				/*get_photo_[name|path] always return a non NULL value*/
				char *name = strdup(get_photo_name());
				char *path = strdup(get_photo_path());

				if(get_photo_sufix_flag())
				{
					char *new_name = add_file_suffix(path, name);
					free(name); /*free old name*/
					name = new_name; /*replace with suffixed name*/
				}
				int pathsize = strlen(path);
				if(path[pathsize] != '/')
					img_filename = smart_cat(path, '/', name);
				else
					img_filename = smart_cat(path, 0, name);

				//if(debug_level > 1)
				//	printf("GUVCVIEW: saving image to %s\n", img_filename);

				snprintf(status_message, 79, _("saving image to %s"), img_filename);
				gui_status_message(status_message);

				v4l2core_save_image(frame, img_filename, get_photo_format());

				free(path);
				free(name);
				free(img_filename);

				save_image = 0; /*reset*/
			}

			/*save the frame (video)*/
			if(video_capture_get_save_video())
			{
				int size = (frame->width * frame->height * 3) / 2;

				uint8_t *input_frame = frame->yuv_frame;
				/*
				 * TODO: check codec_id, format and frame flags
				 * (we may want to store a compressed format
				 */
				if(get_video_codec_ind() == 0) //raw frame
				{
					switch(v4l2core_get_requested_frame_format(my_vd))
					{
						case  V4L2_PIX_FMT_H264:
							input_frame = frame->h264_frame;
							size = (int) frame->h264_frame_size;
							break;
						default:
							input_frame = frame->raw_frame;
							size = (int) frame->raw_frame_size;
							break;
					}

				}
				/*add the frame to the encoder buffer*/
				encoder_add_video_frame(input_frame, size, frame->timestamp, frame->isKeyframe);

				/*
				 * exponencial scheduler
				 *  with 50% threshold (milisec)
				 *  and max value of 250 ms (4 fps)
				 */
				double time_sched = encoder_buff_scheduler(ENCODER_SCHED_LIN, 0.5, 250);
				if(time_sched > 0)
				{
					switch(v4l2core_get_requested_frame_format(my_vd))
					{
						case  V4L2_PIX_FMT_H264:
						{
							uint32_t framerate = lround(time_sched * 1E6); /*nanosec*/
							v4l2core_set_h264_frame_rate_config(my_vd, framerate);
							break;
						}
						default:
						{
							struct timespec req = {
								.tv_sec = 0,
								.tv_nsec = (uint32_t) time_sched * 1E6};/*nanosec*/
							nanosleep(&req, NULL);
							break;
						}
					}
				}
			}

			/* render the osd
			 * must be done after saving the frame 
			 * (we don't want to record the osd effects)
			 */
			render_frame_osd(frame->yuv_frame);

			/* finally render the frame */
			snprintf(render_caption, 29, "Guvcview  (%2.2f fps)", 
				v4l2core_get_realfps(my_vd));
			render_set_caption(render_caption);
			render_frame(frame->yuv_frame);

			/*we are done with the frame buffer release it*/
			v4l2core_release_frame(my_vd, frame);
		}
	}

	v4l2core_stop_stream(my_vd);
	
	/*if we are still saving video then stop it*/
	if(video_capture_get_save_video())
		stop_encoder_thread();

	render_close();

	return ((void *) 0);
}

/*
 * start the encoder thread
 * args:
 *   data - pointer to user data
 *
 * asserts:
 *   none
 *
 * returns: error code
 */
int start_encoder_thread(void *data)
{
	int ret = __THREAD_CREATE(&encoder_thread, encoder_loop, data);
	
	if(ret)
		fprintf(stderr, "GUVCVIEW: encoder thread creation failed (%i)\n", ret);
	else if(debug_level > 2)
		printf("GUVCVIEW: created encoder thread with tid: %u\n", 
			(unsigned int) encoder_thread);

	return ret;
}

/*
 * stop the encoder thread
 * args:
 *   none
 *
 * asserts:
 *   none
 *
 * returns: error code
 */
int stop_encoder_thread()
{
	video_capture_save_video(0);

	__THREAD_JOIN(encoder_thread);

	if(debug_level > 1)
		printf("GUVCVIEW: encoder thread terminated and joined\n");
			
	return 0;
}
