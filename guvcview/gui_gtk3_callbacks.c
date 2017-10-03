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

#include <gtk/gtk.h>
#include <glib.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
/* support for internationalization - i18n */
#include <glib/gi18n.h>

#include "gviewv4l2core.h"
#include "video_capture.h"
#include "gviewencoder.h"
#include "gviewrender.h"
#include "gui.h"
#include "gui_gtk3.h"
#include "core_io.h"
#include "config.h"

extern int debug_level;

static int entry_has_focus = 0;

/*
 * delete event (close window)
 * args:
 *   widget - pointer to event widget
 *   event - pointer to event data
 *   data - pointer to user data
 *
 * asserts:
 *   none
 *
 * returns: error code
 */
int delete_event (GtkWidget *widget, GdkEventConfigure *event, void *data)
{
	/* Terminate program */
	quit_callback(NULL);
	return 0;
}

/*
 * quit button clicked event
 * args:
 *    widget - pointer to widget that caused the event
 *    data - pointer to user data
 *
 * asserts:
 *    none
 *
 * returns: none
 */
void quit_button_clicked(GtkWidget *widget, void *data)
{
	/* Terminate program */
	quit_callback(NULL);
}

/*
 * camera_button_menu toggled event
 * args:
 *   widget - pointer to event widget
 *   data - pointer to user data
 *
 * asserts:
 *   none
 *
 * returns: none
 */
void camera_button_menu_changed (GtkWidget *item, void *data)
{
	int flag = GPOINTER_TO_INT(g_object_get_data (G_OBJECT (item), "camera_default"));

	if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(item)))
		set_default_camera_button_action(flag);
}

/*
 * control default clicked event
 * args:
 *   widget - pointer to event widget
 *   data - pointer to user data
 *
 * asserts:
 *   none
 *
 * returns: none
 */
void control_defaults_clicked (GtkWidget *item, void *data)
{
    v4l2core_set_control_defaults(get_v4l2_device_handler());

    gui_gtk3_update_controls_state();
}

/*
 * called from profile format combo in file dialog
 * args:
 *    chooser - format combo that caused the event
 *    file_dialog - chooser parent
 *
 * asserts:
 *    none
 *
 * returns: none
 */
static void profile_update_extension (GtkComboBox *chooser, GtkWidget *file_dialog)
{
	int format = gtk_combo_box_get_active (chooser);

	GtkFileFilter *filter = gtk_file_filter_new();

	switch(format)
	{
		case 1:
			gtk_file_filter_add_pattern(filter, "*.*");
			break;

		default:
		case 0:
			gtk_file_filter_add_pattern(filter, "*.gpfl");
			break;
	}

	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER (file_dialog), filter);
}

/*
 * control profile (load/save) clicked event
 * args:
 *   widget - pointer to event widget
 *   data - pointer to user data
 *
 * asserts:
 *   none
 *
 * returns: none
 */
void controls_profile_clicked (GtkWidget *item, void *data)
{
	GtkWidget *FileDialog;

	int save_or_load = GPOINTER_TO_INT(g_object_get_data (G_OBJECT (item), "profile_dialog"));

	if(debug_level > 0)
		printf("GUVCVIEW: Profile dialog (%d)\n", save_or_load);

	GtkWidget *main_window = get_main_window_gtk3();

	if (save_or_load > 0) /*save*/
	{
		FileDialog = gtk_file_chooser_dialog_new (_("Save Profile"),
			GTK_WINDOW(main_window),
			GTK_FILE_CHOOSER_ACTION_SAVE,
			 _("_Cancel"), GTK_RESPONSE_CANCEL,
			 _("_Save"), GTK_RESPONSE_ACCEPT,
			NULL);

		gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (FileDialog), TRUE);

		gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (FileDialog),
			get_profile_name());
	}
	else /*load*/
	{
		FileDialog = gtk_file_chooser_dialog_new (_("Load Profile"),
			GTK_WINDOW(main_window),
			GTK_FILE_CHOOSER_ACTION_OPEN,
			 _("_Cancel"), GTK_RESPONSE_CANCEL,
			 _("_Open"), GTK_RESPONSE_ACCEPT,
			NULL);
	}

	/** create a file filter */
	GtkFileFilter *filter = gtk_file_filter_new();

	GtkWidget *FBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	GtkWidget *format_label = gtk_label_new(_("File Format:"));
	gtk_widget_set_halign (FBox, GTK_ALIGN_FILL);
	gtk_widget_set_hexpand (FBox, TRUE);
	gtk_widget_set_hexpand (format_label, FALSE);
	gtk_widget_show(FBox);
	gtk_widget_show(format_label);
	gtk_box_pack_start(GTK_BOX(FBox), format_label, FALSE, FALSE, 2);

	GtkWidget *FileFormat = gtk_combo_box_text_new ();
	gtk_widget_set_halign (FileFormat, GTK_ALIGN_FILL);
	gtk_widget_set_hexpand (FileFormat, TRUE);

	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(FileFormat),_("gpfl  (*.gpfl)"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(FileFormat),_("any (*.*)"));

	gtk_combo_box_set_active(GTK_COMBO_BOX(FileFormat), 0);
	gtk_box_pack_start(GTK_BOX(FBox), FileFormat, FALSE, FALSE, 2);
	gtk_widget_show(FileFormat);

	gtk_file_filter_add_pattern(filter, "*.gpfl");

	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER (FileDialog), filter);
	gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER (FileDialog), FBox);

	g_signal_connect (GTK_COMBO_BOX(FileFormat), "changed",
		G_CALLBACK (profile_update_extension), FileDialog);


	gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (FileDialog),
		get_profile_path());

	if (gtk_dialog_run (GTK_DIALOG (FileDialog)) == GTK_RESPONSE_ACCEPT)
	{
		/*Save Controls Data*/
		const char *filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (FileDialog));

		if(save_or_load > 0)
		{
			v4l2core_save_control_profile(get_v4l2_device_handler(), filename);
		}
		else
		{
			v4l2core_load_control_profile(get_v4l2_device_handler(), filename);
			gui_gtk3_update_controls_state();
		}

		char *basename = get_file_basename(filename);
		if(basename)
		{
			set_profile_name(basename);
			free(basename);
		}
		char *pathname = get_file_pathname(filename);
		if(pathname)
		{
			set_profile_path(pathname);
			free(pathname);
		}
	}
	gtk_widget_destroy (FileDialog);
}

/*
 * photo suffix toggled event
 * args:
 *    item - widget that generated the event
 *    data - pointer to user data
 *
 * asserts:
 *    none
 *
 * returns: none
 */
void photo_sufix_toggled (GtkWidget *item, void *data)
{
  	int flag = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(item)) ? 1 : 0;
	set_photo_sufix_flag(flag);

	/*update config*/
	config_t *my_config = config_get();
	my_config->photo_sufix = flag;
}

/*
 * video suffix toggled event
 * args:
 *    item - widget that generated the event
 *    data - pointer to user data
 *
 * asserts:
 *    none
 *
 * returns: none
 */
void video_sufix_toggled (GtkWidget *item, void *data)
{
	int flag = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(item)) ? 1 : 0;
	set_video_sufix_flag(flag);

	/*update config*/
	config_t *my_config = config_get();
	my_config->video_sufix = flag;
}

/*
 * video codec changed event
 * args:
 *    item - widget that generated the event
 *    data - pointer to user data
 *
 * asserts:
 *    none
 *
 * returns: none
 */
void video_codec_changed (GtkRadioMenuItem *item, void *data)
{
   GSList *vgroup = (GSList *) data;

	if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(item)))
	{
		/*
		 * GSList indexes (g_slist_index) are in reverse order:
		 * last inserted has index 0
		 * so count backwards
		 */
		int num_codecs = g_slist_length(vgroup);
		int index = g_slist_index (vgroup, item);
		index = num_codecs - (index + 1); //reverse order and 0 indexed
		fprintf(stderr,"GUVCVIEW: video codec changed to %i\n", index);

		set_video_codec_ind(index);

		if( get_video_muxer() == ENCODER_MUX_WEBM &&
			!encoder_check_webm_video_codec(index))
		{
			/*change from webm to matroska*/
			set_video_muxer(ENCODER_MUX_MKV);
			char *newname = set_file_extension(get_video_name(), "mkv");
			set_video_name(newname);

			free(newname);
		}
	}
}

/*
 * audio codec changed event
 * args:
 *    item - widget that generated the event
 *    data - pointer to user data
 *
 * asserts:
 *    none
 *
 * returns: none
 */
void audio_codec_changed (GtkRadioMenuItem *item, void *data)
{
   GSList *agroup = (GSList *) data;

	if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(item)))
	{
		/*
		 * GSList indexes (g_slist_index) are in reverse order:
		 * last inserted has index 0
		 * so count backwards
		 */
		int num_codecs = g_slist_length(agroup);
		int index = g_slist_index (agroup, item);
		index = num_codecs - (index + 1); //reverse order and 0 indexed
		fprintf(stderr,"GUVCVIEW: audio codec changed to %i\n", index);

		set_audio_codec_ind(index);

		if( get_video_muxer() == ENCODER_MUX_WEBM &&
			!encoder_check_webm_audio_codec(index))
		{
			/*change from webm to matroska*/
			set_video_muxer(ENCODER_MUX_MKV);
			char *newname = set_file_extension(get_video_name(), "mkv");
			set_video_name(newname);
			free(newname);
		}
	}
}

/*
 * called from photo format combo in file dialog
 * args:
 *    chooser - format combo that caused the event
 *    file_dialog - chooser parent
 *
 * asserts:
 *    none
 *
 * returns: none
 */
static void photo_update_extension (GtkComboBox *chooser, GtkWidget *file_dialog)
{
	int format = gtk_combo_box_get_active (chooser);

	set_photo_format(format);

	char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER (file_dialog));
	char *basename = get_file_basename(filename);

	GtkFileFilter *filter = gtk_file_filter_new();

	switch(format)
	{
		case IMG_FMT_RAW:
			gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (file_dialog),
				set_file_extension(basename, "raw"));
			gtk_file_filter_add_pattern(filter, "*.raw");
			break;
		case IMG_FMT_PNG:
			gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (file_dialog),
				set_file_extension(basename, "png"));
			gtk_file_filter_add_pattern(filter, "*.png");
			break;
		case IMG_FMT_BMP:
			gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (file_dialog),
				set_file_extension(basename, "bmp"));
			gtk_file_filter_add_pattern(filter, "*.bmp");
			break;
		default:
		case IMG_FMT_JPG:
			gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (file_dialog),
				set_file_extension(basename, "jpg"));
			gtk_file_filter_add_pattern(filter, "*.jpg");
			break;
	}

	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER (file_dialog), filter);

	if(filename)
		free(filename);
	if(basename)
		free(basename);
}

