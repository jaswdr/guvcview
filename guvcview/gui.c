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

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
/* support for internationalization - i18n */
#include <locale.h>
#include <libintl.h>

#include "../config.h"

#include "core_io.h"
#include "video_capture.h"
#include "gviewencoder.h"
#include "config.h"

#include "gui.h"
#if HAS_GTK3
#include "gui_gtk3.h"
#endif
#if HAS_QT5
#include "gui_qt5.h"
#endif


extern int debug_level;

int is_control_panel = 0;

#if HAS_GTK3
static int gui_api = GUI_GTK3;
#elif HAS_QT5
static int gui_api = GUI_QT5;
#else
static int gui_api = GUI_NONE;
#endif

/*default camera button action: DEF_ACTION_IMAGE - save image; DEF_ACTION_VIDEO - save video*/
static int default_camera_button_action = 0;

/*control profile file name*/
static char *profile_name = NULL;

/*control profile path to dir*/
static char *profile_path = NULL;

/*photo basename*/
static char *photo_name = NULL;
/*photo path*/
static char *photo_path = NULL;
/*photo sufix flag*/
static int photo_sufix_flag = 1;
/*photo format*/
static int photo_format = IMG_FMT_JPG;

/*video basename*/
static char *video_name = NULL;
/*video path*/
static char *video_path = NULL;
/*video sufix flag*/
static int video_sufix_flag = 1;
/*photo format*/
static int video_muxer = ENCODER_MUX_MKV;

static int my_video_codec_ind = 0;
static int my_audio_codec_ind = 0;

/*index: 0 numerator; 1 denominator*/
static int my_fps[2] = {0, 0};

/*
 * sets the Gui API 
 * args:
 *   gui api
 *
 * asserts:
 *   none
 *
 * returns: none
 */
void set_gui_api(int gui)
{
	gui_api = GUI_NONE;
	
	if( gui == GUI_NONE || 
		gui == GUI_GTK3 || 
		gui == GUI_QT5 )
	{
		gui_api = gui;
	}
}

/*
 * gets the current video codec index
 * args:
 *   none
 *
 * asserts:
 *   none
 *
 * returns: current codec index
 */
int get_video_codec_ind()
{
	return my_video_codec_ind;
}

/*
 * sets the current video codec index
 * args:
 *   index - codec index
 *
 * asserts:
 *   none
 *
 * returns: none
 */
void set_video_codec_ind(int index)
{
	my_video_codec_ind = index;

	/*update config*/
	config_t *my_config = config_get();
	if(index == 0)
		strncpy(my_config->video_codec, "raw", 4);
	else
	{
		const char *codec_4cc = encoder_get_video_codec_4cc(index);
		if(codec_4cc)
		{
			strncpy(my_config->video_codec, codec_4cc, 4);
			lowercase(my_config->video_codec);
		}
	}
}

/*
 * gets the current audio codec index
 * args:
 *   none
 *
 * asserts:
 *   none
 *
 * returns: current codec index
 */
int get_audio_codec_ind()
{
	return my_audio_codec_ind;
}

/*
 * sets the current audio codec index
 * args:
 *   index - codec index
 *
 * asserts:
 *   none
 *
 * returns: none
 */
void set_audio_codec_ind(int index)
{
	my_audio_codec_ind = index;

	/*update config*/
	config_t *my_config = config_get();

	const char *codec_name = encoder_get_audio_codec_name(index);
	if(codec_name)
	{
		strncpy(my_config->audio_codec, codec_name, 4);
		lowercase(my_config->video_codec);
	}
}

/*
 * gets the current fps numerator
 * args:
 *   none
 *
 * asserts:
 *   none
 *
 * returns: current fps numerator
 */
//int gui_get_fps_num()
//{
//	return my_fps[0];
//}

/*
 * gets the current fps denominator
 * args:
 *   none
 *
 * asserts:
 *   none
 *
 * returns: current fps denominator
 */
//int gui_get_fps_denom()
//{
//	return my_fps[1];
//}

/*
 * stores the fps
 * args:
 *   fps - array with fps numerator and denominator
 *
 * asserts:
 *   none
 *
 * returns: none
 */
