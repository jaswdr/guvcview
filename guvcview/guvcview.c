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
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <errno.h>

#include "gview.h"
#include "gviewv4l2core.h"
#include "gviewrender.h"
#include "gviewencoder.h"
#include "core_time.h"

#include "../config.h"
#include "video_capture.h"
#include "options.h"
#include "config.h"
#include "gui.h"
#include "core_io.h"

int debug_level = 0;

static __THREAD_TYPE capture_thread;

__MUTEX_TYPE capture_mutex = __STATIC_MUTEX_INIT;
__COND_TYPE capture_cond;

/*
 * signal callback
 * args:
 *    signum - signal number
 *
 * return: none
 */
void signal_callback_handler(int signum)
{
	printf("GUVCVIEW Caught signal %d\n", signum);

	switch(signum)
	{
		case SIGINT:
			/* Terminate program */
			quit_callback(NULL);
			break;

		case SIGUSR1:
			gui_click_video_capture_button();
			break;

		case SIGUSR2:
			/* save image */
			video_capture_save_image();
			break;
	}
}

int main(int argc, char *argv[])
{
	/*check stack size*/
	const rlim_t kStackSize = 128L * 1024L * 1024L;   /* min stack size = 128 Mb*/
    struct rlimit rl;
    int result;

    result = getrlimit(RLIMIT_STACK, &rl);
    if (result == 0)
    {
        if (rl.rlim_cur < kStackSize)
        {
            rl.rlim_cur = kStackSize;
            result = setrlimit(RLIMIT_STACK, &rl);
            if (result != 0)
            {
                fprintf(stderr, "GUVCVIEW: setrlimit returned result = %d\n", result);
            }
        }
    }
	
	// Register signal and signal handler
	signal(SIGINT,  signal_callback_handler);
	signal(SIGUSR1, signal_callback_handler);
	signal(SIGUSR2, signal_callback_handler);
	
	/*localization*/
	char* lc_all = setlocale (LC_ALL, "");
	char* lc_dir = bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	char* txtdom = textdomain (GETTEXT_PACKAGE);

	/*parse command line options*/
	if(options_parse(argc, argv))
		return 0;
	
	printf("GUVCVIEW: version %s\n", VERSION);

	/*get command line options*/
	options_t *my_options = options_get();

	char *config_path = smart_cat(getenv("HOME"), '/', ".config/guvcview2");
	mkdir(config_path, 0777);

	char *device_name = get_file_basename(my_options->device);

	char *config_file = smart_cat(config_path, '/', device_name);

	/*clean strings*/
	free(config_path);
	free(device_name);

	/*load config data*/
	config_load(config_file);

	/*update config with options*/
	config_update(my_options);

	/*get config data*/
	config_t *my_config = config_get();

	debug_level = my_options->verbosity;
	
	if (debug_level > 1) printf("GUVCVIEW: language catalog=> dir:%s type:%s cat:%s.mo\n",
		lc_dir, lc_all, txtdom);

	/*select render API*/
	int render = RENDER_SDL;

	if(strcasecmp(my_config->render, "none") == 0)
		render = RENDER_NONE;
	else if(strcasecmp(my_config->render, "sdl") == 0)
	{
#if ENABLE_SDL2
		render = RENDER_SDL;
#elif ENABLE_SFML
		render = RENDER_SFML;
		printf("GUVCVIEW: not build with sdl2 support (rebuild with --enable-sdl2)\n");
		printf("GUVCVIEW: using sfml render instead\n");
#else
		render = RENDER_NONE;
		printf("GUVCVIEW: not build with sfml or sdl2 support\n");
		printf("GUVCVIEW: not rendering any video preview\n");
#endif
	}
	else if(strcasecmp(my_config->render, "sfml") == 0)
	{
#if ENABLE_SFML
		render = RENDER_SFML;
#elif ENABLE_SDL2
		render = RENDER_SDL;
		printf("GUVCVIEW: not build with sfml support (rebuild with --enable-sfml)\n");
		printf("GUVCVIEW: using sdl2 render instead\n");
#else
		render = RENDER_NONE;
		printf("GUVCVIEW: not build with sfml or sdl2 support\n");
		printf("GUVCVIEW: not rendering any video preview\n");
#endif
	}

	/*select gui API*/
#if HAS_GTK3
	int gui = GUI_GTK3;
#elif HAS_QT5
	int gui = GUI_QT5;
#else
	int gui = GUI_NONE;
#endif

	if(strncasecmp(my_config->gui, "none", 4) == 0)
		gui = GUI_NONE;
	else if(strncasecmp(my_config->gui, "gtk3", 4) == 0)
	{
#if HAS_GTK3
		gui = GUI_GTK3;
#elif HAS_QT5
		gui = GUI_QT5;
		fprintf(stderr, "Guvcview was not build with Gtk3 support: using Qt5\n");
		strncpy(my_config->gui, "qt5", 4);
#else
		gui = GUI_NONE;
		fprintf(stderr, "Guvcview was not build with gui support\n");
		strncpy(my_config->gui, "none", 4);
#endif
	}
	else if(strncasecmp(my_config->gui, "qt5", 3) == 0)
	{
#if HAS_QT5
		gui = GUI_QT5;
#elif HAS_GTK3
		gui = GUI_GTK3;
		fprintf(stderr, "Guvcview was not build with Qt5 support: using Gtk3\n");
		strncpy(my_config->gui, "gtk3", 4);
#else
		gui = GUI_NONE;
		fprintf(stderr, "Guvcview was not build with gui support\n");
		strncpy(my_config->gui, "none", 4);
#endif
	}

	if(debug_level > 0)
		printf("Guvcview: using GUI %i for option %c%c%c\n", gui, my_config->gui[0], my_config->gui[1],my_config->gui[2]);

	set_gui_api(gui);

	/*select audio API*/
	int audio = AUDIO_PORTAUDIO;

	if(strcasecmp(my_config->audio, "none") == 0)
		audio = AUDIO_NONE;
	else if(strcasecmp(my_config->audio, "port") == 0)
		audio = AUDIO_PORTAUDIO;
#if HAS_PULSEAUDIO
	else if(strcasecmp(my_config->audio, "pulse") == 0)
		audio = AUDIO_PULSE;
#endif

	if(debug_level > 1)
		printf("GUVCVIEW: main thread (tid: %u)\n",
			(unsigned int) syscall (SYS_gettid));
		
	/*set the v4l2 core verbosity*/
	v4l2core_set_verbosity(debug_level);

	/*set the v4l2core device (redefines language catalog)*/
	v4l2_dev_t *vd = create_v4l2_device_handler(my_options->device);
	if(!vd)
	{
		char message[50];
		snprintf(message, 49, "no video device (%s) found", my_options->device);
		gui_error("Guvcview error", "no video device found", 1);
		options_clean();
		return -1;
	}
	else		
		set_render_flag(render);
	
	if(my_options->disable_libv4l2)
		v4l2core_disable_libv4l2(vd);

	/*select capture method*/
	if(strcasecmp(my_config->capture, "read") == 0)
		v4l2core_set_capture_method(vd, IO_READ);
	else
		v4l2core_set_capture_method(vd, IO_MMAP);

	/*set software autofocus sort method*/
	v4l2core_soft_autofocus_set_sort(AUTOF_SORT_INSERT);

	/*set the intended fps*/
	v4l2core_define_fps(vd, my_config->fps_num,my_config->fps_denom);

	/*set fx masks*/
	set_render_fx_mask(my_config->video_fx);
	set_audio_fx_mask(my_config->audio_fx);

	/*set OSD mask*/
	/*make sure VU meter OSD is disabled since it's set by the audio capture*/
	my_config->osd_mask &= ~REND_OSD_VUMETER_MONO;
	my_config->osd_mask &= ~REND_OSD_VUMETER_STEREO;
	render_set_osd_mask(my_config->osd_mask);

	/*select video codec*/
	if(debug_level > 1)
		printf("GUVCVIEW: setting video codec to '%s'\n", my_config->video_codec);
		
	int vcodec_ind = encoder_get_video_codec_ind_4cc(my_config->video_codec);
	if(vcodec_ind < 0)
	{
		char message[50];
		snprintf(message, 49, "invalid video codec '%s' using raw input", my_config->video_codec);
		gui_error("Guvcview warning", message, 0);

		fprintf(stderr, "GUVCVIEW: invalid video codec '%s' using raw input\n", my_config->video_codec);
		vcodec_ind = 0;
	}
	set_video_codec_ind(vcodec_ind);

	/*select audio codec*/
	if(debug_level > 1)
		printf("GUVCVIEW: setting audio codec to '%s'\n", my_config->audio_codec);
	int acodec_ind = encoder_get_audio_codec_ind_name(my_config->audio_codec);
	if(acodec_ind < 0)
	{
		char message[50];
		snprintf(message, 49, "invalid audio codec '%s' using pcm input", my_config->audio_codec);
		gui_error("Guvcview warning", message, 0);

		fprintf(stderr, "GUVCVIEW: invalid audio codec '%s' using pcm input\n", my_config->audio_codec);
		acodec_ind = 0;
	}
	set_audio_codec_ind(acodec_ind);

	/*check if need to load a profile*/
	if(my_options->prof_filename)
		v4l2core_load_control_profile(vd, my_options->prof_filename);

	/*set the profile file*/
	if(!my_config->profile_name)
		my_config->profile_name = strdup(get_profile_name());
	if(!my_config->profile_path)
		my_config->profile_path = strdup(get_profile_path());
	set_profile_name(my_config->profile_name);
	set_profile_path(my_config->profile_path);

	/*set the video file*/
	if(!my_config->video_name)
		my_config->video_name = strdup(get_video_name());
	if(!my_config->video_path)
		my_config->video_path = strdup(get_video_path());
	set_video_name(my_config->video_name);
	set_video_path(my_config->video_path);

	/*set the photo(image) file*/
	if(!my_config->photo_name)
		my_config->photo_name = strdup(get_photo_name());
	if(!my_config->photo_path)
		my_config->photo_path = strdup(get_photo_path());
	set_photo_name(my_config->photo_name);
	set_photo_path(my_config->photo_path);

	/*set audio interface verbosity*/
	audio_set_verbosity(debug_level);

	/*create the inital audio context (stored staticly in video_capture)*/
	audio_context_t *audio_ctx = create_audio_context(audio, my_config->audio_device);

	if(audio_ctx != NULL)
		my_config->audio_device = audio_get_device_index(audio_ctx);
	else
		fprintf(stderr, "GUVCVIEW: couldn't get a valid audio context for the selected api - disabling audio\n");
	
	encoder_set_verbosity(debug_level);

	/*start capture thread if not in control_panel mode*/
	if(!my_options->control_panel)
	{
		/*
		 * prepare format:
		 *   doing this inside the capture thread may create a race
		 *   condition with gui_attach, as it requires the current
		 *   format to be set
		 */
		v4l2core_prepare_new_format(vd, my_config->format);
		/*prepare resolution*/
		v4l2core_prepare_new_resolution(vd, my_config->width, my_config->height);
		/*try to set the video stream format on the device*/
		int ret = v4l2core_update_current_format(vd);

		if(ret != E_OK)
		{
			fprintf(stderr, "GUCVIEW: could not set the defined stream format\n");
			fprintf(stderr, "GUCVIEW: trying first listed stream format\n");

			v4l2core_prepare_valid_format(vd);
			v4l2core_prepare_valid_resolution(vd);
			ret = v4l2core_update_current_format(vd);

			if(ret != E_OK)
			{
				fprintf(stderr, "GUCVIEW: also could not set the first listed stream format\n");
				fprintf(stderr, "GUVCVIEW: Video capture failed\n");

				gui_error("Guvcview error", "could not start a video stream in the device", 1);
			}
		}

		if(ret == E_OK)
		{
			__INIT_COND(&capture_cond);
			__LOCK_MUTEX(&capture_mutex);

			capture_loop_data_t cl_data;
			cl_data.options = (void *) my_options;
			cl_data.config = (void *) my_config;

			ret = __THREAD_CREATE(&capture_thread, capture_loop, (void *) &cl_data);

			if(ret)
			{
				fprintf(stderr, "GUVCVIEW: Video thread creation failed\n");
				gui_error("Guvcview error", "could not start the video capture thread", 1);
			}
			else if(debug_level > 2)
				printf("GUVCVIEW: created capture thread with tid: %u\n", (unsigned int) capture_thread);

			struct timespec now;
			clock_gettime(CLOCK_REALTIME, &now);
			now.tv_sec += 5; /*wait at most 5 seconds for capture_cond*/
			ret = __COND_TIMED_WAIT(&capture_cond,&capture_mutex, &now);
			__UNLOCK_MUTEX(&capture_mutex);
			__CLOSE_COND(&capture_cond);

			if(ret == ETIMEDOUT)
				fprintf(stderr, "GUVCVIEW: capture_cond wait timedout (5 sec)\n");
			else if (ret != 0)
				fprintf(stderr, "GUVCVIEW: capture_cond wait unknown error: %i\n", ret);
		}
	}

	/*initialize the gui */
	gui_attach(800, 600, my_options->control_panel);

	/*run the gui loop*/
	gui_run();

	if(debug_level > 2)
		printf("GUVCVIEW: joining capture thread\n");
	if(!my_options->control_panel)
		__THREAD_JOIN(capture_thread);

	if(debug_level > 1)
		printf("GUVCVIEW: closing audio context\n");
	/*closes the audio context (stored staticly in video_capture)*/
	close_audio_context();
	/*closes the v4l2 device handler (stored staticly in video_capture)*/
	close_v4l2_device_handler();

    /*save config before cleaning the options*/
	config_save(config_file);

	if(config_file)
		free(config_file);

	config_clean();
	options_clean();

	if(debug_level > 0)
		printf("GUVCVIEW: good bye\n");

	return 0;
}