/*
 * called from video muxer format combo in file dialog
 * args:
 *    chooser - format combo that caused the event
 *    file_dialog - chooser parent
 *
 * asserts:
 *    none
 *
 * returns: none
 */
static void video_update_extension (GtkComboBox *chooser, GtkWidget *file_dialog)
{
	int format = gtk_combo_box_get_active (chooser);

	set_video_muxer(format);

	char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER (file_dialog));
	char *basename = get_file_basename(filename);

	GtkFileFilter *filter = gtk_file_filter_new();

	switch(format)
	{
		case ENCODER_MUX_WEBM:
			gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (file_dialog),
				set_file_extension(basename, "webm"));
			gtk_file_filter_add_pattern(filter, "*.webm");
			break;
		case ENCODER_MUX_AVI:
			gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (file_dialog),
				set_file_extension(basename, "avi"));
			gtk_file_filter_add_pattern(filter, "*.avi");
			break;
		default:
		case ENCODER_MUX_MKV:
			gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (file_dialog),
				set_file_extension(basename, "mkv"));
			gtk_file_filter_add_pattern(filter, "*.mkv");
			break;
	}

	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER (file_dialog), filter);

	if(filename)
		free(filename);
	if(basename)
		free(basename);
}

/*
 * photo file clicked event
 * args:
 *   item - pointer to widget that generated the event
 *   data - pointer to user data
 *
 * asserts:
 *   none
 *
 * returns: none
 */
void photo_file_clicked (GtkWidget *item, void *data)
{
	GtkWidget *FileDialog;

	GtkWidget *main_window = get_main_window_gtk3();

	FileDialog = gtk_file_chooser_dialog_new (_("Photo file name"),
			GTK_WINDOW(main_window),
			GTK_FILE_CHOOSER_ACTION_SAVE,
			_("_Cancel"), GTK_RESPONSE_CANCEL,
			_("_Save"), GTK_RESPONSE_ACCEPT,
			NULL);
	gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (FileDialog), TRUE);

	/** create a file filter */
	GtkFileFilter *filter = gtk_file_filter_new();

	GtkWidget *FBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	GtkWidget *format_label = gtk_label_new(_("File Format:"));
	gtk_widget_set_halign (FBox, GTK_ALIGN_FILL);
	gtk_widget_set_hexpand (FBox, TRUE);
	gtk_widget_set_hexpand (format_label, FALSE);
	gtk_widget_show(FBox);
	gtk_widget_show(format_label);
	gtk_box_pack_start(GTK_BOX(FBox), format_label, FALSE, FALSE, 2);

	GtkWidget *ImgFormat = gtk_combo_box_text_new ();
	gtk_widget_set_halign (ImgFormat, GTK_ALIGN_FILL);
	gtk_widget_set_hexpand (ImgFormat, TRUE);

	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(ImgFormat),_("Raw  (*.raw)"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(ImgFormat),_("Jpeg (*.jpg)"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(ImgFormat),_("Png  (*.png)"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(ImgFormat),_("Bmp  (*.bmp)"));

	gtk_combo_box_set_active(GTK_COMBO_BOX(ImgFormat), get_photo_format());
	gtk_box_pack_start(GTK_BOX(FBox), ImgFormat, FALSE, FALSE, 2);
	gtk_widget_show(ImgFormat);

	/**add a pattern to the filter*/
	switch(get_photo_format())
	{
		case IMG_FMT_RAW:
			gtk_file_filter_add_pattern(filter, "*.raw");
			break;
		case IMG_FMT_PNG:
			gtk_file_filter_add_pattern(filter, "*.png");
			break;
		case IMG_FMT_BMP:
			gtk_file_filter_add_pattern(filter, "*.bmp");
			break;
		default:
		case IMG_FMT_JPG:
			gtk_file_filter_add_pattern(filter, "*.jpg");
			break;
	}

	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER (FileDialog), filter);
	gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER (FileDialog), FBox);

	g_signal_connect (GTK_COMBO_BOX(ImgFormat), "changed",
		G_CALLBACK (photo_update_extension), FileDialog);

	gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (FileDialog),
		get_photo_name());

	gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (FileDialog),
		get_photo_path());

	if (gtk_dialog_run (GTK_DIALOG (FileDialog)) == GTK_RESPONSE_ACCEPT)
	{
		const char *filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (FileDialog));

		char *basename = get_file_basename(filename);
		if(basename)
		{
			set_photo_name(basename);
			free(basename);
		}
		char *pathname = get_file_pathname(filename);
		if(pathname)
		{
			set_photo_path(pathname);
			free(pathname);
		}
	}
	gtk_widget_destroy (FileDialog);
}

/*
 * video file clicked event
 * args:
 *   item - pointer to widget that generated the event
 *   data - pointer to user data
 *
 * asserts:
 *   none
 *
 * returns: none
 */
void video_file_clicked (GtkWidget *item, void *data)
{
	GtkWidget *FileDialog;

	GtkWidget *main_window = get_main_window_gtk3();

	FileDialog = gtk_file_chooser_dialog_new (_("Video file name"),
			GTK_WINDOW(main_window),
			GTK_FILE_CHOOSER_ACTION_SAVE,
			_("_Cancel"), GTK_RESPONSE_CANCEL,
			_("_Save"), GTK_RESPONSE_ACCEPT,
			NULL);

	gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (FileDialog), TRUE);

	/** create a file filter */
	GtkFileFilter *filter = gtk_file_filter_new();

	GtkWidget *FBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	GtkWidget *format_label = gtk_label_new(_("File Format:"));
	gtk_widget_set_halign (FBox, GTK_ALIGN_FILL);
	gtk_widget_set_hexpand (FBox, TRUE);
	gtk_widget_set_hexpand (format_label, FALSE);
	gtk_widget_show(FBox);
	gtk_widget_show(format_label);
	gtk_box_pack_start(GTK_BOX(FBox), format_label, FALSE, FALSE, 2);

	GtkWidget *VideoFormat = gtk_combo_box_text_new ();
	gtk_widget_set_halign (VideoFormat, GTK_ALIGN_FILL);
	gtk_widget_set_hexpand (VideoFormat, TRUE);

	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(VideoFormat),_("Matroska  (*.mkv)"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(VideoFormat),_("WebM (*.webm)"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(VideoFormat),_("Avi  (*.avi)"));

	gtk_combo_box_set_active(GTK_COMBO_BOX(VideoFormat), get_video_muxer());
	gtk_box_pack_start(GTK_BOX(FBox), VideoFormat, FALSE, FALSE, 2);
	gtk_widget_show(VideoFormat);

	/**add a pattern to the filter*/
	switch(get_video_muxer())
	{
		case ENCODER_MUX_WEBM:
			gtk_file_filter_add_pattern(filter, "*.webm");
			break;
		case ENCODER_MUX_AVI:
			gtk_file_filter_add_pattern(filter, "*.avi");
			break;
		default:
		case ENCODER_MUX_MKV:
			gtk_file_filter_add_pattern(filter, "*.mkv");
			break;

	}

	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER (FileDialog), filter);
	gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER (FileDialog), FBox);

	g_signal_connect (GTK_COMBO_BOX(VideoFormat), "changed",
		G_CALLBACK (video_update_extension), FileDialog);

	gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (FileDialog),
		get_video_name());

	gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (FileDialog),
		get_video_path());

	if (gtk_dialog_run (GTK_DIALOG (FileDialog)) == GTK_RESPONSE_ACCEPT)
	{
		const char *filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (FileDialog));

		char *basename = get_file_basename(filename);
		if(basename)
		{
			set_video_name(basename);
			free(basename);
		}
		char *pathname = get_file_pathname(filename);
		if(pathname)
		{
			set_video_path(pathname);
			free(pathname);
		}
	}
	gtk_widget_destroy (FileDialog);
}

/*
 * capture image button clicked event
 * args:
 *   button - widget that generated the event
 *   data - pointer to user data
 *
 * asserts:
 *   none
 *
 * returns: none
 */
void capture_image_clicked (GtkButton *button, void *data)
{
	int is_photo_timer = GPOINTER_TO_INT(g_object_get_data (G_OBJECT (button), "control_info"));

	if(is_photo_timer)
	{
		stop_photo_timer();
		gtk_button_set_label(button, _("Cap. Image (I)"));
	}
	else
		video_capture_save_image();
}

/*
 * capture video button clicked event
 * args:
 *   button - widget that generated the event
 *   data - pointer to user data
 *
 * asserts:
 *   none
 *
 * returns: none
 */
void capture_video_clicked(GtkToggleButton *button, void *data)
{
	int active = gtk_toggle_button_get_active (button);

	if(debug_level > 0)
		printf("GUVCVIEW: video capture toggled(%i)\n", active);

	if(active)
	{
		start_encoder_thread();
		gtk_button_set_label(GTK_BUTTON(button), _("Stop Video (V)"));

	}
	else
	{
		stop_encoder_thread();
		gtk_button_set_label(GTK_BUTTON(button), _("Cap. Video (V)"));
		/*make sure video timer is reset*/
		reset_video_timer();
	}
}

/*
 * pan/tilt step changed
 * args:
 *    spin - spinbutton that generated the event
 *    data - pointer to user data
 *
 * asserts:
 *    none
 *
 * returns:
 *    none
 */
void pan_tilt_step_changed (GtkSpinButton *spin, void *data)
{
	int id = GPOINTER_TO_INT(g_object_get_data (G_OBJECT (spin), "control_info"));

	int val = gtk_spin_button_get_value_as_int (spin);

	if(id == V4L2_CID_PAN_RELATIVE)
		v4l2core_set_pan_step(get_v4l2_device_handler(), val);
	if(id == V4L2_CID_TILT_RELATIVE)
		v4l2core_set_tilt_step(get_v4l2_device_handler(), val);
}

/*
 * Pan Tilt button 1 clicked
 * args:
 *    button - button that generated the event
 *    data - pointer to user data
 *
 * asserts:
 *    none
 *
 * returns: none
 */
void button_PanTilt1_clicked (GtkButton * Button, void *data)
{
    int id = GPOINTER_TO_INT(g_object_get_data (G_OBJECT (Button), "control_info"));

    v4l2_ctrl_t *control = v4l2core_get_control_by_id(get_v4l2_device_handler(), id);

	if(id == V4L2_CID_PAN_RELATIVE)
		control->value = v4l2core_get_pan_step(get_v4l2_device_handler());
	else
		control->value = v4l2core_get_tilt_step(get_v4l2_device_handler());

    if(v4l2core_set_control_value_by_id(get_v4l2_device_handler(), id))
		fprintf(stderr, "GUVCVIEW: error setting pan/tilt\n");
}

