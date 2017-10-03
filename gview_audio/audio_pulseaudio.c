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
#include <pulse/pulseaudio.h>
/* support for internationalization - i18n */
#include <locale.h>
#include <libintl.h>

#include "gview.h"
#include "audio.h"
#include "core_time.h"
#include "gviewaudio.h"

extern int verbosity;

static int sample_index = 0;


// From pulsecore/macro.h
#define pa_memzero(x,l) (memset((x), 0, (l)))
#define pa_zero(x) (pa_memzero(&(x), sizeof(x)))

static pa_stream *recordstream = NULL; // pulse audio stream
static pa_context *pa_ctx = NULL; //pulse context

static uint32_t latency_ms = 15; // requested initial latency in milisec: 0 use max
static pa_usec_t latency = 0; //real latency in usec (for timestamping)

static int sink_index = 0;
static int source_index = 0;

static __THREAD_TYPE my_read_thread;

/*
 * clean up and disconnect
 * args:
 *    pa_ctx - pointer to pulse context
 *    pa_ml - pointer to pulse mainloop
 *
 * asserts:
 *    none
 *
 * returns:
 *    none
 */
static void finish(pa_context *pa_ctx, pa_mainloop *pa_ml)
{
	/* clean up and disconnect */
	pa_context_disconnect(pa_ctx);
	pa_context_unref(pa_ctx);
	pa_mainloop_free(pa_ml);
}

/*
 * This callback gets called when our context changes state.
 *  We really only care about when it's ready or if it has failed
 * args:
 *    c -pointer to pulse context
 *    data - pointer to user data
 *
 * asserts:
 *    none
 *
 * retuns: none
 */
static void pa_state_cb(pa_context *c, void *data)
{
	pa_context_state_t state;
	int *pa_ready = data;
	state = pa_context_get_state(c);
	switch  (state)
	{
		// These are just here for reference
		case PA_CONTEXT_UNCONNECTED:
		case PA_CONTEXT_CONNECTING:
		case PA_CONTEXT_AUTHORIZING:
		case PA_CONTEXT_SETTING_NAME:
		default:
			break;
		case PA_CONTEXT_FAILED:
		case PA_CONTEXT_TERMINATED:
			*pa_ready = 2;
			break;
		case PA_CONTEXT_READY:
			*pa_ready = 1;
			break;
	}
}

/*
 * pa_mainloop will call this function when it's ready to tell us
 *  about a source (input).
 *  Since we're not threading when listing devices,
 *  there's no need for mutexes on the devicelist structure
 * args:
 *    c - pointer to pulse context
 *    l - pointer to source info
 *    eol - end of list
 *    data - pointer to user data (audio context)
 *
 * asserts:
 *    none
 *
 * returns: none
 */
