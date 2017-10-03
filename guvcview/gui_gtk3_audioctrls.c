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
#include "gui_gtk3.h"
#include "gui_gtk3_callbacks.h"
#include "gui.h"
#include "gviewaudio.h"
#include "video_capture.h"
/*add this last to avoid redefining _() and N_()*/
#include "gview.h"

extern int debug_level;

static audio_widgets_t my_audio_widgets =
{
	.api = NULL,
	.device = NULL,
	.channels = NULL,
	.samprate = NULL,
	.latency = NULL,
};


/*
 * attach audio controls tab widget
 * args:
 *   parent - tab parent widget
 *
 * asserts:
 *   parent is not null
 *
 * returns: error code (0 -OK)
 */
int gui_attach_gtk3_audioctrls(GtkWidget *parent)
{
	/*assertions*/
	assert(parent != NULL);

	if(debug_level > 1)
		printf("GUVCVIEW: attaching audio controls\n");

	int line = 0;

	/*get the current audio context*/
	audio_context_t *audio_ctx = get_audio_context();

	GtkWidget *audio_controls_grid = gtk_grid_new();
	gtk_widget_show (audio_controls_grid);

	gtk_grid_set_column_homogeneous (GTK_GRID(audio_controls_grid), FALSE);
	gtk_widget_set_hexpand (audio_controls_grid, TRUE);
	gtk_widget_set_halign (audio_controls_grid, GTK_ALIGN_FILL);

	gtk_grid_set_row_spacing (GTK_GRID(audio_controls_grid), 4);
	gtk_grid_set_column_spacing (GTK_GRID (audio_controls_grid), 4);
	gtk_container_set_border_width (GTK_CONTAINER (audio_controls_grid), 2);

	/*API*/
	line++;

	GtkWidget *label_SndAPI = gtk_label_new(_("Audio API:"));
#if GTK_VER_AT_LEAST(3,15)
	gtk_label_set_xalign(GTK_LABEL(label_SndAPI), 1);
	gtk_label_set_yalign(GTK_LABEL(label_SndAPI), 0.5);
#else
	gtk_misc_set_alignment (GTK_MISC (label_SndAPI), 1, 0.5);
#endif
	gtk_grid_attach (GTK_GRID(audio_controls_grid), label_SndAPI, 0, line, 1, 1);
	gtk_widget_show (label_SndAPI);

	my_audio_widgets.api = gtk_combo_box_text_new ();
	gtk_widget_set_halign (my_audio_widgets.api, GTK_ALIGN_FILL);
	gtk_widget_set_hexpand (my_audio_widgets.api, TRUE);
	gtk_grid_attach(GTK_GRID(audio_controls_grid), my_audio_widgets.api, 1, line, 1, 1);

	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(my_audio_widgets.api),_("NO SOUND"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(my_audio_widgets.api),_("PORTAUDIO"));

#if HAS_PULSEAUDIO
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(my_audio_widgets.api),_("PULSEAUDIO"));
#endif
	gtk_widget_show (my_audio_widgets.api);

	int api = AUDIO_NONE;
	if(audio_ctx != NULL)
		api = audio_get_api(audio_ctx);

	gtk_combo_box_set_active(GTK_COMBO_BOX(my_audio_widgets.api), api);

	gtk_widget_set_sensitive (my_audio_widgets.api, TRUE);
	g_signal_connect (GTK_COMBO_BOX_TEXT(my_audio_widgets.api), "changed",
		G_CALLBACK (audio_api_changed), &my_audio_widgets);

	/*devices*/
	line++;

	GtkWidget *label_SndDevice = gtk_label_new(_("Input Device:"));
#if GTK_VER_AT_LEAST(3,15)
	gtk_label_set_xalign(GTK_LABEL(label_SndDevice), 1);
	gtk_label_set_yalign(GTK_LABEL(label_SndDevice), 0.5);
#else
	gtk_misc_set_alignment (GTK_MISC (label_SndDevice), 1, 0.5);