void gui_set_fps(int fps[2])
{
	my_fps[0] = fps[0];
	my_fps[1] = fps[1];

	/*update config*/
	config_t *my_config = config_get();

	my_config->fps_num = my_fps[0];
	my_config->fps_denom = my_fps[1];
}

/*
 * gets the default camera button action
 * args:
 *   none
 *
 * asserts:
 *   none
 *
 * returns: default camera button action
 */
int get_default_camera_button_action()
{
	return default_camera_button_action;
}

/*
 * sets the default camera button action
 * args:
 *   action: camera button default action
 *
 * asserts:
 *   none
 *
 * returns: none
 */
void set_default_camera_button_action(int action)
{
	default_camera_button_action = action;
}

/*
 * click image capture button
 * args:
 *    none
 *
 * asserts:
 *    none
 *
 * returns: none
 */
void gui_click_image_capture_button()
{
	switch(gui_api)
	{
#if HAS_QT5
		case GUI_QT5:
			gui_click_image_capture_button_qt5();
			break;
#endif
#if HAS_GTK3
		case GUI_GTK3:
			gui_click_image_capture_button_gtk3();
			break;
#endif
		case GUI_NONE:
		default:
			video_capture_save_image();
			break;
	}
}

/*
 * click video capture button
 * args:
 *   none
 *
 * asserts:
 *    none
 *
 * returns: none
 */
void gui_click_video_capture_button()
{
	switch(gui_api)
	{
#if HAS_QT5
		case GUI_QT5:
			gui_click_video_capture_button_qt5();
			break;
#endif
#if HAS_GTK3
		case GUI_GTK3:
			gui_click_video_capture_button_gtk3();
			break;
#endif
		case GUI_NONE:
		default:
			if(!get_encoder_status())
				start_encoder_thread();
			else
			{
				if(check_video_timer())
					reset_video_timer();
				stop_encoder_thread();
			}
			break;
		
	}
}
/*
 * sets the Image capture button label
 * args:
 *   label: Image capture button label
 *
 * asserts:
 *   none
 *
 * returns: none
 */
void gui_set_image_capture_button_label(const char *label)
{
	switch(gui_api)
	{
#if HAS_GTK3
		case GUI_GTK3:
			gui_set_image_capture_button_label_gtk3(label);
			break;
#endif
		case GUI_NONE:
		default:
			break;
	}
}

/*
 * sets the Video capture button status (on|off)
 * args:
 *   flag: video capture button status
 *
 * asserts:
 *   none
 *
 * returns: none
 */
void gui_set_video_capture_button_status(int flag)
{
	switch(gui_api)
	{
#if HAS_GTK3
		case GUI_GTK3:
			gui_set_video_capture_button_status_gtk3(flag);
			break;
#endif
		case GUI_NONE:
		default:
			break;
	}
}

/*
 * gets the control profile file name
 * args:
 *   none
 *
 * asserts:
 *   none
 *
 * returns: control profile file name
 */
char *get_profile_name()
{
	if(!profile_name)
		profile_name = strdup("default.gpfl");

	return profile_name;
}

/*
 * sets the control profile file name
 * args:
 *   name: control profile file name
 *
 * asserts:
 *   none
 *
 * returns: none
 */
void set_profile_name(const char *name)
{
	if(profile_name != NULL)
		free(profile_name);

	profile_name = strdup(name);

	/* update the config */
	config_t *my_config = config_get();

	/*this can be the function arg 'name'*/
	if(my_config->profile_name)
		free(my_config->profile_name);

	/*so here we use the dup string*/
	my_config->profile_name = strdup(profile_name);
}

/*
 * gets the control profile path (to dir)
 * args:
 *   none
 *
 * asserts:
 *   none
 *
 * returns: control profile file path
 */
char *get_profile_path()
{
	if(!profile_path)
		profile_path = strdup(getenv("HOME"));

	return profile_path;
}

/*
 * sets the control profile path (to dir)
 * args:
 *   path: control profile path
 *
 * asserts:
 *   none
 *
 * returns: none
 */
