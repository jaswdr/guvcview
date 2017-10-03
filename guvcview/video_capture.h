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
#ifndef VIDEO_CAPTURE_H
#define VIDEO_CAPTURE_H

#include <inttypes.h>
#include <sys/types.h>

#include "gviewaudio.h"
#include "gviewv4l2core.h"

typedef struct _capture_loop_data_t
{
	void *options;
	void *config;
	void *device;
} capture_loop_data_t;

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
void set_render_flag(int value);

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
uint32_t get_render_fx_mask();

/*
 * set render fx mask
 * args:
 *    new_mask - new render fx filter mask
 *
 * asserts:
 *    none
 *
 * returns: none
 */
void set_render_fx_mask(uint32_t new_mask);

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
uint32_t get_audio_fx_mask();

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
void set_audio_fx_mask(uint32_t new_mask);

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
void set_soft_autofocus(int value);

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
void set_soft_focus(int value);

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
int check_photo_timer();

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
void stop_photo_timer();

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
int check_video_timer();

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
void reset_video_timer();

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
int quit_callback(void *data);

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
void video_capture_save_image();

/*
 * get encoder status
 * args:
 *    none
 *
 * asserts:
 *    none
 *
 * returns: encoder status (1 -running; 0 -not started)
 */
int get_encoder_status();

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
void request_format_update();

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
v4l2_dev_t *create_v4l2_device_handler(const char *device);

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
void close_v4l2_device_handler();

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
v4l2_dev_t *get_v4l2_device_handler();

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
audio_context_t *create_audio_context(int api, int device);

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
void close_audio_context();

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
audio_context_t *get_audio_context();

/*
 * start the encoder thread
 * args:
 *   none
 *
 * asserts:
 *   none
 *
 * returns: error code
 */
int start_encoder_thread();

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
int stop_encoder_thread();

/*
 * capture loop (should run in a separate thread)
 * args:
 *    data - pointer to user data
 *
 * asserts:
 *    device data is not null
 *
 * returns: pointer to return code
 */
void *capture_loop(void *data);

#endif