static void pa_sourcelist_cb(pa_context *c, const pa_source_info *l, int eol, void *data)
{
    audio_context_t *audio_ctx = (audio_context_t *) data;

    int channels = 1;

	 /*
     * If eol is set to a positive number,
     * you're at the end of the list
     */
    if (eol > 0)
        return;

	source_index++;

	if(l->sample_spec.channels <1)
		channels = 1;
	else
		channels = l->sample_spec.channels;

	double my_latency = 0.0;
	
	if(verbosity > 0)
	{
		printf("AUDIO: =======[ Input Device #%d ]=======\n", source_index);
		printf("       Description: %s\n", l->description);
		printf("       Name: %s\n", l->name);
		printf("       Index: %d\n", l->index);
		printf("       Channels: %d (default to: %d)\n", l->sample_spec.channels, channels);
		printf("       SampleRate: %d\n", l->sample_spec.rate);
		printf("       Latency: %llu (usec)\n", (long long unsigned) l->latency);
		printf("       Configured Latency: %llu (usec)\n", (long long unsigned) l->configured_latency);
		printf("       Card: %d\n", l->card);
		printf("\n");
	}
	
	if(my_latency <= 0.0)
		my_latency = (double) latency_ms / 1000;

	if(l->monitor_of_sink == PA_INVALID_INDEX)
	{
		audio_ctx->num_input_dev++;
		/*add device to list*/
		audio_ctx->list_devices = realloc(audio_ctx->list_devices, audio_ctx->num_input_dev * sizeof(audio_device_t));
		if(audio_ctx->list_devices == NULL)
		{
			fprintf(stderr,"AUDIO: FATAL memory allocation failure (pa_sourcelist_cb): %s\n", strerror(errno));
			exit(-1);
		}
		/*fill device data*/
		audio_ctx->list_devices[audio_ctx->num_input_dev-1].id = l->index; /*saves dev id*/
		strncpy(audio_ctx->list_devices[audio_ctx->num_input_dev-1].name,  l->name, 511);
		strncpy(audio_ctx->list_devices[audio_ctx->num_input_dev-1].description, l->description, 255);
		audio_ctx->list_devices[audio_ctx->num_input_dev-1].channels = channels;
		audio_ctx->list_devices[audio_ctx->num_input_dev-1].samprate = l->sample_spec.rate;
		audio_ctx->list_devices[audio_ctx->num_input_dev-1].low_latency = my_latency; /*in seconds*/
		audio_ctx->list_devices[audio_ctx->num_input_dev-1].high_latency = my_latency; /*in seconds*/ 
		
	}
}

/*
 * pa_mainloop will call this function when it's ready to tell us
 *  about a source (input).
 *  This callback is pretty much identical to the previous
 *  but it will only print the output devices
 * args:
 *    c - pointer to pulse context
 *    l - pointer to sink info
 *    eol - end of list
 *    data - pointer to user data (audio context)
 *
 * asserts:
 *    none
 *
 * returns: none
 */
static void pa_sinklist_cb(pa_context *c, const pa_sink_info *l, int eol, void *userdata)
{
	//audio_context_t *audio_ctx = (audio_context_t *) data;

    /*
     * If eol is set to a positive number,
     * you're at the end of the list
     */
    if (eol > 0)
        return;

	sink_index++;

	if(verbosity > 0)
	{
		printf("AUDIO: =======[ Output Device #%d ]=======\n", sink_index);
		printf("       Description: %s\n", l->description);
		printf("       Name: %s\n", l->name);
		printf("       Index: %d\n", l->index);
		printf("       Channels: %d\n", l->channel_map.channels);
		printf("       SampleRate: %d\n", l->sample_spec.rate);
		printf("       Latency: %llu (usec)\n", (long long unsigned) l->latency);
		printf("       Configured Latency: %llu (usec)\n", (long long unsigned) l->configured_latency);
		printf("       Card: %d\n", l->card);
		printf("\n");
	}
}

/*
 * iterate the main loop until all devices are listed
 * args:
 *    audio_ctx - pointer to audio context
 *
 * asserts:
 *    audio_ctx is not null
 *
 * returns: error code
 */