/*
 * Pan Tilt button 2 clicked
 * args:
 *    button - button that generated the event
 *    data - pointer to user data
 *
 * asserts:
 *    none
 *
 * returns: none
 */
void button_PanTilt2_clicked (GtkButton * Button, void *data)
{
    int id = GPOINTER_TO_INT(g_object_get_data (G_OBJECT (Button), "control_info"));

    v4l2_ctrl_t *control = v4l2core_get_control_by_id(get_v4l2_device_handler(), id);

    if(id == V4L2_CID_PAN_RELATIVE)
		control->value =  - v4l2core_get_pan_step(get_v4l2_device_handler());
	else
		control->value =  - v4l2core_get_tilt_step(get_v4l2_device_handler());

    if(v4l2core_set_control_value_by_id(get_v4l2_device_handler(), id))
		fprintf(stderr, "GUVCVIEW: error setting pan/tilt\n");
}

/*
 * generic button clicked
 * args:
 *    button - button that generated the event
 *    data - pointer to user data
 *
 * asserts:
 *    none
 *
 * returns: none
 */
void button_clicked (GtkButton * Button, void *data)
{
    int id = GPOINTER_TO_INT(g_object_get_data (G_OBJECT (Button), "control_info"));

    v4l2_ctrl_t *control = v4l2core_get_control_by_id(get_v4l2_device_handler(), id);

	control->value = 1;

    if(v4l2core_set_control_value_by_id(get_v4l2_device_handler(), id))
		fprintf(stderr, "GUVCVIEW: error setting button value\n");

	gui_gtk3_update_controls_state();
}

/*
 * a string control apply button clicked
 * args:
 *    button - button that generated the event
 *    data - pointer to user data
 *
 * asserts:
 *    control->string not null
 *
 * returns: none
 */
void string_button_clicked(GtkButton * Button, void *data)
{
	int id = GPOINTER_TO_INT(g_object_get_data (G_OBJECT (Button), "control_info"));
	GtkWidget *entry = (GtkWidget *) g_object_get_data (G_OBJECT (Button), "control_entry");

	v4l2_ctrl_t *control = v4l2core_get_control_by_id(get_v4l2_device_handler(), id);

	assert(control->string != NULL);

	strncpy(control->string, gtk_entry_get_text(GTK_ENTRY(entry)), control->control.maximum + 1);

	if(v4l2core_set_control_value_by_id(get_v4l2_device_handler(), id))
		fprintf(stderr, "GUVCVIEW: error setting string value\n");
}

/*
 * a int64 control apply button clicked
 * args:
 *    button - button that generated the event
 *    data - pointer to user data
 *
 * asserts:
 *    none
 *
 * returns: none
 */
void int64_button_clicked(GtkButton * Button, void *data)
{
	int id = GPOINTER_TO_INT(g_object_get_data (G_OBJECT (Button), "control_info"));
	GtkWidget *entry = (GtkWidget *) g_object_get_data (G_OBJECT (Button), "control_entry");

	v4l2_ctrl_t *control = v4l2core_get_control_by_id(get_v4l2_device_handler(), id);

	char* text_input = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
	text_input = g_strstrip(text_input);
	if( g_str_has_prefix(text_input, "0x")) //hex format
	{
		text_input = g_strcanon(text_input, "0123456789ABCDEFabcdef", '0');
		control->value64 = g_ascii_strtoll(text_input, NULL, 16);
	}
	else //decimal or hex ?
	{
		text_input = g_strcanon(text_input, "0123456789ABCDEFabcdef", '0');
		control->value64 = g_ascii_strtoll(text_input, NULL, 0);
	}
	g_free(text_input);

	if(debug_level > 1)
		printf("GUVCVIEW: applying int64 value: %" PRId64 "\n", control->value64);

	if(v4l2core_set_control_value_by_id(get_v4l2_device_handler(), id))
		fprintf(stderr, "GUVCVIEW: error setting string value\n");

}

/*
 * a bitmask control apply button clicked
 * args:
 *    button - button that generated the event
 *    data - pointer to user data
 *
 * asserts:
 *    none
 *
 * returns: none
 */
void bitmask_button_clicked(GtkButton * Button, void *data)
{
	int id = GPOINTER_TO_INT(g_object_get_data (G_OBJECT (Button), "control_info"));
	GtkWidget *entry = (GtkWidget *) g_object_get_data (G_OBJECT (Button), "control_entry");

	v4l2_ctrl_t *control = v4l2core_get_control_by_id(get_v4l2_device_handler(), id);

	char* text_input = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
	text_input = g_strcanon(text_input,"0123456789ABCDEFabcdef", '0');
	control->value = (int32_t) g_ascii_strtoll(text_input, NULL, 16);
	g_free(text_input);

	if(v4l2core_set_control_value_by_id(get_v4l2_device_handler(), id))
		fprintf(stderr, "GUVCVIEW: error setting string value\n");
}

/*
 * slider changed event
 * args:
 *    range - widget that generated the event
 *    data - pointer to user data
 *
 * asserts:
 *    none
 *
 * returns: none
 */
void slider_changed (GtkRange * range, void *data)
{
    int id = GPOINTER_TO_INT(g_object_get_data (G_OBJECT (range), "control_info"));
    v4l2_ctrl_t *control = v4l2core_get_control_by_id(get_v4l2_device_handler(), id);

    int val = (int) gtk_range_get_value (range);

    control->value = val;

    if(v4l2core_set_control_value_by_id(get_v4l2_device_handler(), id))
		fprintf(stderr, "GUVCVIEW: error setting slider value\n");

   /*
    if(widget2)
    {
        //disable widget signals
        g_signal_handlers_block_by_func(GTK_SPIN_BUTTON(widget2),
            G_CALLBACK (spin_changed), data);
        gtk_spin_button_set_value (GTK_SPIN_BUTTON(widget2), control->value);
        //enable widget signals
        g_signal_handlers_unblock_by_func(GTK_SPIN_BUTTON(widget2),
            G_CALLBACK (spin_changed), data);
    }
	*/
}

/*
 * spin changed event
 * args:
 *    spin - widget that generated the event
 *    data - pointer to user data
 *
 * asserts:
 *    none
 *
 * returns: none
 */
void spin_changed (GtkSpinButton * spin, void *data)
{
    int id = GPOINTER_TO_INT(g_object_get_data (G_OBJECT (spin), "control_info"));
    v4l2_ctrl_t *control = v4l2core_get_control_by_id(get_v4l2_device_handler(), id);

	int val = gtk_spin_button_get_value_as_int (spin);
    control->value = val;

     if(v4l2core_set_control_value_by_id(get_v4l2_device_handler(), id))
		fprintf(stderr, "GUVCVIEW: error setting spin value\n");

	/*
    if(widget)
    {
        //disable widget signals
        g_signal_handlers_block_by_func(GTK_SCALE (widget),
            G_CALLBACK (slider_changed), data);
        gtk_range_set_value (GTK_RANGE (widget), control->value);
        //enable widget signals
        g_signal_handlers_unblock_by_func(GTK_SCALE (widget),
            G_CALLBACK (slider_changed), data);
    }
	*/

}

/*
 * combo box changed event
 * args:
 *    combo - widget that generated the event
 *    data - pointer to user data
 *
 * asserts:
 *    none
 *
 * returns: none
 */
void combo_changed (GtkComboBox * combo, void *data)
{
    int id = GPOINTER_TO_INT(g_object_get_data (G_OBJECT (combo), "control_info"));
    v4l2_ctrl_t *control = v4l2core_get_control_by_id(get_v4l2_device_handler(), id);

    int index = gtk_combo_box_get_active (combo);
    control->value = control->menu[index].index;

	if(v4l2core_set_control_value_by_id(get_v4l2_device_handler(), id))
		fprintf(stderr, "GUVCVIEW: error setting menu value\n");

	gui_gtk3_update_controls_state();
}

/*
 * bayer pixel order combo box changed event
 * args:
 *    combo - widget that generated the event
 *    data - pointer to user data
 *
 * asserts:
 *    none
 *
 * returns: none
 */
void bayer_pix_ord_changed (GtkComboBox * combo, void *data)
{
	//int id = GPOINTER_TO_INT(g_object_get_data (G_OBJECT (combo), "control_info"));

	int index = gtk_combo_box_get_active (combo);
	v4l2core_set_bayer_pix_order(get_v4l2_device_handler(), index);
}

/*
 * check box changed event
 * args:
 *    toggle - widget that generated the event
 *    data - pointer to user data
 *
 * asserts:
 *    none
 *
 * returns: none
 */
void check_changed (GtkToggleButton *toggle, void *data)
{
    int id = GPOINTER_TO_INT(g_object_get_data (G_OBJECT (toggle), "control_info"));
    v4l2_ctrl_t *control = v4l2core_get_control_by_id(get_v4l2_device_handler(), id);

    int val = gtk_toggle_button_get_active (toggle) ? 1 : 0;

    control->value = val;

	if(v4l2core_set_control_value_by_id(get_v4l2_device_handler(), id))
		fprintf(stderr, "GUVCVIEW: error setting menu value\n");

    if(id == V4L2_CID_DISABLE_PROCESSING_LOGITECH)
    {
        if (control->value > 0)
			v4l2core_set_isbayer(get_v4l2_device_handler(), 1);
        else
			v4l2core_set_isbayer(get_v4l2_device_handler(), 0);

        /*
         * must restart stream and requeue
         * the buffers for changes to take effect
         * (updating fps provides all that is needed)
         */
        v4l2core_request_framerate_update (get_v4l2_device_handler());
    }

    gui_gtk3_update_controls_state();
}

/*
 * device list box changed event
 * args:
 *    wgtDevices - widget that generated the event
 *    data - pointer to user data
 *
 * asserts:
 *    none
 *
 * returns: none
 */
