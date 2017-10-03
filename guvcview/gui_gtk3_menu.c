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

#include "gui_gtk3.h"
#include "gui_gtk3_callbacks.h"
#include "gui.h"
/*add this last to avoid redefining _() and N_()*/
#include "gview.h"


extern int debug_level;
extern int is_control_panel;

/*
 * attach top menu widget
 * args:
 *   parent - menu parent widget
 *
 * asserts:
 *   parent is not null
 *
 * returns: error code (0 -OK)
 */
int gui_attach_gtk3_menu(GtkWidget *parent)
{
	/*assertions*/
	assert(parent != NULL);

	GtkWidget *menubar = gtk_menu_bar_new();
	gtk_menu_bar_set_pack_direction(GTK_MENU_BAR(menubar), GTK_PACK_DIRECTION_LTR);

	GtkWidget *controls_menu = gtk_menu_new();
	GtkWidget *controls_top = gtk_menu_item_new_with_label(_("Settings"));
	GtkWidget *controls_load = gtk_menu_item_new_with_label(_("Load Profile"));
	GtkWidget *controls_save = gtk_menu_item_new_with_label(_("Save Profile"));
	GtkWidget *controls_default = gtk_menu_item_new_with_label(_("Hardware Defaults"));

	gtk_widget_show (controls_top);
	gtk_widget_show (controls_load);
	gtk_widget_show (controls_save);
	gtk_widget_show (controls_default);

	GtkWidget *camera_button_menu = gtk_menu_new();
	GtkWidget *camera_button_top = gtk_menu_item_new_with_label(_("Camera Button"));

	gtk_menu_item_set_submenu(GTK_MENU_ITEM(camera_button_top), camera_button_menu);

	GSList *camera_button_group = NULL;
	GtkWidget *def_image = gtk_radio_menu_item_new_with_label(camera_button_group, _("Capture Image"));
	g_object_set_data (G_OBJECT (def_image), "camera_default", GINT_TO_POINTER(0));
	gtk_menu_shell_append(GTK_MENU_SHELL(camera_button_menu), def_image);

	camera_button_group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (def_image));
	GtkWidget *def_video = gtk_radio_menu_item_new_with_label(camera_button_group, _("Capture Video"));
	g_object_set_data (G_OBJECT (def_video), "camera_default", GINT_TO_POINTER(1));
	gtk_menu_shell_append(GTK_MENU_SHELL(camera_button_menu), def_video);

	gtk_widget_show (camera_button_top);
	gtk_widget_show (camera_button_menu);
	gtk_widget_show (def_image);
	gtk_widget_show (def_video);

	/*default camera button action*/
	if (get_default_camera_button_action() == 0)
		gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (def_image), TRUE);
	else
		gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (def_video), TRUE);

	g_signal_connect (GTK_RADIO_MENU_ITEM(def_image), "toggled",
		G_CALLBACK (camera_button_menu_changed), NULL);
	g_signal_connect (GTK_RADIO_MENU_ITEM(def_video), "toggled",
		G_CALLBACK (camera_button_menu_changed), NULL);

	/* profile events*/
	g_object_set_data (G_OBJECT (controls_save), "profile_dialog", GINT_TO_POINTER(1));
	g_signal_connect (GTK_MENU_ITEM(controls_save), "activate",
		G_CALLBACK (controls_profile_clicked), NULL);
	g_object_set_data (G_OBJECT (controls_load), "profile_dialog", GINT_TO_POINTER(0));
	g_signal_connect (GTK_MENU_ITEM(controls_load), "activate",
		G_CALLBACK (controls_profile_clicked), NULL);
	g_signal_connect (GTK_MENU_ITEM(controls_default), "activate",
		G_CALLBACK (control_defaults_clicked), NULL);

	gtk_menu_item_set_submenu(GTK_MENU_ITEM(controls_top), controls_menu);
	gtk_menu_shell_append(GTK_MENU_SHELL(controls_menu), controls_load);
	gtk_menu_shell_append(GTK_MENU_SHELL(controls_menu), controls_save);
	gtk_menu_shell_append(GTK_MENU_SHELL(controls_menu), controls_default);
	gtk_menu_shell_append(GTK_MENU_SHELL(controls_menu), camera_button_top);
	gtk_menu_shell_append(GTK_MENU_SHELL(menubar), controls_top);

	/*control panel mode exclusions */
	if(!is_control_panel)
	{
		/*photo menu*/
		GtkWidget *photo_menu = gtk_menu_new();

		GtkWidget *menu_photo_top = gtk_menu_item_new_with_label(_("Photo"));
		GtkWidget *photo_file = gtk_menu_item_new_with_label(_("File"));
		GtkWidget *photo_sufix = gtk_check_menu_item_new_with_label(_("Increment Filename"));

		gtk_widget_show (menu_photo_top);
		gtk_widget_show (photo_file);
		gtk_widget_show (photo_sufix);

		g_signal_connect (GTK_MENU_ITEM(photo_file), "activate",
			G_CALLBACK (photo_file_clicked), NULL);

		/** add callback to Append sufix */
		gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (photo_sufix), (get_photo_sufix_flag() > 0));
		g_signal_connect (GTK_CHECK_MENU_ITEM(photo_sufix), "toggled",
			G_CALLBACK (photo_sufix_toggled), NULL);

		gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_photo_top), photo_menu);
		gtk_menu_shell_append(GTK_MENU_SHELL(photo_menu), photo_file);
		gtk_menu_shell_append(GTK_MENU_SHELL(photo_menu), photo_sufix);
		gtk_menu_shell_append(GTK_MENU_SHELL(menubar), menu_photo_top);

		/*video menu*/
		GtkWidget *video_menu = gtk_menu_new();

		GtkWidget *menu_video_top = gtk_menu_item_new_with_label(_("Video"));
		GtkWidget *video_file = gtk_menu_item_new_with_label(_("File"));
		GtkWidget *video_sufix = gtk_check_menu_item_new_with_label(_("Increment Filename"));

		gtk_widget_show (menu_video_top);
		gtk_widget_show (video_file);
		gtk_widget_show (video_sufix);

		g_signal_connect (GTK_MENU_ITEM(video_file), "activate",
			G_CALLBACK (video_file_clicked), NULL);

		/** add callback to Append sufix */
		gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (video_sufix), get_video_sufix_flag() > 0);
		g_signal_connect (GTK_CHECK_MENU_ITEM(video_sufix), "toggled",
			G_CALLBACK (video_sufix_toggled), NULL);

		GtkWidget *video_codec_menu = gtk_menu_new();
		GtkWidget *video_codec_top = gtk_menu_item_new_with_label(_("Video Codec"));
		gtk_widget_show (video_codec_top);
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(video_codec_top), video_codec_menu);
		/*Add codecs to submenu*/
		GSList *vgroup = NULL;
		int num_vcodecs = encoder_get_valid_video_codecs();
		int vcodec_ind =0;
		for (vcodec_ind =0; vcodec_ind < num_vcodecs; vcodec_ind++)
		{
			GtkWidget *item = gtk_radio_menu_item_new_with_label(
				vgroup,
				gettext(encoder_get_video_codec_description(vcodec_ind)));
			if (vcodec_ind == get_video_codec_ind())
			{
				gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item), TRUE);
			}
			/*NOTE: GSList indexes (g_slist_index) are in reverse order: last inserted has index 0*/
			vgroup = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (item));

			gtk_widget_show (item);
			gtk_menu_shell_append(GTK_MENU_SHELL(video_codec_menu), item);

			g_signal_connect (GTK_RADIO_MENU_ITEM(item), "toggled",
                G_CALLBACK (video_codec_changed), vgroup);
		}
		set_video_codec_group_list_gtk3(vgroup);

		GtkWidget *video_codec_prop =  gtk_menu_item_new_with_label(_("Video Codec Properties"));
		gtk_widget_show (video_codec_prop);
		g_signal_connect (GTK_MENU_ITEM(video_codec_prop), "activate",
			G_CALLBACK (encoder_video_properties), NULL);

		GtkWidget *audio_codec_menu = gtk_menu_new();
		GtkWidget *audio_codec_top = gtk_menu_item_new_with_label(_("Audio Codec"));
		gtk_widget_show (audio_codec_top);
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(audio_codec_top), audio_codec_menu);
		/*Add codecs to submenu*/
		GSList *agroup = NULL;
		int num_acodecs = encoder_get_valid_audio_codecs();
		int acodec_ind = 0;
		for (acodec_ind = 0; acodec_ind < num_acodecs; acodec_ind++)
		{
			GtkWidget *item = gtk_radio_menu_item_new_with_label(
				agroup,
				gettext(encoder_get_audio_codec_description(acodec_ind)));
			if (acodec_ind == get_audio_codec_ind())
			{
				gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item), TRUE);
			}
			/*NOTE: GSList indexes (g_slist_index) are in reverse order: last inserted has index 0*/
			agroup = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (item));

			gtk_widget_show (item);
			gtk_menu_shell_append(GTK_MENU_SHELL(audio_codec_menu), item);

			g_signal_connect (GTK_RADIO_MENU_ITEM(item), "toggled",
                G_CALLBACK (audio_codec_changed), agroup);
		}
		set_audio_codec_group_list_gtk3(agroup);

		GtkWidget *audio_codec_prop =  gtk_menu_item_new_with_label(_("Audio Codec Properties"));
		gtk_widget_show (audio_codec_prop);
		g_signal_connect (GTK_MENU_ITEM(audio_codec_prop), "activate",
			G_CALLBACK (encoder_audio_properties), NULL);

		gtk_menu_item_set_submenu(GTK_MENU_ITEM(video_codec_top), video_codec_menu);
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(audio_codec_top), audio_codec_menu);
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_video_top), video_menu);
		gtk_menu_shell_append(GTK_MENU_SHELL(video_menu), video_file);
		gtk_menu_shell_append(GTK_MENU_SHELL(video_menu), video_sufix);
		gtk_menu_shell_append(GTK_MENU_SHELL(video_menu), video_codec_top);
		gtk_menu_shell_append(GTK_MENU_SHELL(video_menu), video_codec_prop);
		gtk_menu_shell_append(GTK_MENU_SHELL(video_menu), audio_codec_top);
		gtk_menu_shell_append(GTK_MENU_SHELL(video_menu), audio_codec_prop);
		gtk_menu_shell_append(GTK_MENU_SHELL(menubar), menu_video_top);
	}

	/*show the menu*/
	gtk_widget_show (menubar);
	//gtk_container_set_resize_mode (GTK_CONTAINER(menubar), GTK_RESIZE_PARENT);

	/* Attach the menu to parent container*/
	gtk_box_pack_start(GTK_BOX(parent), menubar, FALSE, TRUE, 2);

	return 0;
}