void set_profile_path(const char *path)
{
	if(profile_path != NULL)
		free(profile_path);

	profile_path = strdup(path);

	/* update the config */
	config_t *my_config = config_get();

	/*this can be the function arg 'path'*/
	if(my_config->profile_path)
		free(my_config->profile_path);

	/*so here we use the dup string*/
	my_config->profile_path = strdup(profile_path);
}

/*
 * gets video sufix flag
 * args:
 *   none
 *
 * asserts:
 *   none
 *
 * returns: video sufix flag
 */
int get_video_sufix_flag()
{
	return video_sufix_flag;
}

/*
 * sets the video sufix flag
 * args:
 *   flag: video sufix flag
 *
 * asserts:
 *   none
 *
 * returns: none
 */
void set_video_sufix_flag(int flag)
{
	video_sufix_flag = flag;
}

/*
 * gets video muxer
 * args:
 *   none
 *
 * asserts:
 *   none
 *
 * returns: video muxer
 */
int get_video_muxer()
{
	return video_muxer;
}

/*
 * sets video muxer
 * args:
 *   muxer - video muxer (ENCODER_MUX_[MKV|WEBM|AVI])
 *
 * asserts:
 *   none
 *
 * returns: none
 */
void set_video_muxer(int muxer)
{
	video_muxer = muxer;
}

/*
 * gets the video file basename
 * args:
 *   none
 *
 * asserts:
 *   none
 *
 * returns: video file basename
 */
char *get_video_name()
{
	if(!video_name)
		video_name = strdup("my_video.mkv");

	return video_name;
}

/*
 * sets the video file basename
 * args:
 *   name: video file basename
 *
 * asserts:
 *   none
 *
 * returns: none
 */
void set_video_name(const char *name)
{
	if(video_name != NULL)
		free(video_name);

	video_name = strdup(name);

	/*get image format*/
	char *ext = get_file_extension(name);
	if(ext == NULL)
	{
		if(video_name)
			free(video_name);

		fprintf(stderr, "GUVCVIEW: no valid file extension for video file %s\n",
			name);
		fprintf(stderr, "GUVCVIEW: using muxer %i\n", get_video_muxer());
		switch(get_video_muxer())
		{
			case ENCODER_MUX_MKV:
				video_name = set_file_extension(name, "mkv");
				break;
			case ENCODER_MUX_WEBM:
				video_name = set_file_extension(name, "webm");
				break;
			default:
				video_name = set_file_extension(name, "avi");
				break;
		}
	}
	else if( strcasecmp(ext, "mkv") == 0)
		set_video_muxer(ENCODER_MUX_MKV);
	else if ( strcasecmp(ext, "webm") == 0 )
	{
		set_video_muxer(ENCODER_MUX_WEBM);
		/*force webm codecs*/
		set_webm_codecs();
	}
	else if ( strcasecmp(ext, "avi") == 0 )
		set_video_muxer(ENCODER_MUX_AVI);

	if(ext)
		free(ext);
	
	/* update the config */
	config_t *my_config = config_get();

	/*this can be the function arg 'name'*/
	if(my_config->video_name)
		free(my_config->video_name);

	/*so here we use the dup string*/
	my_config->video_name = strdup(video_name);
}

/*
 * gets the video file path (to dir)
 * args:
 *   none
 *
 * asserts:
 *   none
 *
 * returns: video file path
 */
char *get_video_path()
{
	if(!video_path)
		video_path = strdup(getenv("HOME"));

	return video_path;
}

/*
 * sets video path (to dir)
 * args:
 *   path: video file path
 *
 * asserts:
 *   none
 *
 * returns: none
 */
void set_video_path(const char *path)
{
	if(video_path != NULL)
		free(video_path);

	video_path = strdup(path);

	/* update the config */
	config_t *my_config = config_get();

	/*this can be the function arg 'path'*/
	if(my_config->video_path)
		free(my_config->video_path);

	/*so here we use the dup string*/
	my_config->video_path = strdup(video_path);
}

/*
 * gets photo sufix flag
 * args:
 *   none
 *
 * asserts:
 *   none
 *
 * returns: photo sufix flag
 */
int get_photo_sufix_flag()
{
	return photo_sufix_flag;
}