void devices_changed (GtkComboBox *wgtDevices, void *data)
{
	GError *error = NULL;

	int index = gtk_combo_box_get_active(wgtDevices);
	if(index == v4l2core_get_this_device_index(get_v4l2_device_handler()))
		return;

	GtkWidget *restartdialog = gtk_dialog_new_with_buttons (_("start new"),
		GTK_WINDOW(get_main_window_gtk3()),
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		_("restart"),
		GTK_RESPONSE_ACCEPT,
		_("new"),
		GTK_RESPONSE_REJECT,
		_("cancel"),
		GTK_RESPONSE_CANCEL,
		NULL);

	GtkWidget *content_area = gtk_dialog_get_content_area (GTK_DIALOG (restartdialog));
	GtkWidget *message = gtk_label_new (_("launch new process or restart?.\n\n"));
	gtk_container_add (GTK_CONTAINER (content_area), message);
	gtk_widget_show_all(restartdialog);

	gint result = gtk_dialog_run (GTK_DIALOG (restartdialog));

	/*check device index only after dialog response*/
	char videodevice[30];
	strncpy(videodevice, v4l2core_get_device_sys_data(index)->device, 29);
	gchar *command = g_strjoin("",
		g_get_prgname(),
		" --device=",
		videodevice,
		NULL);

	switch (result)
	{
		case GTK_RESPONSE_ACCEPT: /*FIXME: restart or reset device without closing the app*/
			if(debug_level > 1)
				printf("GUVCVIEW: spawning new process: '%s'\n", command);
			/*spawn new process*/
			if(!(g_spawn_command_line_async(command, &error)))
			{
				fprintf(stderr, "GUVCVIEW: spawn failed: %s\n", error->message);
				g_error_free( error );
			}
			else
				quit_callback(NULL);
			break;
		case GTK_RESPONSE_REJECT:
			if(debug_level > 1)
				printf("GUVCVIEW: spawning new process: '%s'\n", command);
			/*spawn new process*/
			if(!(g_spawn_command_line_async(command, &error)))
			{
				fprintf(stderr, "GUVCVIEW: spawn failed: %s\n", error->message);
				g_error_free( error );
			}
			break;
		default:
			/* do nothing since dialog was canceled*/
			break;
	}
	/*reset to current device*/
	gtk_combo_box_set_active(GTK_COMBO_BOX(wgtDevices), 
		v4l2core_get_this_device_index(get_v4l2_device_handler()));

	gtk_widget_destroy (restartdialog);
	g_free(command);
}

/*
 * frame rate list box changed event
 * args:
 *    wgtFrameRate - widget that generated the event
 *    data - pointer to user data
 *
 * asserts:
 *    none
 *
 * returns: none
 */
void frame_rate_changed (GtkComboBox *wgtFrameRate, void *data)
{
	int format_index = v4l2core_get_frame_format_index(get_v4l2_device_handler(), v4l2core_get_requested_frame_format(get_v4l2_device_handler()));

	int resolu_index = v4l2core_get_format_resolution_index(
		get_v4l2_device_handler(),
		format_index,
		v4l2core_get_frame_width(get_v4l2_device_handler()),
		v4l2core_get_frame_height(get_v4l2_device_handler()));

	int index = gtk_combo_box_get_active (wgtFrameRate);

	v4l2_stream_formats_t *list_stream_formats = v4l2core_get_formats_list(get_v4l2_device_handler());
	
	int fps_denom = list_stream_formats[format_index].list_stream_cap[resolu_index].framerate_denom[index];
	int fps_num = list_stream_formats[format_index].list_stream_cap[resolu_index].framerate_num[index];
	
	v4l2core_define_fps(get_v4l2_device_handler(), fps_num, fps_denom);

	int fps[2] = {fps_num, fps_denom};
	gui_set_fps(fps);

	v4l2core_request_framerate_update (get_v4l2_device_handler());
}

/*
 * resolution list box changed event
 * args:
 *    wgtResolution - widget that generated the event
 *    data - pointer to user data
 *
 * asserts:
 *    none
 *
 * returns: none
 */
void resolution_changed (GtkComboBox *wgtResolution, void *data)
{
	int format_index = v4l2core_get_frame_format_index(
		get_v4l2_device_handler(),
		v4l2core_get_requested_frame_format(get_v4l2_device_handler()));

	int cmb_index = gtk_combo_box_get_active(wgtResolution);

	GtkWidget *wgtFrameRate = (GtkWidget *) g_object_get_data (G_OBJECT (wgtResolution), "control_fps");

	char temp_str[20];

	/*disable fps combobox signals*/
	g_signal_handlers_block_by_func(GTK_COMBO_BOX_TEXT(wgtFrameRate), G_CALLBACK (frame_rate_changed), NULL);
	/* clear out the old fps list... */
	GtkListStore *store = GTK_LIST_STORE(gtk_combo_box_get_model (GTK_COMBO_BOX(wgtFrameRate)));
	gtk_list_store_clear(store);

	v4l2_stream_formats_t *list_stream_formats = v4l2core_get_formats_list(get_v4l2_device_handler());
	
	int width = list_stream_formats[format_index].list_stream_cap[cmb_index].width;
	int height = list_stream_formats[format_index].list_stream_cap[cmb_index].height;

	/*check if frame rate is available at the new resolution*/
	int i=0;
	int deffps=0;

	for ( i = 0 ; i < list_stream_formats[format_index].list_stream_cap[cmb_index].numb_frates ; i++)
	{
		g_snprintf(
			temp_str,
			18,
			"%i/%i fps",
			list_stream_formats[format_index].list_stream_cap[cmb_index].framerate_denom[i],
			list_stream_formats[format_index].list_stream_cap[cmb_index].framerate_num[i]);

		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(wgtFrameRate), temp_str);

		if (( v4l2core_get_fps_num(get_v4l2_device_handler()) == list_stream_formats[format_index].list_stream_cap[cmb_index].framerate_num[i]) &&
			( v4l2core_get_fps_denom(get_v4l2_device_handler()) == list_stream_formats[format_index].list_stream_cap[cmb_index].framerate_denom[i]))
				deffps=i;
	}

	/*set default fps in combo*/
	gtk_combo_box_set_active(GTK_COMBO_BOX(wgtFrameRate), deffps);

	/*enable fps combobox signals*/
	g_signal_handlers_unblock_by_func(GTK_COMBO_BOX_TEXT(wgtFrameRate), G_CALLBACK (frame_rate_changed), NULL);

	if (list_stream_formats[format_index].list_stream_cap[cmb_index].framerate_num)
		v4l2core_define_fps(
			get_v4l2_device_handler(),
			list_stream_formats[format_index].list_stream_cap[cmb_index].framerate_num[deffps], -1);

	if (list_stream_formats[format_index].list_stream_cap[cmb_index].framerate_denom)
		v4l2core_define_fps(
			get_v4l2_device_handler(),
			-1, 
			list_stream_formats[format_index].list_stream_cap[cmb_index].framerate_denom[deffps]);

	/*change resolution (try new format and reset render)*/
	v4l2core_prepare_new_resolution(get_v4l2_device_handler(), width, height);

	request_format_update();

	/*update the config data*/
	config_t *my_config = config_get();

	my_config->width = width;
	my_config->height= height;
}

/*
 * device pixel format list box changed event
 * args:
 *    wgtInpType - widget that generated the event
 *    data - pointer to user data
 *
 * asserts:
 *    none
 *
 * returns: none
 */
void format_changed(GtkComboBox *wgtInpType, void *data)
{
	char temp_str[20];
	int index = gtk_combo_box_get_active(wgtInpType);

	//check if format is supported
	v4l2_stream_formats_t *list_stream_formats = v4l2core_get_formats_list(get_v4l2_device_handler());
	if(!list_stream_formats[index].dec_support)
	{
		int fmtind=0;
		for (fmtind=0; fmtind < v4l2core_get_number_formats(get_v4l2_device_handler()); fmtind++)
		{
			//keep current format
			if(v4l2core_get_requested_frame_format(get_v4l2_device_handler()) == list_stream_formats[fmtind].format)
			{
				g_signal_handlers_block_by_func(GTK_COMBO_BOX_TEXT(wgtInpType), G_CALLBACK (format_changed), NULL);
				gtk_combo_box_set_active(GTK_COMBO_BOX(wgtInpType), fmtind); /*set active*/
				g_signal_handlers_unblock_by_func(GTK_COMBO_BOX_TEXT(wgtInpType), G_CALLBACK (format_changed), NULL);
			}
		}
		return;
	}

	//GtkWidget *wgtFrameRate = (GtkWidget *) g_object_get_data (G_OBJECT (wgtInpType), "control_fps");
	GtkWidget *wgtResolution = (GtkWidget *) g_object_get_data (G_OBJECT (wgtInpType), "control_resolution");

	/*disable resolution combobox signals*/
	g_signal_handlers_block_by_func(GTK_COMBO_BOX_TEXT(wgtResolution), G_CALLBACK (resolution_changed), NULL);

	/* clear out the old resolution list... */
	GtkListStore *store = GTK_LIST_STORE(gtk_combo_box_get_model (GTK_COMBO_BOX(wgtResolution)));
	gtk_list_store_clear(store);
		
	int format = list_stream_formats[index].format;

	/*update config*/
	config_t *my_config = config_get();
	my_config->format =  list_stream_formats[index].format;

	int i=0;
	int defres = 0;
	/*redraw resolution combo for new format*/
	for(i = 0 ; i < list_stream_formats[index].numb_res ; i++)
	{
		if (list_stream_formats[index].list_stream_cap[i].width > 0)
		{
			g_snprintf(
				temp_str,
				18,
				"%ix%i",
				list_stream_formats[index].list_stream_cap[i].width,
				list_stream_formats[index].list_stream_cap[i].height);

			gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(wgtResolution), temp_str);

			if ((v4l2core_get_frame_width(get_v4l2_device_handler()) == list_stream_formats[index].list_stream_cap[i].width) &&
				(v4l2core_get_frame_height(get_v4l2_device_handler()) == list_stream_formats[index].list_stream_cap[i].height))
			{
				/*current resolution is valid for new format*/
				defres=i;//set resolution index for new format
				if(debug_level > 1)
					printf("GUVCVIEW (Gtk3): selected format (%i) supports current resolution at index %i\n", index, defres);
			}
		}
	}

	/*enable resolution combobox signals*/
	g_signal_handlers_unblock_by_func(GTK_COMBO_BOX_TEXT(wgtResolution), G_CALLBACK (resolution_changed), NULL);

	/*prepare new format*/
	v4l2core_prepare_new_format(get_v4l2_device_handler(), format);
	
	/*update resolution*/
	gtk_combo_box_set_active(GTK_COMBO_BOX(wgtResolution), defres); /*reset render*/
}

/*
 * render fx filter changed event
 * args:
 *    toggle - widget that generated the event
 *    data - pointer to user data
 *
 * asserts:
 *    none
 *
 * returns: none
 */
void render_fx_filter_changed(GtkToggleButton *toggle, void *data)
{
	int filter = GPOINTER_TO_INT(g_object_get_data (G_OBJECT (toggle), "filt_info"));

	uint32_t mask = gtk_toggle_button_get_active (toggle) ?
			get_render_fx_mask() | filter :
			get_render_fx_mask() & ~filter;

	set_render_fx_mask(mask);
}