#endif

	gtk_grid_attach (GTK_GRID(audio_controls_grid), label_SndDevice, 0, line, 1, 1);
	gtk_widget_show (label_SndDevice);

	my_audio_widgets.device = gtk_combo_box_text_new ();
	gtk_widget_set_halign (my_audio_widgets.device, GTK_ALIGN_FILL);
	gtk_widget_set_hexpand (my_audio_widgets.device, TRUE);
	gtk_grid_attach(GTK_GRID(audio_controls_grid), my_audio_widgets.device, 1, line, 1, 1);
	gtk_widget_show (my_audio_widgets.device);

	if(audio_ctx != NULL)
	{
		int i = 0;
		for(i = 0; i < audio_get_num_inp_devices(audio_ctx); ++i)
		{
			gtk_combo_box_text_append_text(
				GTK_COMBO_BOX_TEXT(my_audio_widgets.device),
				audio_get_device(audio_ctx,i)->description);
		}

		gtk_widget_set_sensitive (my_audio_widgets.device, TRUE);
		gtk_combo_box_set_active(GTK_COMBO_BOX(my_audio_widgets.device), audio_get_device_index(audio_ctx));
	}
	else
		gtk_widget_set_sensitive (my_audio_widgets.device, FALSE);

	g_signal_connect (GTK_COMBO_BOX_TEXT(my_audio_widgets.device), "changed",
		G_CALLBACK (audio_device_changed), &my_audio_widgets);


	/*sample rate*/
	line++;

	GtkWidget *label_SndSampRate = gtk_label_new(_("Sample Rate:"));
#if GTK_VER_AT_LEAST(3,15)
	gtk_label_set_xalign(GTK_LABEL(label_SndSampRate), 1);
	gtk_label_set_yalign(GTK_LABEL(label_SndSampRate), 0.5);
#else
	gtk_misc_set_alignment (GTK_MISC (label_SndSampRate), 1, 0.5);
#endif


	gtk_grid_attach (GTK_GRID(audio_controls_grid), label_SndSampRate, 0, line, 1, 1);
	gtk_widget_show (label_SndSampRate);

	my_audio_widgets.samprate = gtk_combo_box_text_new ();
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(my_audio_widgets.samprate),_("Dev. Default"));

	/* add some standard sample rates:
	 *  96000, 88200, 64000, 48000, 44100,
	 *  32000, 24000, 22050, 16000, 12000,
	 *  11025, 8000, 7350
	 */
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(my_audio_widgets.samprate), "7350");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(my_audio_widgets.samprate), "8000");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(my_audio_widgets.samprate), "11025");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(my_audio_widgets.samprate), "12000");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(my_audio_widgets.samprate), "16000");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(my_audio_widgets.samprate), "22050");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(my_audio_widgets.samprate), "24000");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(my_audio_widgets.samprate), "32000");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(my_audio_widgets.samprate), "44100");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(my_audio_widgets.samprate), "48000");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(my_audio_widgets.samprate), "64000");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(my_audio_widgets.samprate), "88200");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(my_audio_widgets.samprate), "96000");

	gtk_widget_set_halign (my_audio_widgets.samprate, GTK_ALIGN_FILL);
	gtk_widget_set_hexpand (my_audio_widgets.samprate, TRUE);
	gtk_grid_attach(GTK_GRID(audio_controls_grid), my_audio_widgets.samprate, 1, line, 1, 1);
	gtk_widget_show (my_audio_widgets.samprate);

	gtk_combo_box_set_active(GTK_COMBO_BOX(my_audio_widgets.samprate), 0); /*device default*/

	if(audio_ctx != NULL)
		gtk_widget_set_sensitive (my_audio_widgets.samprate, TRUE);
	else
		gtk_widget_set_sensitive (my_audio_widgets.samprate, FALSE);

	g_signal_connect (GTK_COMBO_BOX_TEXT(my_audio_widgets.samprate), "changed",
		G_CALLBACK (audio_samplerate_changed), &my_audio_widgets);

	/*channels*/
	line++;

	GtkWidget *label_SndNumChan = gtk_label_new(_("Channels:"));
#if GTK_VER_AT_LEAST(3,15)
	gtk_label_set_xalign(GTK_LABEL(label_SndNumChan), 1);
	gtk_label_set_yalign(GTK_LABEL(label_SndNumChan), 0.5);
#else
	gtk_misc_set_alignment (GTK_MISC (label_SndNumChan), 1, 0.5);