static int pa_get_devicelist(audio_context_t *audio_ctx)
{
	/*assertions*/
	assert(audio_ctx != NULL);

	/* Define our pulse audio loop and connection variables */
	pa_mainloop *pa_ml;
	pa_mainloop_api *pa_mlapi;
	pa_operation *pa_op = NULL;
	pa_context *pa_ctx;

	/* We'll need these state variables to keep track of our requests */
    int state = 0;
    int pa_ready = 0;

    /* Create a mainloop API and connection to the default server */
    pa_ml = pa_mainloop_new();
    pa_mlapi = pa_mainloop_get_api(pa_ml);
    pa_ctx = pa_context_new(pa_mlapi, "getDevices");

    /* This function connects to the pulse server */
    if(pa_context_connect(pa_ctx, NULL, 0, NULL) < 0)
    {
		fprintf(stderr,"AUDIO: PULSE - unable to connect to server: pa_context_connect failed\n");
		finish(pa_ctx, pa_ml);
		return -1;
	}

    /*
	 * This function defines a callback so the server will tell us
	 * it's state.
     * Our callback will wait for the state to be ready.
     * The callback will modify the variable to 1 so we know when we
     * have a connection and it's ready.
     * If there's an error, the callback will set pa_ready to 2
	 */
    pa_context_set_state_callback(pa_ctx, pa_state_cb, &pa_ready);

    /*
	 * Now we'll enter into an infinite loop until we get the data
	 * we receive or if there's an error
	 */
    for (;;)
	{
        /*
		 * We can't do anything until PA is ready,
		 * so just iterate the mainloop and continue
		 */
        if (pa_ready == 0)
		{
            pa_mainloop_iterate(pa_ml, 1, NULL);
            continue;
        }
        /* We couldn't get a connection to the server, so exit out */
        if (pa_ready == 2)
		{
            finish(pa_ctx, pa_ml);
            return -1;
        }
        /*
		 * At this point, we're connected to the server and ready
		 * to make requests
		 */
        switch (state)
		{
            /* State 0: we haven't done anything yet */
            case 0:
                /*
				 * This sends an operation to the server.
				 * pa_sinklist_cb is our callback function and a pointer
				 * o our devicelist will be passed to the callback
				 * (audio_ctx) The operation ID is stored in the
                 * pa_op variable
				 */
                pa_op = pa_context_get_sink_info_list(
						  pa_ctx,
                          pa_sinklist_cb,
                          (void *) audio_ctx);

                /* Update state for next iteration through the loop */
                state++;
                break;
            case 1:
                /*
				 * Now we wait for our operation to complete.
				 * When it's complete our pa_output_devicelist is
				 * filled out, and we move along to the next state
				 */
                if (pa_operation_get_state(pa_op) == PA_OPERATION_DONE)
				{
                    pa_operation_unref(pa_op);

                    /*
					 * Now we perform another operation to get the
					 * source(input device) list just like before.
					 * This time we pass a pointer to our input structure
					 */
                    pa_op = pa_context_get_source_info_list(
							  pa_ctx,
                              pa_sourcelist_cb,
                              (void *) audio_ctx);
                    /* Update the state so we know what to do next */
                    state++;
                }
                break;
            case 2:
                if (pa_operation_get_state(pa_op) == PA_OPERATION_DONE)
				{
                    /*
                     * Now we're done,
                     * clean up and disconnect and return
                     */
                    pa_operation_unref(pa_op);
                    finish(pa_ctx, pa_ml);
                    return 0;
                }
                break;
            default:
                /* We should never see this state */
                printf("AUDIO: Pulseaudio in state %d\n", state);
                return -1;
        }
        /*
		 * Iterate the main loop and go again.  The second argument is whether
         * or not the iteration should block until something is ready to be
         * done.  Set it to zero for non-blocking.
		 */
        pa_mainloop_iterate(pa_ml, 1, NULL);
    }

    return 0;
}

/*
 * update pulseaudio latency
 * args:
 *    s - pointer to pa_stream
 *
 * asserts:
 *    none
 *
 * returns:none
 */
static void get_latency(pa_stream *s)
{
	pa_usec_t l;
	int negative;

	pa_stream_get_timing_info(s);

	if (pa_stream_get_latency(s, &l, &negative) != 0)
	{
		fprintf(stderr, "AUDIO: Pulseaudio pa_stream_get_latency() failed\n");
		return;
	}

	//latency = l * (negative?-1:1);
	latency = l; /*can only be negative in monitoring streams*/

	//printf("AUDIO: pulseaudio latency is %0.0f usec    \r", (float)latency);
}

/*
 * audio record callback
 * args:
 *   s - pointer to pa_stream
 *   length - buffer length
 *   data - pointer to user data
 *
 * asserts:
 *   none
 *
 * returns: none
 */