/*
 * sets the photo sufix flag
 * args:
 *   flag: photo sufix flag
 *
 * asserts:
 *   none
 *
 * returns: none
 */
void set_photo_sufix_flag(int flag)
{
	photo_sufix_flag = flag;
}

/*
 * gets photo format
 * args:
 *   none
 *
 * asserts:
 *   none
 *
 * returns: photo format
 */
int get_photo_format()
{
	return photo_format;
}

/*
 * sets photo format
 * args:
 *   format - photo format (IMG_FMT_[JPG|BMP|PNG|RAW])
 *
 * asserts:
 *   none
 *
 * returns: none
 */
void set_photo_format(int format)
{
	photo_format = format;
}

/*
 * gets the photo file basename
 * args:
 *   none
 *
 * asserts:
 *   none
 *
 * returns: photo file basename
 */
char *get_photo_name()
{
	if(!photo_name)
		photo_name = strdup("my_photo.jpg");

	return photo_name;
}

/*
 * sets the photo file basename and image format
 * args:
 *   name: photo file basename
 *
 * asserts:
 *   none
 *
 * returns: none
 */
void set_photo_name(const char *name)
{
	if(photo_name)
		free(photo_name);

	photo_name = strdup(name);

	/*get image format*/
	char *ext = get_file_extension(name);
	if(ext == NULL)
	{
		if(photo_name)
			free(photo_name);

		fprintf(stderr, "GUVCVIEW: no valid file extension for image file %s\n",
			name);
		fprintf(stderr, "GUVCVIEW: using format %i\n", get_photo_format());
		switch(get_photo_format())
		{
			case IMG_FMT_JPG:
				photo_name = set_file_extension(name, "jpg");
				break;
			case IMG_FMT_PNG:
				photo_name = set_file_extension(name, "png");
				break;
			case IMG_FMT_BMP:
				photo_name = set_file_extension(name, "bmp");
				break;
			default:
				photo_name = set_file_extension(name, "raw");
				break;
		}
	}
	else if( strcasecmp(ext, "jpg") == 0 ||
			 strcasecmp(ext, "jpeg") == 0 )
		set_photo_format(IMG_FMT_JPG);
	else if ( strcasecmp(ext, "png") == 0 )
		set_photo_format(IMG_FMT_PNG);
	else if ( strcasecmp(ext, "bmp") == 0 )
		set_photo_format(IMG_FMT_BMP);
	else if ( strcasecmp(ext, "raw") == 0 )
		set_photo_format(IMG_FMT_RAW);

	if(ext)
		free(ext);

	/*update the config*/
	config_t *my_config = config_get();

	/*this can be the function arg 'name'*/
	if(my_config->photo_name)
		free(my_config->photo_name);

	/*so here we use the dup string*/
	my_config->photo_name = strdup(photo_name);
}

/*
 * gets the photo file path (to dir)
 * args:
 *   none
 *
 * asserts:
 *   none
 *
 * returns: photo file path
 */
char *get_photo_path()
{
	if(!photo_path)
		photo_path = strdup(getenv("HOME"));

	return photo_path;
}

/*
 * sets photo path (to dir)
 * args:
 *   path: photo file path
 *
 * asserts:
 *   none
 *
 * returns: none
 */
void set_photo_path(const char *path)
{
	if(photo_path != NULL)
		free(photo_path);

	photo_path = strdup(path);

	/*update the config*/
	config_t *my_config = config_get();

	/*this can be the function arg 'path'*/
	if(my_config->photo_path)
		free(my_config->photo_path);

	/*so here we use the dup string*/
	my_config->photo_path = strdup(photo_path);
}

/*
 * set webm codecs in codecs list
 * args:
 *   none
 *
 * asserts:
 *   none
 *
 * returns: none
 */
void set_webm_codecs()
{
	switch(gui_api)
	{
#if HAS_QT5
		case GUI_QT5:
			set_webm_codecs_qt5();
			break;
#endif		
#if HAS_GTK3
		case GUI_GTK3:
			set_webm_codecs_gtk3();
			break;
#endif
		case GUI_NONE:
		default:
			break;
	}
}