/*
 * audio fx filter changed event
 * args:
 *    toggle - widget that generated the event
 *    data - pointer to user data
 *
 * asserts:
 *    none
 *
 * returns: none
 */
void audio_fx_filter_changed(GtkToggleButton *toggle, void *data)
{
	int filter = GPOINTER_TO_INT(g_object_get_data (G_OBJECT (toggle), "filt_info"));

	uint32_t mask = gtk_toggle_button_get_active (toggle) ?
			get_audio_fx_mask() | filter :
			get_audio_fx_mask() & ~filter;

	set_audio_fx_mask(mask);
}

/*
 * render osd changed event
 * args:
 *    toggle - widget that generated the event
 *    data - pointer to user data
 *
 * asserts:
 *    none
 *
 * returns: none
 */
void render_osd_changed(GtkToggleButton *toggle, void *data)
{
	int osd = GPOINTER_TO_INT(g_object_get_data (G_OBJECT (toggle), "osd_info"));

	uint32_t mask = gtk_toggle_button_get_active (toggle) ?
			render_get_osd_mask() | osd :
			render_get_osd_mask() & ~osd;

	render_set_osd_mask(mask);

	/* update config */
	config_t *my_config = config_get();
	my_config->osd_mask = render_get_osd_mask();
	/*make sure to disable VU meter OSD in config - it's set by audio capture*/
	my_config->osd_mask &= ~REND_OSD_VUMETER_MONO;
	my_config->osd_mask &= ~REND_OSD_VUMETER_STEREO;
}

/*
 * software autofocus checkbox changed event
 * args:
 *    toggle - widget that generated the event
 *    data - pointer to user data
 *
 * asserts:
 *    none
 *
 * returns: none
 */
void autofocus_changed (GtkToggleButton * toggle, void *data)
{
	int val = gtk_toggle_button_get_active (toggle) ? 1 : 0;

	GtkWidget *wgtFocus_slider = (GtkWidget *) g_object_get_data (G_OBJECT (toggle), "control_entry");
	GtkWidget *wgtFocus_spin = (GtkWidget *) g_object_get_data (G_OBJECT (toggle), "control2_entry");
	/*if autofocus disable manual focus control*/
	gtk_widget_set_sensitive (wgtFocus_slider, !val);
	gtk_widget_set_sensitive (wgtFocus_spin, !val);

	set_soft_autofocus(val);

}

/*
 * software autofocus button clicked event
 * args:
 *    button - widget that generated the event
 *    data - pointer to user data
 *
 * asserts:
 *    none
 *
 * returns: none
 */
void setfocus_clicked (GtkButton * button, void *data)
{
	set_soft_focus(1);
}

/******************* AUDIO CALLBACKS *************************/

/*
 * audio device list box changed event
 * args:
 *    combo - widget that generated the event
 *    data - pointer to user data
 *
 * asserts:
 *    none
 *
 * returns: none
 */
void audio_device_changed(GtkComboBox *combo, void *data)
{
	audio_widgets_t *my_audio_widgets = (audio_widgets_t *) data;

	int index = gtk_combo_box_get_active(combo);

	/*update the audio context for the new api*/
	audio_context_t *audio_ctx = get_audio_context();

	if(index < 0)
		index = 0;
	else if (index >= audio_get_num_inp_devices(audio_ctx))
		index = audio_get_num_inp_devices(audio_ctx) - 1;
	
	/*update config*/
	config_t *my_config = config_get();
	my_config->audio_device = index;
	
	/*set the audio device defaults*/
	audio_set_device_index(audio_ctx, my_config->audio_device);

	if(debug_level > 0)
		printf("GUVCVIEW: audio device changed to %i\n", 
			audio_get_device_index(audio_ctx));

	index = gtk_combo_box_get_active(GTK_COMBO_BOX(my_audio_widgets->channels));

	if(index == 0)
	{
		int channels = audio_get_device(audio_ctx, 
			audio_get_device_index(audio_ctx))->channels;
		audio_set_channels(audio_ctx, channels);
		if(audio_get_channels(audio_ctx) > 2)
			audio_set_channels(audio_ctx, 2);/*limit it to stereo input*/
	}

	index = gtk_combo_box_get_active(GTK_COMBO_BOX(my_audio_widgets->samprate));

	if(index == 0)
	{
		int samprate = audio_get_device(audio_ctx, 
			audio_get_device_index(audio_ctx))->samprate;
		audio_set_samprate(audio_ctx, samprate);
	}	
	gtk_range_set_value(GTK_RANGE(my_audio_widgets->latency), 
		audio_get_latency(audio_ctx));
}

/*
 * audio samplerate list box changed event
 * args:
 *    combo - widget that generated the event
 *    data - pointer to user data
 *
 * asserts:
 *    none
 *
 * returns: none
 */
void audio_samplerate_changed(GtkComboBox *combo, void *data)
{
	int index = gtk_combo_box_get_active(combo);

	/*update the audio context for the new api*/
	audio_context_t *audio_ctx = get_audio_context();

	int samprate = 44100;

	switch(index)
	{
		case 0:
			samprate = audio_get_device(audio_ctx, 
				audio_get_device_index(audio_ctx))->samprate;
			break;
		case 1:
			samprate =  7350;
			break;
		case 2:
			samprate = 8000;
			break;
		case 3:
			samprate = 11025;
			break;
		case 4:
			samprate = 12000;
			break;
		case 5:
			samprate = 16000;
			break;
		case 6:
			samprate = 22050;
			break;
		case 7:
			samprate = 24000;
			break;
		case 8:
			samprate = 32000;
			break;
		case 9:
			samprate = 44100;
			break;
		case 10:
			samprate = 48000;
			break;
		case 11:
			samprate = 64000;
			break;
		case 12:
			samprate = 88200;
			break;
		case 13:
			samprate = 96000;
			break;
		default:
			samprate = 44100;
			break;
	}

	audio_set_samprate(audio_ctx, samprate);
}

/*
 * audio channels list box changed event
 * args:
 *    combo - widget that generated the event
 *    data - pointer to user data
 *
 * asserts:
 *    none
 *
 * returns: none
 */
void audio_channels_changed(GtkComboBox *combo, void *data)
{
	int index = gtk_combo_box_get_active(combo);

	/*update the audio context for the new api*/
	audio_context_t *audio_ctx = get_audio_context();

	int channels = 0;

	switch(index)
	{
		case 0:
			channels = audio_get_device(audio_ctx, 
				audio_get_device_index(audio_ctx))->channels;
			break;
		case 1:
			channels = 1;
			break;
		default:
		case 2:
			channels = 2;
			break;
	}

	if(channels > audio_get_device(audio_ctx,
				audio_get_device_index(audio_ctx))->channels)
		channels = audio_get_device(audio_ctx,
				audio_get_device_index(audio_ctx))->channels;

	if(channels > 2)
		channels = 2; /*limit to stereo*/

	audio_set_channels(audio_ctx, channels);
}

/*
 * audio api list box changed event
 * args:
 *    combo - widget that generated the event
 *    data - pointer to user data
 *
 * asserts:
 *    none
 *
 * returns: none
 */
void audio_api_changed(GtkComboBox *combo, void *data)
{
	audio_widgets_t *my_audio_widgets = (audio_widgets_t *) data;

	int api = gtk_combo_box_get_active(combo);

	/*update the audio context for the new api*/
	audio_context_t *audio_ctx = create_audio_context(api, -1);
	if(!audio_ctx)
		api = AUDIO_NONE;
		
	/*update the config audio entry*/
	config_t *my_config = config_get();
	switch(api)
	{
		case AUDIO_NONE:
			strncpy(my_config->audio, "none", 5);
			break;
		case AUDIO_PULSE:
			strncpy(my_config->audio, "pulse", 5);
			break;
		default:
			strncpy(my_config->audio, "port", 5);
			break;
	}


	if(api == AUDIO_NONE || audio_ctx == NULL)
	{
		gtk_combo_box_set_active(combo, api);
		gtk_widget_set_sensitive(my_audio_widgets->device, FALSE);
		gtk_widget_set_sensitive(my_audio_widgets->channels, FALSE);
		gtk_widget_set_sensitive(my_audio_widgets->samprate, FALSE);
		gtk_widget_set_sensitive(my_audio_widgets->latency, FALSE);
	}
	else
	{
		g_signal_handlers_block_by_func(
			GTK_COMBO_BOX_TEXT(my_audio_widgets->device),
			G_CALLBACK (audio_device_changed),
			my_audio_widgets);

		/* clear out the old device list... */
		GtkListStore *store = GTK_LIST_STORE(gtk_combo_box_get_model (GTK_COMBO_BOX(my_audio_widgets->device)));
		gtk_list_store_clear(store);

		int i = 0;
		for(i = 0; i < audio_get_num_inp_devices(audio_ctx); ++i)
		{
			gtk_combo_box_text_append_text(
				GTK_COMBO_BOX_TEXT(my_audio_widgets->device),
				audio_get_device(audio_ctx, i)->description);
		}

		gtk_combo_box_set_active(GTK_COMBO_BOX(my_audio_widgets->device), 
			audio_get_device_index(audio_ctx));

		g_signal_handlers_unblock_by_func(
			GTK_COMBO_BOX_TEXT(my_audio_widgets->device),
			G_CALLBACK (audio_device_changed),
			my_audio_widgets);

		gtk_widget_set_sensitive (my_audio_widgets->device, TRUE);
		gtk_widget_set_sensitive(my_audio_widgets->channels, TRUE);
		gtk_widget_set_sensitive(my_audio_widgets->samprate, TRUE);
		gtk_widget_set_sensitive(my_audio_widgets->latency, TRUE);

		/*update channels*/
		int index = gtk_combo_box_get_active(GTK_COMBO_BOX(my_audio_widgets->channels));

		if(index == 0) /*auto*/
		{
			int channels = audio_get_device(audio_ctx, 
				audio_get_device_index(audio_ctx))->channels;
			audio_set_channels(audio_ctx, channels);
		}

		/*update samprate*/
		index = gtk_combo_box_get_active(GTK_COMBO_BOX(my_audio_widgets->samprate));

		if(index == 0) /*auto*/
		{
			int samprate = audio_get_device(audio_ctx, 
				audio_get_device_index(audio_ctx))->samprate;
			audio_set_samprate(audio_ctx, samprate);
		}

		gtk_range_set_value(GTK_RANGE(my_audio_widgets->latency), 
			audio_get_latency(audio_ctx));	
	}

}

/*
 * audio latency changed event
 * args:
 *    range - widget that generated the event
 *    data - pointer to user data
 *
 * asserts:
 *    none
 *
 * returns: none
 */