static void stream_request_cb(pa_stream *s, size_t length, void *data)
{

    audio_context_t *audio_ctx = (audio_context_t *) data;

	if(audio_ctx->channels == 0)
	{
		fprintf(stderr, "AUDIO: (pulseaudio) stream_request_cb failed: channels = 0\n");
		return;
	}
	
	if(audio_ctx->samprate == 0)
	{
		fprintf(stderr, "AUDIO: (pulseaudio) stream_request_cb failed: samprate = 0\n");
		return;
	}
	
	uint64_t frame_length = NSEC_PER_SEC / audio_ctx->samprate; /*in nanosec*/
	int64_t ts = 0;
	int64_t buff_ts = 0;
	uint32_t i = 0;

	while (pa_stream_readable_size(s) > 0)
	{
		const void *inputBuffer;
		size_t length;

		/*read from stream*/
		if (pa_stream_peek(s, &inputBuffer, &length) < 0)
		{
			fprintf(stderr, "AUDIO: (pulseaudio) pa_stream_peek() failed\n");
			return;
		}

		if(length == 0)
		{
			fprintf(stderr, "AUDIO: (pulseaudio) empty buffer!\n");
			return; /*buffer is empty*/
		}

		get_latency(s);

		ts = ns_time_monotonic() - (latency * 1000);

		if(audio_ctx->last_ts <= 0)
			audio_ctx->last_ts = ts;


		uint32_t numSamples = (uint32_t) length / sizeof(sample_t);

		const sample_t *rptr = (const sample_t*) inputBuffer;
		sample_t *capture_buff = (sample_t *) audio_ctx->capture_buff;

		int chan = 0;
		/*store capture samples or silence if inputBuffer == NULL (hole)*/
		for( i = 0; i < numSamples; ++i )
		{
			capture_buff[sample_index] = inputBuffer ? *rptr++ : 0;
			sample_index++;

			/*store peak value*/
			if(audio_ctx->capture_buff_level[chan] < capture_buff[sample_index])
				audio_ctx->capture_buff_level[chan] = capture_buff[sample_index];
			chan++;
			if(chan >= audio_ctx->channels)
				chan = 0;

			if(sample_index >= audio_ctx->capture_buff_size)
			{
				buff_ts = ts + ( i / audio_ctx->channels ) * frame_length;

				audio_fill_buffer(audio_ctx, buff_ts);

				/*reset*/
				audio_ctx->capture_buff_level[0] = 0;
				audio_ctx->capture_buff_level[1] = 0;
				sample_index = 0;
			}
		}

		pa_stream_drop(s); /*clean the samples*/
	}

}

/*
 * Iterate the main loop while recording is on.
 * This function runs under it's own thread called by audio_pulse_start
 * args:
 *   data - pointer to user data (audio context)
 *
 * asserts:
 *   data is not null
 *
 * returns: pointer to error code
 */