/*
 * GUI warning/error dialog
 * args:
 *   title - dialog title string
 *   message - error message string
 *   fatal - flag a fatal error (display device list combo box)
 *
 * asserts:
 *   none
 *
 * returns: none
 */
void gui_error(
	const char *title,
	const char *message,
	int fatal)
{
	fprintf(stderr, "GUVCVIEW (%i): %s\n\t %s\n", gui_api, title, message);
	switch(gui_api)
	{
#if HAS_QT5
		case GUI_QT5:
			gui_error_qt5(title, message, fatal);
			break;
#endif
#if HAS_GTK3
		case GUI_GTK3:
			gui_error_gtk3(title, message, fatal);
			break;
#endif
		case GUI_NONE:
		default:
			break;
	}
}

/*
 * adds a message to the status bar
 * args:
 *    message - message string
 *
 * asserts:
 *    none
 *
 * returns: none
 */
void gui_status_message(const char *message)
{
	switch(gui_api)
	{
#if HAS_QT5
		case GUI_QT5:
			gui_status_message_qt5(message);
			break;
#endif
#if HAS_GTK3
		case GUI_GTK3:
			gui_status_message_gtk3(message);
			break;
#endif
		case GUI_NONE:
		default:
			break;
	}

	printf("GUVCVIEW: (status) %s\n", message);
}

/*
 * GUI initialization
 * args:
 *   width - window width
 *   height - window height
 *   control_panel - flag control panel mode (1 -set; 0 -no)
 *
 * asserts:
 *   none
 *
 * returns: error code
 */
int gui_attach(int width, int height, int control_panel)
{
	int ret = 0;

	is_control_panel = control_panel;

	switch(gui_api)
	{
#if HAS_QT5		
		case GUI_QT5:
			ret = gui_attach_qt5(width, height);
			if(ret)
				gui_api = GUI_NONE;
			break;
#endif
#if HAS_GTK3
		case GUI_GTK3:
			ret = gui_attach_gtk3(width, height);
			if(ret)
				gui_api = GUI_NONE;
			break;
#endif
		case GUI_NONE:
		default:
			break;
	}

	return ret;
}

/*
 * run the GUI loop
 * args:
 *   none
 *
 * asserts:
 *   none
 *
 * returns: error code
 */
int gui_run()
{

	int ret = 0;

	switch(gui_api)
	{
#if HAS_QT5
		case GUI_QT5:
			ret = gui_run_qt5();
			break;
#endif
#if HAS_GTK3		
		case GUI_GTK3:
			ret = gui_run_gtk3();
			break;
#endif
		case GUI_NONE:
		default:
			break;
	}

	return ret;
}

/*
 * closes and cleans the GUI
 * args:
 *   none
 *
 * asserts:
 *   none
 *
 * returns: none
 */
void gui_close()
{
	if(debug_level > 1)
		printf("GUVCVIEW: free profile name\n");
	if(profile_name != NULL)
		free(profile_name);
	profile_name = NULL;
	if(debug_level > 1)
		printf("GUVCVIEW: free profile path\n");
	if(profile_path != NULL)
		free(profile_path);
	profile_path = NULL;
	if(debug_level > 1)
		printf("GUVCVIEW: free video name\n");
	if(video_name != NULL)
		free(video_name);
	video_name = NULL;
	if(debug_level > 1)
		printf("GUVCVIEW: free video path\n");
	if(video_path != NULL)
		free(video_path);
	video_path = NULL;
	if(debug_level > 1)
		printf("GUVCVIEW: free photo name\n");
	if(photo_name != NULL)
		free(photo_name);
	photo_name = NULL;
	if(debug_level > 1)
		printf("GUVCVIEW: free photo path\n");
	if(photo_path != NULL)
		free(photo_path);
	photo_path = NULL;

	if(debug_level > 1)
		printf("GUVCVIEW: close GUI API\n");
	switch(gui_api)
	{
#if HAS_QT5		
		case GUI_QT5:
			gui_close_qt5();
			break;
#endif
#if HAS_GTK3
		case GUI_GTK3:
			gui_close_gtk3();
			break;
#endif
		case GUI_NONE:
		default:
			break;
	}
}