void audio_latency_changed(GtkRange *range, void *data)
{
	/**update the audio context for the new api*/
	audio_context_t *audio_ctx = get_audio_context();

	if(audio_ctx != NULL)
		audio_set_latency(audio_ctx, (double) gtk_range_get_value (range));
}

/*
 * video encoder properties clicked event
 * args:
 *    item - widget that generated the event
 *    data - pointer to user data
 *
 * asserts:
 *    none
 *
 * returns: none
 */
void encoder_video_properties(GtkMenuItem *item, void *data)
{
	int line = 0;
	video_codec_t *defaults = encoder_get_video_codec_defaults(get_video_codec_ind());

	GtkWidget *codec_dialog = gtk_dialog_new_with_buttons (_("video codec values"),
		GTK_WINDOW(get_main_window_gtk3()),
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		_("_OK"), GTK_RESPONSE_ACCEPT,
		_("_Cancel"), GTK_RESPONSE_REJECT,
		NULL);

	GtkWidget *table = gtk_grid_new();

	GtkWidget *lbl_fps = gtk_label_new(_("                              encoder fps:   \n (0 - use fps combobox value)"));
#if GTK_VER_AT_LEAST(3,15)
	gtk_label_set_xalign(GTK_LABEL(lbl_fps), 1);
	gtk_label_set_yalign(GTK_LABEL(lbl_fps), 0.5);
#else
	gtk_misc_set_alignment (GTK_MISC (lbl_fps), 1, 0.5);
#endif
	gtk_grid_attach (GTK_GRID(table), lbl_fps, 0, line, 1, 1);
	gtk_widget_show (lbl_fps);

	GtkWidget *enc_fps = gtk_spin_button_new_with_range(0,60,5);
	gtk_editable_set_editable(GTK_EDITABLE(enc_fps),TRUE);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(enc_fps), defaults->fps);

	gtk_grid_attach (GTK_GRID(table), enc_fps, 1, line, 1, 1);
	gtk_widget_show (enc_fps);
	line++;

	GtkWidget *monotonic_pts = gtk_check_button_new_with_label (_(" monotonic pts"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(monotonic_pts),(defaults->monotonic_pts != 0));

	gtk_grid_attach (GTK_GRID(table), monotonic_pts, 1, line, 1, 1);
	gtk_widget_show (monotonic_pts);
	line++;

	GtkWidget *lbl_bit_rate = gtk_label_new(_("bit rate:   "));
#if GTK_VER_AT_LEAST(3,15)
	gtk_label_set_xalign(GTK_LABEL(lbl_bit_rate), 1);
	gtk_label_set_yalign(GTK_LABEL(lbl_bit_rate), 0.5);
#else
	gtk_misc_set_alignment (GTK_MISC (lbl_bit_rate), 1, 0.5);
#endif
	gtk_grid_attach (GTK_GRID(table), lbl_bit_rate, 0, line, 1, 1);
	gtk_widget_show (lbl_bit_rate);

	GtkWidget *bit_rate = gtk_spin_button_new_with_range(160000,4000000,10000);
	gtk_editable_set_editable(GTK_EDITABLE(bit_rate),TRUE);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(bit_rate), defaults->bit_rate);

	gtk_grid_attach (GTK_GRID(table), bit_rate, 1, line, 1, 1);
	gtk_widget_show (bit_rate);
	line++;

	GtkWidget *lbl_qmax = gtk_label_new(_("qmax:   "));
#if GTK_VER_AT_LEAST(3,15)
	gtk_label_set_xalign(GTK_LABEL(lbl_qmax), 1);
	gtk_label_set_yalign(GTK_LABEL(lbl_qmax), 0.5);
#else
	gtk_misc_set_alignment (GTK_MISC (lbl_qmax), 1, 0.5);
#endif
	gtk_grid_attach (GTK_GRID(table), lbl_qmax, 0, line, 1 ,1);
	gtk_widget_show (lbl_qmax);

	GtkWidget *qmax = gtk_spin_button_new_with_range(1,60,1);
	gtk_editable_set_editable(GTK_EDITABLE(qmax),TRUE);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(qmax), defaults->qmax);

	gtk_grid_attach (GTK_GRID(table), qmax, 1, line, 1, 1);
	gtk_widget_show (qmax);
	line++;

	GtkWidget *lbl_qmin = gtk_label_new(_("qmin:   "));
#if GTK_VER_AT_LEAST(3,15)
	gtk_label_set_xalign(GTK_LABEL(lbl_qmin), 1);
	gtk_label_set_yalign(GTK_LABEL(lbl_qmin), 0.5);
#else
	gtk_misc_set_alignment (GTK_MISC (lbl_qmin), 1, 0.5);
#endif
	gtk_grid_attach (GTK_GRID(table), lbl_qmin, 0, line, 1, 1);
	gtk_widget_show (lbl_qmin);

	GtkWidget *qmin = gtk_spin_button_new_with_range(1,31,1);
	gtk_editable_set_editable(GTK_EDITABLE(qmin),TRUE);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(qmin), defaults->qmin);

	gtk_grid_attach (GTK_GRID(table), qmin, 1, line, 1, 1);
	gtk_widget_show (qmin);
	line++;

	GtkWidget *lbl_max_qdiff = gtk_label_new(_("max. qdiff:   "));
#if GTK_VER_AT_LEAST(3,15)
	gtk_label_set_xalign(GTK_LABEL(lbl_max_qdiff), 1);
	gtk_label_set_yalign(GTK_LABEL(lbl_max_qdiff), 0.5);
#else
	gtk_misc_set_alignment (GTK_MISC (lbl_max_qdiff), 1, 0.5);
#endif
	gtk_grid_attach (GTK_GRID(table), lbl_max_qdiff, 0, line, 1, 1);
	gtk_widget_show (lbl_max_qdiff);

	GtkWidget *max_qdiff = gtk_spin_button_new_with_range(1,4,1);
	gtk_editable_set_editable(GTK_EDITABLE(max_qdiff),TRUE);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(max_qdiff), defaults->max_qdiff);

	gtk_grid_attach (GTK_GRID(table), max_qdiff, 1, line, 1, 1);
	gtk_widget_show (max_qdiff);
	line++;

	GtkWidget *lbl_dia = gtk_label_new(_("dia size:   "));
#if GTK_VER_AT_LEAST(3,15)
	gtk_label_set_xalign(GTK_LABEL(lbl_dia), 1);
	gtk_label_set_yalign(GTK_LABEL(lbl_dia), 0.5);
#else
	gtk_misc_set_alignment (GTK_MISC (lbl_dia), 1, 0.5);
#endif
	gtk_grid_attach (GTK_GRID(table), lbl_dia, 0, line, 1, 1);
	gtk_widget_show (lbl_dia);

	GtkWidget *dia = gtk_spin_button_new_with_range(-1,4,1);
	gtk_editable_set_editable(GTK_EDITABLE(dia),TRUE);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(dia), defaults->dia);

	gtk_grid_attach (GTK_GRID(table), dia, 1, line, 1, 1);
	gtk_widget_show (dia);
	line++;

	GtkWidget *lbl_pre_dia = gtk_label_new(_("pre dia size:   "));
#if GTK_VER_AT_LEAST(3,15)
	gtk_label_set_xalign(GTK_LABEL(lbl_pre_dia), 1);
	gtk_label_set_yalign(GTK_LABEL(lbl_pre_dia), 0.5);
#else
	gtk_misc_set_alignment (GTK_MISC (lbl_pre_dia), 1, 0.5);
#endif
	gtk_grid_attach (GTK_GRID(table), lbl_pre_dia, 0, line, 1, 1);
	gtk_widget_show (lbl_pre_dia);

	GtkWidget *pre_dia = gtk_spin_button_new_with_range(1,4,1);
	gtk_editable_set_editable(GTK_EDITABLE(pre_dia),TRUE);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(pre_dia), defaults->pre_dia);

	gtk_grid_attach (GTK_GRID(table), pre_dia, 1, line, 1, 1);
	gtk_widget_show (pre_dia);
	line++;

	GtkWidget *lbl_pre_me = gtk_label_new(_("pre me:   "));
#if GTK_VER_AT_LEAST(3,15)
	gtk_label_set_xalign(GTK_LABEL(lbl_pre_me), 1);
	gtk_label_set_yalign(GTK_LABEL(lbl_pre_me), 0.5);
#else
	gtk_misc_set_alignment (GTK_MISC (lbl_pre_me), 1, 0.5);
#endif
	gtk_grid_attach (GTK_GRID(table), lbl_pre_me, 0, line, 1, 1);
	gtk_widget_show (lbl_pre_me);

	GtkWidget *pre_me = gtk_spin_button_new_with_range(0,2,1);
	gtk_editable_set_editable(GTK_EDITABLE(pre_me),TRUE);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(pre_me), defaults->pre_me);

	gtk_grid_attach (GTK_GRID(table), pre_me, 1, line, 1, 1);
	gtk_widget_show (pre_me);
	line++;

	GtkWidget *lbl_me_pre_cmp = gtk_label_new(_("pre cmp:   "));
#if GTK_VER_AT_LEAST(3,15)
	gtk_label_set_xalign(GTK_LABEL(lbl_me_pre_cmp), 1);
	gtk_label_set_yalign(GTK_LABEL(lbl_me_pre_cmp), 0.5);
#else
	gtk_misc_set_alignment (GTK_MISC (lbl_me_pre_cmp), 1, 0.5);
#endif
	gtk_grid_attach (GTK_GRID(table), lbl_me_pre_cmp, 0, line, 1, 1);
	gtk_widget_show (lbl_me_pre_cmp);

	GtkWidget *me_pre_cmp = gtk_spin_button_new_with_range(0,6,1);
	gtk_editable_set_editable(GTK_EDITABLE(me_pre_cmp),TRUE);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(me_pre_cmp), defaults->me_pre_cmp);

	gtk_grid_attach (GTK_GRID(table), me_pre_cmp, 1, line, 1, 1);
	gtk_widget_show (me_pre_cmp);
	line++;

	GtkWidget *lbl_me_cmp = gtk_label_new(_("cmp:   "));
#if GTK_VER_AT_LEAST(3,15)
	gtk_label_set_xalign(GTK_LABEL(lbl_me_cmp), 1);
	gtk_label_set_yalign(GTK_LABEL(lbl_me_cmp), 0.5);
#else
	gtk_misc_set_alignment (GTK_MISC (lbl_me_cmp), 1, 0.5);
#endif
	gtk_grid_attach (GTK_GRID(table), lbl_me_cmp, 0, line, 1, 1);
	gtk_widget_show (lbl_me_cmp);

	GtkWidget *me_cmp = gtk_spin_button_new_with_range(0,6,1);
	gtk_editable_set_editable(GTK_EDITABLE(me_cmp),TRUE);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(me_cmp), defaults->me_cmp);

	gtk_grid_attach (GTK_GRID(table), me_cmp, 1, line, 1, 1);
	gtk_widget_show (me_cmp);
	line++;

	GtkWidget *lbl_me_sub_cmp = gtk_label_new(_("sub cmp:   "));