#endif

	gtk_grid_attach (GTK_GRID(audio_controls_grid), label_SndNumChan, 0, line, 1, 1);
	gtk_widget_show (label_SndNumChan);

	my_audio_widgets.channels = gtk_combo_box_text_new ();
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(my_audio_widgets.channels),_("Dev. Default"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(my_audio_widgets.channels),_("1 - mono"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(my_audio_widgets.channels),_("2 - stereo"));

	gtk_widget_set_halign (my_audio_widgets.channels, GTK_ALIGN_FILL);
	gtk_widget_set_hexpand (my_audio_widgets.channels, TRUE);
	gtk_grid_attach(GTK_GRID(audio_controls_grid), my_audio_widgets.channels, 1, line, 1, 1);
	gtk_widget_show (my_audio_widgets.channels);

	gtk_combo_box_set_active(GTK_COMBO_BOX(my_audio_widgets.channels), 0); /*device default*/

	if(audio_ctx != NULL)
		gtk_widget_set_sensitive (my_audio_widgets.channels, TRUE);
	else
		gtk_widget_set_sensitive (my_audio_widgets.channels, FALSE);

	g_signal_connect (GTK_COMBO_BOX_TEXT(my_audio_widgets.channels), "changed",
		G_CALLBACK (audio_channels_changed), &my_audio_widgets);

	/*latency*/
	line++;

	GtkWidget *label_Latency = gtk_label_new(_("Latency:"));
#if GTK_VER_AT_LEAST(3,15)
	gtk_label_set_xalign(GTK_LABEL(label_Latency), 1);
	gtk_label_set_yalign(GTK_LABEL(label_Latency), 0.5);
#else
	gtk_misc_set_alignment (GTK_MISC (label_Latency), 1, 0.5);
#endif

	gtk_grid_attach (GTK_GRID(audio_controls_grid), label_Latency, 0, line, 1, 1);
	gtk_widget_show (label_Latency);
	
	double latency = 0.0;
	if(audio_ctx != NULL)
		latency = audio_get_latency(audio_ctx);
	
	if(debug_level > 2)
		printf("GUVCVIEW: audio latency is set to %f\n", latency);
	
	GtkAdjustment *adjustment =  gtk_adjustment_new (
		latency,
		0.001,
		0.1,
		0.001,
		0.01,
		0);
								
	my_audio_widgets.latency = gtk_scale_new (GTK_ORIENTATION_HORIZONTAL, adjustment);
	gtk_scale_set_digits(GTK_SCALE(my_audio_widgets.latency), 3);
	gtk_scale_set_value_pos(GTK_SCALE(my_audio_widgets.latency), GTK_POS_RIGHT);
	gtk_widget_set_halign (my_audio_widgets.latency, GTK_ALIGN_FILL);
	gtk_widget_set_hexpand (my_audio_widgets.latency, TRUE);
	gtk_grid_attach(GTK_GRID(audio_controls_grid), my_audio_widgets.latency, 1, line, 1, 1);
	gtk_widget_show (my_audio_widgets.latency);

	if(audio_ctx != NULL)
		gtk_widget_set_sensitive (my_audio_widgets.latency, TRUE);
	else
		gtk_widget_set_sensitive (my_audio_widgets.latency, FALSE);

	g_signal_connect (GTK_SCALE(my_audio_widgets.latency), "value-changed",
		G_CALLBACK (audio_latency_changed), &my_audio_widgets);

	/* ----- Filter controls -----*/
	line++;
	GtkWidget *label_audioFilters = gtk_label_new(_("---- Audio Filters ----"));
#if GTK_VER_AT_LEAST(3,15)
	gtk_label_set_xalign(GTK_LABEL(label_audioFilters), 0.5);
	gtk_label_set_yalign(GTK_LABEL(label_audioFilters), 0.5);
#else
	gtk_misc_set_alignment (GTK_MISC (label_audioFilters), 0.5, 0.5);
#endif

	gtk_grid_attach (GTK_GRID(audio_controls_grid), label_audioFilters, 0, line, 3, 1);
	gtk_widget_show (label_audioFilters);

	/*filters grid*/
	line++;
	GtkWidget *table_filt = gtk_grid_new();
	gtk_grid_set_row_spacing (GTK_GRID (table_filt), 4);
	gtk_grid_set_column_spacing (GTK_GRID (table_filt), 4);
	gtk_container_set_border_width (GTK_CONTAINER (table_filt), 4);
	gtk_widget_set_size_request (table_filt, -1, -1);

	gtk_widget_set_halign (table_filt, GTK_ALIGN_FILL);
	gtk_widget_set_hexpand (table_filt, TRUE);
	gtk_grid_attach (GTK_GRID(audio_controls_grid), table_filt, 0, line, 3, 1);
	gtk_widget_show (table_filt);

	/* Echo FX */
	GtkWidget *FiltEchoEnable = gtk_check_button_new_with_label (_(" Echo"));
	g_object_set_data (G_OBJECT (FiltEchoEnable), "filt_info", GINT_TO_POINTER(AUDIO_FX_ECHO));
	gtk_widget_set_halign (FiltEchoEnable, GTK_ALIGN_FILL);
	gtk_widget_set_hexpand (FiltEchoEnable, TRUE);
	gtk_grid_attach(GTK_GRID(table_filt), FiltEchoEnable, 0, 0, 1, 1);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(FiltEchoEnable),
		(get_audio_fx_mask() & AUDIO_FX_ECHO) > 0);
	gtk_widget_show (FiltEchoEnable);
	g_signal_connect (GTK_CHECK_BUTTON(FiltEchoEnable), "toggled",
		G_CALLBACK (audio_fx_filter_changed), NULL);

	/* Reverb FX */
	GtkWidget *FiltReverbEnable = gtk_check_button_new_with_label (_(" Reverb"));
	g_object_set_data (G_OBJECT (FiltReverbEnable), "filt_info", GINT_TO_POINTER(AUDIO_FX_REVERB));
	gtk_widget_set_halign (FiltReverbEnable, GTK_ALIGN_FILL);
	gtk_widget_set_hexpand (FiltReverbEnable, TRUE);
	gtk_grid_attach(GTK_GRID(table_filt), FiltReverbEnable, 1, 0, 1, 1);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(FiltReverbEnable),
		(get_audio_fx_mask() & AUDIO_FX_REVERB) > 0);
	gtk_widget_show (FiltReverbEnable);
	g_signal_connect (GTK_CHECK_BUTTON(FiltReverbEnable), "toggled",
		G_CALLBACK (audio_fx_filter_changed), NULL);

	/* Fuzz FX */
	GtkWidget *FiltFuzzEnable = gtk_check_button_new_with_label (_(" Fuzz"));
	g_object_set_data (G_OBJECT (FiltFuzzEnable), "filt_info", GINT_TO_POINTER(AUDIO_FX_FUZZ));
	gtk_widget_set_halign (FiltFuzzEnable, GTK_ALIGN_FILL);
	gtk_widget_set_hexpand (FiltFuzzEnable, TRUE);
	gtk_grid_attach(GTK_GRID(table_filt), FiltFuzzEnable, 2, 0, 1, 1);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(FiltFuzzEnable),
		(get_audio_fx_mask() & AUDIO_FX_FUZZ) > 0);
	gtk_widget_show (FiltFuzzEnable);
	g_signal_connect (GTK_CHECK_BUTTON(FiltFuzzEnable), "toggled",
		G_CALLBACK (audio_fx_filter_changed), NULL);

	/* WahWah FX */
	GtkWidget *FiltWahEnable = gtk_check_button_new_with_label (_(" WahWah"));
	g_object_set_data (G_OBJECT (FiltWahEnable), "filt_info", GINT_TO_POINTER(AUDIO_FX_WAHWAH));
	gtk_widget_set_halign (FiltWahEnable, GTK_ALIGN_FILL);
	gtk_widget_set_hexpand (FiltWahEnable, TRUE);
	gtk_grid_attach(GTK_GRID(table_filt), FiltWahEnable, 3, 0, 1, 1);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(FiltWahEnable),
		(get_audio_fx_mask() & AUDIO_FX_WAHWAH) > 0);
	gtk_widget_show (FiltWahEnable);
	g_signal_connect (GTK_CHECK_BUTTON(FiltWahEnable), "toggled",
		G_CALLBACK (audio_fx_filter_changed), NULL);

	/* Ducky FX */
	GtkWidget *FiltDuckyEnable = gtk_check_button_new_with_label (_(" Ducky"));
	g_object_set_data (G_OBJECT (FiltDuckyEnable), "filt_info", GINT_TO_POINTER(AUDIO_FX_DUCKY));
	gtk_widget_set_halign (FiltDuckyEnable, GTK_ALIGN_FILL);
	gtk_widget_set_hexpand (FiltDuckyEnable, TRUE);
	gtk_grid_attach(GTK_GRID(table_filt), FiltDuckyEnable, 4, 0, 1, 1);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(FiltDuckyEnable),
		(get_audio_fx_mask() & AUDIO_FX_DUCKY) > 0);
	gtk_widget_show (FiltDuckyEnable);
	g_signal_connect (GTK_CHECK_BUTTON(FiltDuckyEnable), "toggled",
		G_CALLBACK (audio_fx_filter_changed), NULL);

	/*add control grid to parent container*/
	gtk_container_add(GTK_CONTAINER(parent), audio_controls_grid);

	return 0;
}