static void *pulse_read_audio(void *data)
{
    audio_context_t *audio_ctx = (audio_context_t *) data;
	/*assertions*/
	assert(audio_ctx != NULL);

    if(verbosity > 0)
		printf("AUDIO: (pulseaudio) read thread started\n");
    pa_mainloop *pa_ml;
    pa_mainloop_api *pa_mlapi;
    pa_buffer_attr bufattr;
    pa_sample_spec ss;
    pa_stream_flags_t flags = 0;
    int r;
    int pa_ready = 0;

    /* Create a mainloop API and connection to the default server */
    pa_ml = pa_mainloop_new();
    pa_mlapi = pa_mainloop_get_api(pa_ml);
    pa_ctx = pa_context_new(pa_mlapi, "guvcview Pulse API");

    if(pa_context_connect(pa_ctx, NULL, 0, NULL) < 0)
    {
		fprintf(stderr,"AUDIO: PULSE - unable to connect to server: pa_context_connect failed\n");
		finish(pa_ctx, pa_ml);
		return ((void *) -1);
	}

    /*
	 * This function defines a callback so the server will tell us it's state.
     * Our callback will wait for the state to be ready.  The callback will
     * modify the variable to 1 so we know when we have a connection and it's
     * ready.
     * If there's an error, the callback will set pa_ready to 2
	 */
    pa_context_set_state_callback(pa_ctx, pa_state_cb, &pa_ready);

    /*
     * This function defines a time event callback (called every TIME_EVENT_USEC)
     */
    //pa_context_rttime_new(pa_ctx, pa_rtclock_now() + TIME_EVENT_USEC, time_event_callback, NULL);

    /*
	 * We can't do anything until PA is ready, so just iterate the mainloop
     * and continue
	 */
    while (pa_ready == 0)
    {
        pa_mainloop_iterate(pa_ml, 1, NULL);
    }
    if (pa_ready == 2)
    {
        finish(pa_ctx, pa_ml);
        return ((void *) -1);
    }

	/* set the sample spec (frame rate, channels and format) */
    ss.rate = audio_ctx->samprate;
    ss.channels = audio_ctx->channels;
    ss.format = PA_SAMPLE_FLOAT32LE; /*for PCM -> PA_SAMPLE_S16LE*/

    recordstream = pa_stream_new(pa_ctx, "Record", &ss, NULL);
    if (!recordstream)
        fprintf(stderr, "AUDIO: (pulseaudio) pa_stream_new failed (chan:%d rate:%d)\n", 
			ss.channels, ss.rate);

    /* define the callbacks */
    pa_stream_set_read_callback(recordstream, stream_request_cb, (void *) audio_ctx);

	// Set properties of the record buffer
    pa_zero(bufattr);
    /* optimal value for all is (uint32_t)-1   ~= 2 sec */
    bufattr.maxlength = (uint32_t) -1;
    bufattr.prebuf = (uint32_t) -1;
    bufattr.minreq = (uint32_t) -1;

    if (audio_ctx->latency > 0)
    {
      bufattr.fragsize = bufattr.tlength = pa_usec_to_bytes((audio_ctx->latency * 1000) * PA_USEC_PER_MSEC, &ss);
      flags |= PA_STREAM_ADJUST_LATENCY;
    }
    else
      bufattr.fragsize = bufattr.tlength = (uint32_t) -1;

	flags |= PA_STREAM_INTERPOLATE_TIMING;
    flags |= PA_STREAM_AUTO_TIMING_UPDATE;

    char * dev = audio_ctx->list_devices[audio_ctx->device].name;
    if(verbosity > 0)
		printf("AUDIO: (pulseaudio) connecting to device %s\n\t (channels %d rate %d)\n",
			dev, ss.channels, ss.rate);
    r = pa_stream_connect_record(recordstream, dev, &bufattr, flags);
    if (r < 0)
    {
        fprintf(stderr, "AUDIO: (pulseaudio) skip latency adjustment\n");
        /*
         * Old pulse audio servers don't like the ADJUST_LATENCY flag,
		 * so retry without that
		 */
        r = pa_stream_connect_record(recordstream, dev, &bufattr,
                                     PA_STREAM_INTERPOLATE_TIMING|
                                     PA_STREAM_AUTO_TIMING_UPDATE);
    }
    if (r < 0)
    {
        fprintf(stderr, "AUDIO: (pulseaudio) pa_stream_connect_record failed\n");
        finish(pa_ctx, pa_ml);
        return ((void *) -1);
    }

    get_latency(recordstream);

    /*
     * Iterate the main loop while streaming.  The second argument is whether
     * or not the iteration should block until something is ready to be
     * done.  Set it to zero for non-blocking.
     */
    while (audio_ctx->stream_flag == AUDIO_STRM_ON)
    {
        pa_mainloop_iterate(pa_ml, 1, NULL);
    }

	if(verbosity > 0)
		printf("AUDIO: (pulseaudio) stream terminated(%i)\n", audio_ctx->stream_flag);

    pa_stream_disconnect (recordstream);
    pa_stream_unref (recordstream);
    finish(pa_ctx, pa_ml);
    return ((void *) 0);
}