#if GTK_VER_AT_LEAST(3,15)
	gtk_label_set_xalign(GTK_LABEL(lbl_me_sub_cmp), 1);
	gtk_label_set_yalign(GTK_LABEL(lbl_me_sub_cmp), 0.5);
#else
	gtk_misc_set_alignment (GTK_MISC (lbl_me_sub_cmp), 1, 0.5);
#endif
	gtk_grid_attach (GTK_GRID(table), lbl_me_sub_cmp, 0, line, 1, 1);
	gtk_widget_show (lbl_me_sub_cmp);

	GtkWidget *me_sub_cmp = gtk_spin_button_new_with_range(0,6,1);
	gtk_editable_set_editable(GTK_EDITABLE(me_sub_cmp),TRUE);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(me_sub_cmp), defaults->me_sub_cmp);

	gtk_grid_attach (GTK_GRID(table), me_sub_cmp, 1, line, 1, 1);
	gtk_widget_show (me_sub_cmp);
	line++;

	GtkWidget *lbl_last_pred = gtk_label_new(_("last predictor count:   "));
#if GTK_VER_AT_LEAST(3,15)
	gtk_label_set_xalign(GTK_LABEL(lbl_last_pred), 1);
	gtk_label_set_yalign(GTK_LABEL(lbl_last_pred), 0.5);
#else
	gtk_misc_set_alignment (GTK_MISC (lbl_last_pred), 1, 0.5);
#endif
	gtk_grid_attach (GTK_GRID(table), lbl_last_pred, 0, line, 1, 1);
	gtk_widget_show (lbl_last_pred);

	GtkWidget *last_pred = gtk_spin_button_new_with_range(1,3,1);
	gtk_editable_set_editable(GTK_EDITABLE(last_pred),TRUE);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(last_pred), defaults->last_pred);

	gtk_grid_attach (GTK_GRID(table), last_pred, 1, line, 1, 1);
	gtk_widget_show (last_pred);
	line++;

	GtkWidget *lbl_gop_size = gtk_label_new(_("gop size:   "));
#if GTK_VER_AT_LEAST(3,15)
	gtk_label_set_xalign(GTK_LABEL(lbl_gop_size), 1);
	gtk_label_set_yalign(GTK_LABEL(lbl_gop_size), 0.5);
#else
	gtk_misc_set_alignment (GTK_MISC (lbl_gop_size), 1, 0.5);
#endif
	gtk_grid_attach (GTK_GRID(table), lbl_gop_size, 0, line, 1, 1);
	gtk_widget_show (lbl_gop_size);

	GtkWidget *gop_size = gtk_spin_button_new_with_range(1,250,1);
	gtk_editable_set_editable(GTK_EDITABLE(gop_size),TRUE);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(gop_size), defaults->gop_size);

	gtk_grid_attach (GTK_GRID(table), gop_size, 1, line, 1, 1);
	gtk_widget_show (gop_size);
	line++;

	GtkWidget *lbl_qcompress = gtk_label_new(_("qcompress:   "));
#if GTK_VER_AT_LEAST(3,15)
	gtk_label_set_xalign(GTK_LABEL(lbl_qcompress), 1);
	gtk_label_set_yalign(GTK_LABEL(lbl_qcompress), 0.5);
#else
	gtk_misc_set_alignment (GTK_MISC (lbl_qcompress), 1, 0.5);
#endif
	gtk_grid_attach (GTK_GRID(table), lbl_qcompress, 0, line, 1, 1);
	gtk_widget_show (lbl_qcompress);

	GtkWidget *qcompress = gtk_spin_button_new_with_range(0,1,0.1);
	gtk_editable_set_editable(GTK_EDITABLE(qcompress),TRUE);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(qcompress), defaults->qcompress);

	gtk_grid_attach (GTK_GRID(table), qcompress, 1, line, 1 ,1);
	gtk_widget_show (qcompress);
	line++;

	GtkWidget *lbl_qblur = gtk_label_new(_("qblur:   "));
#if GTK_VER_AT_LEAST(3,15)
	gtk_label_set_xalign(GTK_LABEL(lbl_qblur), 1);
	gtk_label_set_yalign(GTK_LABEL(lbl_qblur), 0.5);
#else
	gtk_misc_set_alignment (GTK_MISC (lbl_qblur), 1, 0.5);
#endif
	gtk_grid_attach (GTK_GRID(table), lbl_qblur, 0, line, 1 ,1);
	gtk_widget_show (lbl_qblur);

	GtkWidget *qblur = gtk_spin_button_new_with_range(0,1,0.1);
	gtk_editable_set_editable(GTK_EDITABLE(qblur),TRUE);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(qblur), defaults->qblur);

	gtk_grid_attach (GTK_GRID(table), qblur, 1, line, 1 ,1);
	gtk_widget_show (qblur);
	line++;

	GtkWidget *lbl_subq = gtk_label_new(_("subq:   "));
#if GTK_VER_AT_LEAST(3,15)
	gtk_label_set_xalign(GTK_LABEL(lbl_subq), 1);
	gtk_label_set_yalign(GTK_LABEL(lbl_subq), 0.5);
#else
	gtk_misc_set_alignment (GTK_MISC (lbl_subq), 1, 0.5);
#endif
	gtk_grid_attach (GTK_GRID(table), lbl_subq, 0, line, 1 ,1);
	gtk_widget_show (lbl_subq);

	GtkWidget *subq = gtk_spin_button_new_with_range(0,8,1);
	gtk_editable_set_editable(GTK_EDITABLE(subq),TRUE);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(subq), defaults->subq);

	gtk_grid_attach (GTK_GRID(table), subq, 1, line, 1 ,1);
	gtk_widget_show (subq);
	line++;

	GtkWidget *lbl_framerefs = gtk_label_new(_("framerefs:   "));
#if GTK_VER_AT_LEAST(3,15)
	gtk_label_set_xalign(GTK_LABEL(lbl_framerefs), 1);
	gtk_label_set_yalign(GTK_LABEL(lbl_framerefs), 0.5);
#else
	gtk_misc_set_alignment (GTK_MISC (lbl_framerefs), 1, 0.5);
#endif
	gtk_grid_attach (GTK_GRID(table), lbl_framerefs, 0, line, 1 ,1);
	gtk_widget_show (lbl_framerefs);

	GtkWidget *framerefs = gtk_spin_button_new_with_range(0,12,1);
	gtk_editable_set_editable(GTK_EDITABLE(framerefs),TRUE);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(framerefs), defaults->framerefs);

	gtk_grid_attach (GTK_GRID(table), framerefs, 1, line, 1 ,1);
	gtk_widget_show (framerefs);
	line++;

	GtkWidget *lbl_me_method = gtk_label_new(_("me method:   "));
#if GTK_VER_AT_LEAST(3,15)
	gtk_label_set_xalign(GTK_LABEL(lbl_me_method), 1);
	gtk_label_set_yalign(GTK_LABEL(lbl_me_method), 0.5);
#else
	gtk_misc_set_alignment (GTK_MISC (lbl_me_method), 1, 0.5);
#endif
	gtk_grid_attach (GTK_GRID(table), lbl_me_method, 0, line, 1 ,1);
	gtk_widget_show (lbl_me_method);

	GtkWidget *me_method = gtk_spin_button_new_with_range(1,10,1);
	gtk_editable_set_editable(GTK_EDITABLE(me_method),TRUE);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(me_method), defaults->me_method);

	gtk_grid_attach (GTK_GRID(table), me_method, 1, line, 1 ,1);
	gtk_widget_show (me_method);
	line++;

	GtkWidget *lbl_mb_decision = gtk_label_new(_("mb decision:   "));
#if GTK_VER_AT_LEAST(3,15)
	gtk_label_set_xalign(GTK_LABEL(lbl_mb_decision), 1);
	gtk_label_set_yalign(GTK_LABEL(lbl_mb_decision), 0.5);
#else
	gtk_misc_set_alignment (GTK_MISC (lbl_mb_decision), 1, 0.5);
#endif
	gtk_grid_attach (GTK_GRID(table), lbl_mb_decision, 0, line, 1 ,1);
	gtk_widget_show (lbl_mb_decision);

	GtkWidget *mb_decision = gtk_spin_button_new_with_range(0,2,1);
	gtk_editable_set_editable(GTK_EDITABLE(mb_decision),TRUE);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(mb_decision), defaults->mb_decision);

	gtk_grid_attach (GTK_GRID(table), mb_decision, 1, line, 1 ,1);
	gtk_widget_show (mb_decision);
	line++;

	GtkWidget *lbl_max_b_frames = gtk_label_new(_("max B frames:   "));
#if GTK_VER_AT_LEAST(3,15)
	gtk_label_set_xalign(GTK_LABEL(lbl_max_b_frames), 1);
	gtk_label_set_yalign(GTK_LABEL(lbl_max_b_frames), 0.5);
#else
	gtk_misc_set_alignment (GTK_MISC (lbl_max_b_frames), 1, 0.5);
#endif
	gtk_grid_attach (GTK_GRID(table), lbl_max_b_frames, 0, line, 1 ,1);
	gtk_widget_show (lbl_max_b_frames);

	GtkWidget *max_b_frames = gtk_spin_button_new_with_range(0,4,1);
	gtk_editable_set_editable(GTK_EDITABLE(max_b_frames),TRUE);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(max_b_frames), defaults->max_b_frames);

	gtk_grid_attach (GTK_GRID(table), max_b_frames, 1, line, 1 ,1);
	gtk_widget_show (max_b_frames);
	line++;

	GtkWidget *lbl_num_threads = gtk_label_new(_("num threads:   "));
#if GTK_VER_AT_LEAST(3,15)
	gtk_label_set_xalign(GTK_LABEL(lbl_num_threads), 1);
	gtk_label_set_yalign(GTK_LABEL(lbl_num_threads), 0.5);
#else
	gtk_misc_set_alignment (GTK_MISC (lbl_num_threads), 1, 0.5);
#endif
	gtk_grid_attach (GTK_GRID(table), lbl_num_threads, 0, line, 1 ,1);
	gtk_widget_show (lbl_num_threads);

	GtkWidget *num_threads = gtk_spin_button_new_with_range(0,8,1);
	gtk_editable_set_editable(GTK_EDITABLE(num_threads),TRUE);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(num_threads), defaults->num_threads);

	gtk_grid_attach (GTK_GRID(table), num_threads, 1, line, 1 ,1);
	gtk_widget_show (num_threads);
	line++;

	GtkWidget *content_area = gtk_dialog_get_content_area (GTK_DIALOG (codec_dialog));
	gtk_container_add (GTK_CONTAINER (content_area), table);
	gtk_widget_show (table);

	gint result = gtk_dialog_run (GTK_DIALOG (codec_dialog));
	switch (result)
	{
		case GTK_RESPONSE_ACCEPT:
			defaults->fps = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(enc_fps));
			defaults->monotonic_pts = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(monotonic_pts));
			defaults->bit_rate = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(bit_rate));
			defaults->qmax = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(qmax));
			defaults->qmin = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(qmin));
			defaults->max_qdiff = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(max_qdiff));
			defaults->dia = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(dia));
			defaults->pre_dia = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(pre_dia));
			defaults->pre_me = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(pre_me));
			defaults->me_pre_cmp = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(me_pre_cmp));
			defaults->me_cmp = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(me_cmp));
			defaults->me_sub_cmp = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(me_sub_cmp));
			defaults->last_pred = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(last_pred));
			defaults->gop_size = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(gop_size));
			defaults->qcompress = (float) gtk_spin_button_get_value (GTK_SPIN_BUTTON(qcompress));
			defaults->qblur = (float) gtk_spin_button_get_value (GTK_SPIN_BUTTON(qblur));
			defaults->subq = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(subq));
			defaults->framerefs = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(framerefs));
			defaults->me_method = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(me_method));
			defaults->mb_decision = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(mb_decision));
			defaults->max_b_frames = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(max_b_frames));
			defaults->num_threads = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(num_threads));
			break;
		default:
			// do nothing since dialog was cancelled
			break;
	}
	gtk_widget_destroy (codec_dialog);
}

/*
 * audio encoder properties clicked event
 * args:
 *    item - widget that generated the event
 *    data - pointer to user data
 *
 * asserts:
 *    none
 *
 * returns: none
 */
void encoder_audio_properties(GtkMenuItem *item, void *data)
{
	int line = 0;
	audio_codec_t *defaults = encoder_get_audio_codec_defaults(get_audio_codec_ind());

	GtkWidget *codec_dialog = gtk_dialog_new_with_buttons (_("audio codec values"),
		GTK_WINDOW(get_main_window_gtk3()),
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		_("_Ok"), GTK_RESPONSE_ACCEPT,
		_("_Cancel"), GTK_RESPONSE_REJECT,
		NULL);

	GtkWidget *table = gtk_grid_new();
	gtk_grid_set_column_homogeneous (GTK_GRID(table), TRUE);

	/*bit rate*/
	GtkWidget *lbl_bit_rate = gtk_label_new(_("bit rate:   "));
#if GTK_VER_AT_LEAST(3,15)
	gtk_label_set_xalign(GTK_LABEL(lbl_bit_rate), 1);
	gtk_label_set_yalign(GTK_LABEL(lbl_bit_rate), 0.5);
#else
	gtk_misc_set_alignment (GTK_MISC (lbl_bit_rate), 1, 0.5);
#endif
	gtk_grid_attach (GTK_GRID(table), lbl_bit_rate, 0, line, 1, 1);
	gtk_widget_show (lbl_bit_rate);

	GtkWidget *bit_rate = gtk_spin_button_new_with_range(48000,384000,8000);
	gtk_editable_set_editable(GTK_EDITABLE(bit_rate),TRUE);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(bit_rate), defaults->bit_rate);

	gtk_grid_attach (GTK_GRID(table), bit_rate, 1, line, 1, 1);
	gtk_widget_show (bit_rate);
	line++;

	/*sample format*/
	GtkWidget *lbl_sample_fmt = gtk_label_new(_("sample format:   "));
#if GTK_VER_AT_LEAST(3,15)
	gtk_label_set_xalign(GTK_LABEL(lbl_sample_fmt), 1);
	gtk_label_set_yalign(GTK_LABEL(lbl_sample_fmt), 0.5);
#else
	gtk_misc_set_alignment (GTK_MISC (lbl_sample_fmt), 1, 0.5);
#endif
	gtk_grid_attach (GTK_GRID(table), lbl_sample_fmt, 0, line, 1, 1);
	gtk_widget_show (lbl_sample_fmt);

	GtkWidget *sample_fmt = gtk_spin_button_new_with_range(0, encoder_get_max_audio_sample_fmt(), 1);
	gtk_editable_set_editable(GTK_EDITABLE(sample_fmt),TRUE);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(sample_fmt), defaults->sample_format);

	gtk_grid_attach (GTK_GRID(table), sample_fmt, 1, line, 1, 1);
	gtk_widget_show (sample_fmt);
	line++;

	GtkWidget *content_area = gtk_dialog_get_content_area (GTK_DIALOG (codec_dialog));
	gtk_container_add (GTK_CONTAINER (content_area), table);
	gtk_widget_show (table);

	gint result = gtk_dialog_run (GTK_DIALOG (codec_dialog));
	switch (result)
	{
		case GTK_RESPONSE_ACCEPT:
			defaults->bit_rate = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(bit_rate));
			defaults->sample_format = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(sample_fmt));
			break;
		default:
			// do nothing since dialog was cancelled
			break;
	}
	gtk_widget_destroy (codec_dialog);
}

/*
 * gtk3 entry focus in or out event
 * args:
 *   entry - pointer to widget that received event
 *   event - pointer to GDK key event structure
 *   data - pointer to user data
 *
 * asserts:
 *   none
 *
 * returns: true if we handled the event or false otherwise
 */
gboolean entry_focus (GtkWidget *entry, GdkEventKey *event, void *data)
{
	int val = GPOINTER_TO_INT(data);
	//if we have an entry with focus disable window_key_pressed events
	entry_has_focus = val;
	if(debug_level > 1)
	printf("GUVCIVEW: entry focus changed to %i \n", entry_has_focus);
	
	return FALSE;
}

/*
 * gtk3 window key pressed event
 * args:
 *   win - pointer to widget (main window) where event ocurred
 *   event - pointer to GDK key event structure
 *   data - pointer to user data
 *
 * asserts:
 *   none
 *
 * returns: true if we handled the event or false otherwise
 */
gboolean window_key_pressed (GtkWidget *win, GdkEventKey *event, void *data)
{
	/* If we have modifiers, and either Ctrl, Mod1 (Alt), or any
	 * of Mod3 to Mod5 (Mod2 is num-lock...) are pressed, we
	 * let Gtk+ handle the key */
	//printf("GUVCVIEW: key pressed (key:%i)\n", event->keyval);
	if (event->state != 0
		&& ((event->state & GDK_CONTROL_MASK)
		|| (event->state & GDK_MOD1_MASK)
		|| (event->state & GDK_MOD3_MASK)
		|| (event->state & GDK_MOD4_MASK)
		|| (event->state & GDK_MOD5_MASK)))
		return FALSE;

	if(entry_has_focus)
		return FALSE;

    if(v4l2core_has_pantilt_id(get_v4l2_device_handler()))
    {
		int id = 0;
		int value = 0;

        switch (event->keyval)
        {
            case GDK_KEY_Down:
            case GDK_KEY_KP_Down:
				id = V4L2_CID_TILT_RELATIVE;
				value = v4l2core_get_tilt_step(get_v4l2_device_handler());
				break;

            case GDK_KEY_Up:
            case GDK_KEY_KP_Up:
				id = V4L2_CID_TILT_RELATIVE;
				value = - v4l2core_get_tilt_step(get_v4l2_device_handler());
				break;

            case GDK_KEY_Left:
            case GDK_KEY_KP_Left:
				id = V4L2_CID_PAN_RELATIVE;
				value = v4l2core_get_pan_step(get_v4l2_device_handler());
				break;

            case GDK_KEY_Right:
            case GDK_KEY_KP_Right:
                id = V4L2_CID_PAN_RELATIVE;
				value = - v4l2core_get_pan_step(get_v4l2_device_handler());
				break;

            default:
                break;
        }

        if(id != 0 && value != 0)
        {
			v4l2_ctrl_t *control = v4l2core_get_control_by_id(get_v4l2_device_handler(), id);

			if(control)
			{
				control->value =  value;

				if(v4l2core_set_control_value_by_id(get_v4l2_device_handler(), id))
					fprintf(stderr, "GUVCVIEW: error setting pan/tilt value\n");

				return TRUE;
			}
		}
    }

    switch (event->keyval)
    {
        case GDK_KEY_WebCam:
			/* camera button pressed */
			if (get_default_camera_button_action() == DEF_ACTION_IMAGE)
				gui_click_image_capture_button_gtk3();
			else
				gui_click_video_capture_button_gtk3();
			return TRUE;

		case GDK_KEY_V:
		case GDK_KEY_v:
			gui_click_video_capture_button_gtk3();
			return TRUE;

		case GDK_KEY_I:
		case GDK_KEY_i:
			gui_click_image_capture_button_gtk3();
			return TRUE;

	}

    return FALSE;
}

/*
 * device list events timer callback
 * args:
 *   data - pointer to user data
 *
 * asserts:
 *   none
 *
 * returns: true if timer is to be reset or false otherwise
 */
gboolean check_device_events(gpointer data)
{
	if(v4l2core_check_device_list_events())
	{
		/*update device list*/
		g_signal_handlers_block_by_func(GTK_COMBO_BOX_TEXT(get_wgtDevices_gtk3()),
                G_CALLBACK (devices_changed), NULL);

		GtkListStore *store = GTK_LIST_STORE(gtk_combo_box_get_model (GTK_COMBO_BOX(get_wgtDevices_gtk3())));
		gtk_list_store_clear(store);

		int i = 0;
        for(i = 0; i < v4l2core_get_num_devices(); i++)
		{
			gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(get_wgtDevices_gtk3()),
				v4l2core_get_device_sys_data(i)->name);
			if(v4l2core_get_device_sys_data(i)->current)
				gtk_combo_box_set_active(GTK_COMBO_BOX(get_wgtDevices_gtk3()),i);
		}

		g_signal_handlers_unblock_by_func(GTK_COMBO_BOX_TEXT(get_wgtDevices_gtk3()),
                G_CALLBACK (devices_changed), NULL);
	}

	return (TRUE);
}

/*
 * control events timer callback
 * args:
 *   data - pointer to user data
 *
 * asserts:
 *   none
 *
 * returns: true if timer is to be reset or false otherwise
 */
gboolean check_control_events(gpointer data)
{
	if(v4l2core_check_control_events(get_v4l2_device_handler()) > 0)
	{
		/*update the control list*/
		gui_gtk3_update_controls_state();
	}

	return (TRUE);
}