/*
 * init pulseaudio api
 * args:
 *    audio_ctx - pointer to audio context data
 *
 * asserts:
 *    audio_ctx is not null
 *
 * returns: error code (0 = E_OK)
 */
int audio_init_pulseaudio(audio_context_t *audio_ctx)
{
	/*assertions*/
	assert(audio_ctx != NULL);

	if (pa_get_devicelist(audio_ctx) < 0)
	{
		fprintf(stderr, "AUDIO: Pulseaudio failed to get audio device list from PULSE server\n");
		return -1;
	}

	audio_ctx->api = AUDIO_PULSE;

	return 0;
}

/*
 * set audio device
 * args:
 *   audio_ctx - pointer to audio context data
 *   index - device index to set
 *
 * asserts:
 *   audio_ctx is not null
 *
 * returns: none
 */
void audio_set_pulseaudio_device(audio_context_t *audio_ctx, int index)
{
	/*assertions*/
	assert(audio_ctx != NULL);
	
	if(index >= audio_ctx->num_input_dev)
		audio_ctx->device = audio_ctx->num_input_dev - 1;
	else if(index >= 0 )
		audio_ctx->device = index;
	
	if(verbosity > 0)
		printf("AUDIO: Pulseaudio device changed to %i\n", audio_ctx->device);
	
	audio_ctx->latency = audio_ctx->list_devices[audio_ctx->device].high_latency;
	
	audio_ctx->channels = audio_ctx->list_devices[audio_ctx->device].channels;
	if(audio_ctx->channels > 2)
		audio_ctx->channels = 2;/*limit it to stereo input*/
	audio_ctx->samprate = audio_ctx->list_devices[audio_ctx->device].samprate;
}

/*
 * Launch the main loop iteration thread
 * args:
 *   audio_ctx - pointer to audio context data
 *
 * asserts:
 *   audio_ctx is not null
 *
 * returns: error code
 */
int audio_start_pulseaudio(audio_context_t *audio_ctx)
{
	/*assertions*/
	assert(audio_ctx != NULL);

	audio_ctx->stream_flag = AUDIO_STRM_ON;

    /* start audio capture thread */
    if(__THREAD_CREATE(&my_read_thread, pulse_read_audio, (void *) audio_ctx))
    {
		fprintf(stderr, "AUDIO: (pulseaudio) read thread creation failed\n");
		audio_ctx->stream_flag = AUDIO_STRM_OFF;
        return (-1);
    }

    return 0;
}

/*
 * stop and join the main loop iteration thread
 * args:
 *   audio_ctx - pointer to audio context data
 *
 * asserts:
 *   audio_ctx is not null
 *
 * returns: error code
 */
int audio_stop_pulseaudio(audio_context_t *audio_ctx)
{
	/*assertions*/
	assert(audio_ctx != NULL);

	audio_ctx->stream_flag = AUDIO_STRM_OFF;

	__THREAD_JOIN( my_read_thread );

	if(verbosity > 0)
		printf("AUDIO: (pulseaudio) read thread joined\n");

    return 0;
}

/*
 * close and clean audio context for pulseaudio api
 * args:
 *   audio_ctx - pointer to audio context data
 *
 * asserts:
 *   none
 *
 * returns: none
 */
void audio_close_pulseaudio(audio_context_t *audio_ctx)
{
	if(audio_ctx == NULL)
		return;

	if(audio_ctx->stream_flag == AUDIO_STRM_ON)
		audio_stop_pulseaudio(audio_ctx);

	if(audio_ctx->list_devices != NULL)
		free(audio_ctx->list_devices);
	audio_ctx->list_devices = NULL;

	if(audio_ctx->capture_buff)
		free(audio_ctx->capture_buff);

	free(audio_ctx);
}
