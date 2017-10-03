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
#include "video_capture.h"

extern int debug_level;
extern int is_control_panel;

/* The main window*/
static GtkWidget *main_window = NULL;
/* The status bar*/
static GtkWidget *status_bar = NULL;
static int status_warning_id = 0;
/*The photo capture button*/
static GtkWidget *CapImageButt = NULL;
/*The video capture button*/
static GtkWidget *CapVideoButt = NULL;
/*group list for menu video codecs*/
GSList *video_codec_group = NULL;
/*group list for menu audio codecs*/
GSList *audio_codec_group = NULL;
/*flag gtk3_main call*/
static int gtk_main_called = 0;
/*flag gtk3_init called*/
static int gtk_init_called = 0;
/*device list widget*/
static GtkWidget *wgtDevices = NULL;
/*timer id for devce list events check*/
static int gtk_devices_timer_id = 0;
/*timer id for control events check*/
static int gtk_control_events_timer_id = 0;


/*
 * sets the status message
 * args:
 *   message - message string
 * 
 * returns: FALSE
 */
static gboolean set_status_message(const char *message)
{
	if(status_bar)
	{
		gtk_statusbar_pop (GTK_STATUSBAR(status_bar), status_warning_id);
		gtk_statusbar_push (GTK_STATUSBAR(status_bar), status_warning_id, message);
	}
	
	/*execute only once*/
	return FALSE;
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
void gui_status_message_gtk3(const char *message)
{
	/*this maybe called from a different thread, so protect it*/
	gdk_threads_add_idle ((GSourceFunc)set_status_message, (gpointer) message);
}

/*
 * get the main window widget
 * args:
 *    none
 *
 * asserts:
 *    none
 *
 * returns: pointer to main window (GtkWidget)
 */
GtkWidget *get_main_window_gtk3()
{
	return main_window;
}

/*
 * get the device list widget
 * args:
 *    none
 *
 * asserts:
 *    none
 *
 * returns: pointer to the device list widget (GtkWidget)
 */
GtkWidget *get_wgtDevices_gtk3()
{
	return wgtDevices;
}

/*
 * set the device list widget
 * args:
 *    widget - pointer to the device list widget
 *
 * asserts:
 *    none
 *
 * returns: void
 */
void set_wgtDevices_gtk3(GtkWidget *widget)
{
	wgtDevices = widget;
}

/*
 * get the video codec group list
 * args:
 *    none
 *
 * asserts:
 *    none
 *
 * returns: pointer to GSList of video codecs
 */
GSList *get_video_codec_group_list_gtk3()
{
	return video_codec_group;
}

/*
 * set the video codec group list
 * args:
 *    list - pointer to GSList
 *
 * asserts:
 *    none
 *
 * returns: none
 */
void set_video_codec_group_list_gtk3(GSList *list)
{
	video_codec_group = list;
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
void set_webm_codecs_gtk3()
{
	/*force webm codecs*/
	int video_codec_ind = encoder_get_webm_video_codec_index();
	set_video_codec_ind(video_codec_ind);
	int audio_codec_ind = encoder_get_webm_audio_codec_index();
	set_audio_codec_ind(audio_codec_ind);

	/*widgets*/
	GSList *vgroup = get_video_codec_group_list_gtk3();
	int index = g_slist_length (vgroup) - (get_video_codec_ind() + 1);
	GtkWidget* video_codec_item = g_slist_nth_data (vgroup, index);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(video_codec_item), TRUE);

	GSList *agroup = get_audio_codec_group_list_gtk3();
	index = g_slist_length (agroup) - (get_audio_codec_ind() + 1);
	GtkWidget* audio_codec_item = g_slist_nth_data (agroup, index);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(audio_codec_item), TRUE);
}

/*
 * get the audio codec group list
 * args:
 *    none
 *
 * asserts:
 *    none
 *
 * returns: pointer to GSList of audio codecs
 */
GSList *get_audio_codec_group_list_gtk3()
{
	return audio_codec_group;
}

/*
 * set the audio codec group list
 * args:
 *    list - pointer to GSList
 *
 * asserts:
 *    none
 *
 * returns: none
 */
void set_audio_codec_group_list_gtk3(GSList *list)
{
	audio_codec_group = list;
}

/*
 * sends a click event for image capture button
 * args:
 *   pointer to function data
 *
 * asserts:
 *    none
 *
 * returns: FALSE
 */
static gboolean image_capture_toggle_button(gpointer *data)
{
	if(!CapImageButt)
		return FALSE;

	gtk_button_clicked(GTK_BUTTON(CapImageButt));
	
	return FALSE;
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
void gui_click_image_capture_button_gtk3()
{
	/*protect the call since it may come from a different thread*/
	gdk_threads_add_idle ((GSourceFunc) image_capture_toggle_button, NULL);
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
void gui_set_image_capture_button_label_gtk3(const char *label)
{
	if(!CapImageButt)
		return;

	gtk_button_set_label(GTK_BUTTON(CapImageButt), label);
}

/*
 * toggles video capture button status
 * args:
 *    pointer to function data
 *
 * asserts:
 *    none
 *
 * returns: FALSE
 */
static gboolean video_capture_toggle_button(gpointer *data)
{
	if(!CapVideoButt)
		return FALSE;

	int active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(CapVideoButt));
	/*invert status*/
    if(active > 0)
		active = -1;
	else
		active = 1;
    
	gui_set_video_capture_button_status_gtk3(active);
	
	return FALSE;
}

/*
 * click video capture button
 * args:
 *    none
 *
 * asserts:
 *    none
 *
 * returns: none
 */
void gui_click_video_capture_button_gtk3()
{
	/*protect the call since it may come from a different thread*/
	gdk_threads_add_idle ((GSourceFunc) video_capture_toggle_button, NULL);
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
void gui_set_video_capture_button_status_gtk3(int flag)
{
	if(!CapVideoButt)
		return;

	if(flag > 0)
	{
		gtk_button_set_label(GTK_BUTTON(CapVideoButt), _("Stop Video (V)"));
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(CapVideoButt), TRUE);
	}
	else
	{
		gtk_button_set_label(GTK_BUTTON(CapVideoButt), _("Cap. Video (V)"));
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(CapVideoButt), FALSE);
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
void gui_error_gtk3(
	const char *title,
	const char *message,
	int fatal)
{
	if(!gtk_init_called)
	{
		if(!gtk_init_check(NULL, NULL))
		{
			fprintf(stderr, "GUVCVIEW: (GUI) Gtk3 can't open display\n");
			fprintf(stderr, "%s: %s \n", title, message);
			return;
		}

		gtk_init_called = 1;
	}
	/*simple warning message - not fatal and no device selection*/
	if(!fatal)
	{
		GtkWidget *warndialog;
		warndialog = gtk_message_dialog_new (GTK_WINDOW(main_window),
		    GTK_DIALOG_DESTROY_WITH_PARENT,
		    GTK_MESSAGE_WARNING,
		    GTK_BUTTONS_CLOSE,
		    "%s",gettext(title));

		gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(warndialog),
		    "%s",gettext(message));

		gtk_widget_show(warndialog);
		gtk_dialog_run (GTK_DIALOG (warndialog));
		gtk_widget_destroy (warndialog);
		return;
	}

	/*fatal error message*/

	/*add device list (more than one device)*/
	int show_dev_list = (v4l2core_get_num_devices() >= 1) ? 1: 0;

	GtkWidget *errdialog = NULL;
	if(show_dev_list)
		errdialog = gtk_dialog_new_with_buttons (_("Error"),
			GTK_WINDOW(main_window),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			_("_Ok"), GTK_RESPONSE_ACCEPT,
			_("_Cancel"), GTK_RESPONSE_REJECT,
			NULL);
	else
		errdialog = gtk_dialog_new_with_buttons (_("Error"),
			GTK_WINDOW(main_window),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			_("_Ok"), GTK_RESPONSE_ACCEPT,
			NULL);

	GtkWidget *table = gtk_grid_new();

	GtkWidget *title_lbl = gtk_label_new (gettext(title));
	
#if GTK_VER_AT_LEAST(3,16)
	/* Style provider for this label. */
	GtkCssProvider *providerTitle = gtk_css_provider_new();

	gtk_style_context_add_provider(gtk_widget_get_style_context(title_lbl),
	        GTK_STYLE_PROVIDER(providerTitle),
	        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	gtk_css_provider_load_from_data(providerTitle,
	                                "GtkLabel { font: \"Sans bold 10\"; }",
	                                -1, NULL);
	g_object_unref(providerTitle);
#else
	gtk_widget_override_font(title_lbl, pango_font_description_from_string ("Sans bold 10"));
	gtk_misc_set_alignment (GTK_MISC (title_lbl), 0, 0);
#endif
	gtk_grid_attach (GTK_GRID (table), title_lbl, 0, 0, 2, 1);
	gtk_widget_show (title_lbl);

	GtkWidget *text = gtk_label_new (gettext(message));

#if GTK_VER_AT_LEAST(3,16)
	/* Style provider for this label. */
	GtkCssProvider *providerText = gtk_css_provider_new();

	gtk_style_context_add_provider(gtk_widget_get_style_context(text),
	        GTK_STYLE_PROVIDER(providerText),
	        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	gtk_css_provider_load_from_data(providerText,
	                                "GtkLabel { font: \"Sans italic 8\"; }",
	                                -1, NULL);
	g_object_unref(providerText);
#else
	gtk_widget_override_font(text, pango_font_description_from_string ("Sans italic 8"));
	gtk_misc_set_alignment (GTK_MISC (text), 0, 0);
#endif
	
	gtk_grid_attach (GTK_GRID (table), text, 0, 1, 2, 1);
	gtk_widget_show (text);

	GtkWidget *wgtDevices = NULL;

	if(show_dev_list)
	{
		GtkWidget *text2 = gtk_label_new (_("\nYou seem to have video devices installed.\n"
							                "Do you want to try one ?\n"));

#if GTK_VER_AT_LEAST(3,16)
	/* Style provider for this label. */
	GtkCssProvider *providerText2 = gtk_css_provider_new();

	gtk_style_context_add_provider(gtk_widget_get_style_context(text2),
	        GTK_STYLE_PROVIDER(providerText2),
	        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	gtk_css_provider_load_from_data(providerText2,
	                                "GtkLabel { font: \"Sans 10\"; }",
	                                -1, NULL);
	g_object_unref(providerText2);
#else
		gtk_widget_override_font(text2, pango_font_description_from_string ("Sans 10"));
		gtk_misc_set_alignment (GTK_MISC (text2), 0, 0);
#endif
		gtk_grid_attach (GTK_GRID (table), text2, 0, 2, 2, 1);
		gtk_widget_show (text2);

		GtkWidget *dev_lbl = gtk_label_new(_("Device:"));

#if GTK_VER_AT_LEAST(3,15)
		gtk_label_set_xalign(GTK_LABEL(dev_lbl), 0.5);
		gtk_label_set_yalign(GTK_LABEL(dev_lbl), 0.5);
#else
		gtk_misc_set_alignment (GTK_MISC (dev_lbl), 0.5, 0.5);
#endif
		gtk_grid_attach (GTK_GRID(table), dev_lbl, 0, 3, 1, 1);
		gtk_widget_show (dev_lbl);

		wgtDevices = gtk_combo_box_text_new ();
		gtk_widget_set_halign (wgtDevices, GTK_ALIGN_FILL);
		gtk_widget_set_hexpand (wgtDevices, TRUE);

		int i = 0;
		for(i = 0; i < v4l2core_get_num_devices(); i++)
		{
			gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(wgtDevices),
				v4l2core_get_device_sys_data(i)->name);
		}
		/*select the last listed device by default*/
		gtk_combo_box_set_active(GTK_COMBO_BOX(wgtDevices), v4l2core_get_num_devices() - 1);

		gtk_grid_attach(GTK_GRID(table), wgtDevices, 1, 3, 1, 1);
		gtk_widget_show (wgtDevices);
	}

	GtkWidget *content_area = gtk_dialog_get_content_area (GTK_DIALOG (errdialog));
	gtk_container_add (GTK_CONTAINER (content_area), table);
	gtk_widget_show (table);

	int result = gtk_dialog_run (GTK_DIALOG (errdialog));

	if(show_dev_list)
	{
		switch (result)
		{
			case GTK_RESPONSE_ACCEPT:
			{
				/*launch another guvcview instance for the selected device*/
				int index = gtk_combo_box_get_active(GTK_COMBO_BOX(wgtDevices));

				char videodevice[30];
				strncpy(videodevice, v4l2core_get_device_sys_data(index)->device, 29);

				gchar *command = g_strjoin("",
					g_get_prgname(),
					" --device=",
					videodevice,
					NULL);

				/*spawn new process*/
				GError *error = NULL;
				if(!(g_spawn_command_line_async(command, &error)))
				{
					fprintf(stderr, "GUVCVIEW: spawn failed: %s\n", error->message);
					g_error_free( error );
				}
				g_free(command);
			}
			break;

			default:
				/* do nothing since dialog was cancelled or closed */
				break;

		}
	}

	gtk_widget_destroy (errdialog);

	quit_callback(NULL); /*terminate the program*/
}

/*
 * GUI initialization
 * args:
 *   width - window width
 *   height - window height
 *
 * asserts:
 *   none
 *
 * returns: error code (0 -OK)
 */
int gui_attach_gtk3(int width, int height)
{
	if(!gtk_init_called)
	{
		if(!gtk_init_check(NULL, NULL))
		{
			fprintf(stderr, "GUVCVIEW: (GUI) Gtk3 can't open display\n");
			return -1;
		}

		gtk_init_called = 1;
	}


	/*check for device errors*/
	//if(!device)
	//{
	//	gui_error("Guvcview error", "no video device found", 1);
	//	return -1;
	//}

	g_set_application_name(_("Guvcview Video Capture"));

#if !GTK_VER_AT_LEAST(3,12)
	/* make sure the type is realized so that we can change the properties*/
	g_type_class_unref (g_type_class_ref (GTK_TYPE_BUTTON));
	/* make sure gtk-button-images property is set to true (defaults to false in karmic)*/
	g_object_set (gtk_settings_get_default (), "gtk-button-images", TRUE, NULL);
#endif

	/* Create a main window */
	main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (main_window), _("Guvcview"));
	gtk_widget_show (main_window);

	/* get screen resolution */
	GdkScreen* screen = NULL;
	screen = gtk_window_get_screen(GTK_WINDOW(main_window));
	int desktop_width = gdk_screen_get_width(screen);
	int desktop_height = gdk_screen_get_height(screen);

	if(debug_level > 0)
		printf("GUVCVIEW: (GUI) Screen resolution is (%d x %d)\n", desktop_width, desktop_height);

	if((width > desktop_width) && (desktop_width > 0))
		width = desktop_width;
	if((height > desktop_height) && (desktop_height > 0))
		height = desktop_height;

	gtk_window_resize(GTK_WINDOW(main_window), width, height);

	/* Add delete event handler */
	g_signal_connect(GTK_WINDOW(main_window), "delete_event", G_CALLBACK(delete_event), NULL);

	/*window icon*/
	char* icon1path = g_strconcat (PACKAGE_DATA_DIR, "/pixmaps/guvcview/guvcview.png", NULL);
	if (g_file_test(icon1path, G_FILE_TEST_EXISTS))
		gtk_window_set_icon_from_file(GTK_WINDOW (main_window), icon1path, NULL);
	g_free(icon1path);

	/*---------------------------- Main table ---------------------------------*/
	GtkWidget *maintable = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
	gtk_widget_show (maintable);

	/*----------------------------- Top Menu ----------------------------------*/
	gui_attach_gtk3_menu(maintable);

	/*----------------------------- Buttons -----------------------------------*/
	GtkWidget *HButtonBox = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_widget_set_halign (HButtonBox, GTK_ALIGN_FILL);
	gtk_widget_set_hexpand (HButtonBox, TRUE);
	gtk_button_box_set_layout(GTK_BUTTON_BOX(HButtonBox),GTK_BUTTONBOX_SPREAD);
	gtk_box_set_homogeneous(GTK_BOX(HButtonBox),TRUE);
	gtk_widget_show(HButtonBox);

	/*photo button*/
	if(check_photo_timer())
	{
		CapImageButt = gtk_button_new_with_mnemonic (_("Stop Cap. (I)"));
		g_object_set_data (G_OBJECT (CapImageButt), "control_info",
							GINT_TO_POINTER(1));
	}
	else
	{
		CapImageButt = gtk_button_new_with_mnemonic (_("Cap. Image (I)"));
		g_object_set_data (G_OBJECT (CapImageButt), "control_info",
							GINT_TO_POINTER(0));
	}

	char *pix2path = g_strconcat (PACKAGE_DATA_DIR, "/pixmaps/guvcview/camera.png",NULL);
	if (g_file_test(pix2path, G_FILE_TEST_EXISTS))
	{
		GtkWidget *ImgButton_Img = gtk_image_new_from_file (pix2path);
#if GTK_VER_AT_LEAST(3,12)		
		gtk_button_set_always_show_image(GTK_BUTTON(CapImageButt), TRUE);
#endif
		gtk_button_set_image(GTK_BUTTON(CapImageButt), ImgButton_Img);
		gtk_button_set_image_position(GTK_BUTTON(CapImageButt), GTK_POS_TOP);
	}
	g_free(pix2path);
	
	gtk_box_pack_start(GTK_BOX(HButtonBox), CapImageButt, TRUE, TRUE, 2);
	gtk_widget_show (CapImageButt);

	g_signal_connect (GTK_BUTTON(CapImageButt), "clicked",
		G_CALLBACK (capture_image_clicked), NULL);

	/*video button*/
	CapVideoButt = gtk_toggle_button_new_with_mnemonic (_("Cap. Video (V)"));
	gui_set_video_capture_button_status_gtk3(get_encoder_status());

	char *pix3path = g_strconcat (PACKAGE_DATA_DIR, "/pixmaps/guvcview/movie.png",NULL);
	if (g_file_test(pix3path, G_FILE_TEST_EXISTS))
	{
		GtkWidget *VideoButton_Img = gtk_image_new_from_file (pix3path);
#if GTK_VER_AT_LEAST(3,12)
		gtk_button_set_always_show_image(GTK_BUTTON(CapVideoButt), TRUE);
#endif
		gtk_button_set_image(GTK_BUTTON(CapVideoButt), VideoButton_Img);
		gtk_button_set_image_position(GTK_BUTTON(CapVideoButt), GTK_POS_TOP);
	}
	g_free(pix3path);

	gtk_box_pack_start(GTK_BOX(HButtonBox), CapVideoButt, TRUE, TRUE, 2);
	gtk_widget_show (CapVideoButt);

	g_signal_connect (GTK_BUTTON(CapVideoButt), "clicked",
		G_CALLBACK (capture_video_clicked), NULL);

	/*quit button*/
	//GtkWidget *quitButton = gtk_button_new_from_stock(GTK_STOCK_QUIT);
	GtkWidget *quitButton = gtk_button_new_with_mnemonic (_("Quit"));

	char* pix4path = g_strconcat (PACKAGE_DATA_DIR, "/pixmaps/guvcview/close.png", NULL);
	if (g_file_test(pix4path,G_FILE_TEST_EXISTS))
	{
		GtkWidget *QButton_Img = gtk_image_new_from_file (pix4path);
#if GTK_VER_AT_LEAST(3,12)		
		gtk_button_set_always_show_image(GTK_BUTTON(quitButton), TRUE);
#endif
		gtk_button_set_image(GTK_BUTTON(quitButton), QButton_Img);
		gtk_button_set_image_position(GTK_BUTTON(quitButton), GTK_POS_TOP);

	}
	/*must free path strings*/
	g_free(pix4path);
	gtk_box_pack_start(GTK_BOX(HButtonBox), quitButton, TRUE, TRUE, 2);
	gtk_widget_show_all (quitButton);

	g_signal_connect (GTK_BUTTON(quitButton), "clicked",
		G_CALLBACK (quit_button_clicked), NULL);

	gtk_box_pack_start(GTK_BOX(maintable), HButtonBox, FALSE, TRUE, 2);

	/*--------------------------- Tab container -------------------------------*/
	GtkWidget *tab_box = gtk_notebook_new();
	gtk_widget_show (tab_box);

	/*------------------------ Image controls Tab -----------------------------*/

	GtkWidget *scroll_1 = gtk_scrolled_window_new(NULL,NULL);
	gtk_scrolled_window_set_placement(GTK_SCROLLED_WINDOW(scroll_1), GTK_CORNER_TOP_LEFT);
	gtk_widget_show(scroll_1);

	/*
	 * viewport is only needed for gtk < 3.8
	 * for 3.8 and above controls tab can be directly added to scroll1
	 */
	GtkWidget* viewport = gtk_viewport_new(NULL,NULL);
	gtk_widget_show(viewport);

	gtk_container_add(GTK_CONTAINER(scroll_1), viewport);

	gui_attach_gtk3_v4l2ctrls(viewport);

	GtkWidget *tab_1 = gtk_grid_new();
	gtk_widget_show (tab_1);

    GtkWidget *tab_1_label = gtk_label_new(_("Image Controls"));
	gtk_widget_show (tab_1_label);
	/** check for files */
	gchar *tab_1_icon_path = g_strconcat (PACKAGE_DATA_DIR,"/pixmaps/guvcview/image_controls.png",NULL);
	/** don't test for file - use default empty image if load fails */
	/** get icon image*/
	GtkWidget *tab_1_icon = gtk_image_new_from_file(tab_1_icon_path);
	gtk_widget_show (tab_1_icon);

	g_free(tab_1_icon_path);
	gtk_grid_attach (GTK_GRID(tab_1), tab_1_icon, 0, 0, 1, 1);
	gtk_grid_attach (GTK_GRID(tab_1), tab_1_label, 1, 0, 1, 1);

	gtk_notebook_append_page(GTK_NOTEBOOK(tab_box), scroll_1, tab_1);

	/*----------------------------H264 Controls Tab --------------------------*/
	if(v4l2core_get_h264_unit_id(get_v4l2_device_handler()) > 0)
	{
		GtkWidget *scroll_2 = gtk_scrolled_window_new(NULL,NULL);
		gtk_scrolled_window_set_placement(GTK_SCROLLED_WINDOW(scroll_2), GTK_CORNER_TOP_LEFT);
		gtk_widget_show(scroll_2);

		/*
		 * viewport is only needed for gtk < 3.8
		 * for 3.8 and above controls tab can be directly added to scroll1
		 */
		GtkWidget* viewport2 = gtk_viewport_new(NULL,NULL);
		gtk_widget_show(viewport2);

		gtk_container_add(GTK_CONTAINER(scroll_2), viewport2);

		gui_attach_gtk3_h264ctrls(viewport2);

		GtkWidget *tab_2 = gtk_grid_new();
		gtk_widget_show (tab_2);

		GtkWidget *tab_2_label = gtk_label_new(_("H264 Controls"));
		gtk_widget_show (tab_2_label);
		/** check for files */
		gchar *tab_2_icon_path = g_strconcat (PACKAGE_DATA_DIR,"/pixmaps/guvcview/image_controls.png",NULL);
		/** don't test for file - use default empty image if load fails */
		/** get icon image*/
		GtkWidget *tab_2_icon = gtk_image_new_from_file(tab_2_icon_path);
		gtk_widget_show (tab_2_icon);

		g_free(tab_2_icon_path);
		gtk_grid_attach (GTK_GRID(tab_2), tab_2_icon, 0, 0, 1, 1);
		gtk_grid_attach (GTK_GRID(tab_2), tab_2_label, 1, 0, 1, 1);

		gtk_notebook_append_page(GTK_NOTEBOOK(tab_box), scroll_2, tab_2);
	}

	/*exclude video and audio tabs if we are in control panel mode*/
	if(!is_control_panel)
	{
		/*----------------------- Video controls Tab ------------------------------*/

		GtkWidget *scroll_3 = gtk_scrolled_window_new(NULL,NULL);
		gtk_scrolled_window_set_placement(GTK_SCROLLED_WINDOW(scroll_3), GTK_CORNER_TOP_LEFT);
		gtk_widget_show(scroll_3);

		/*
		 * viewport is only needed for gtk < 3.8
		 * for 3.8 and above controls tab can be directly added to scroll1
		 */
		GtkWidget* viewport3 = gtk_viewport_new(NULL,NULL);
		gtk_widget_show(viewport3);

		gtk_container_add(GTK_CONTAINER(scroll_3), viewport3);

		gui_attach_gtk3_videoctrls(viewport3);

		GtkWidget *tab_3 = gtk_grid_new();
		gtk_widget_show (tab_3);

		GtkWidget *tab_3_label = gtk_label_new(_("Video Controls"));
		gtk_widget_show (tab_3_label);
		/** check for files */
		gchar *tab_3_icon_path = g_strconcat (PACKAGE_DATA_DIR,"/pixmaps/guvcview/video_controls.png",NULL);
		/** don't test for file - use default empty image if load fails */
		/** get icon image*/
		GtkWidget *tab_3_icon = gtk_image_new_from_file(tab_3_icon_path);
		gtk_widget_show (tab_3_icon);

		g_free(tab_3_icon_path);
		gtk_grid_attach (GTK_GRID(tab_3), tab_3_icon, 0, 0, 1, 1);
		gtk_grid_attach (GTK_GRID(tab_3), tab_3_label, 1, 0, 1, 1);

		gtk_notebook_append_page(GTK_NOTEBOOK(tab_box), scroll_3, tab_3);

		/*----------------------- Audio controls Tab ------------------------------*/

		GtkWidget *scroll_4 = gtk_scrolled_window_new(NULL,NULL);
		gtk_scrolled_window_set_placement(GTK_SCROLLED_WINDOW(scroll_4), GTK_CORNER_TOP_LEFT);
		gtk_widget_show(scroll_4);

		/*
		 * viewport is only needed for gtk < 3.8
		 * for 3.8 and above controls tab can be directly added to scroll1
		 */
		GtkWidget* viewport4 = gtk_viewport_new(NULL,NULL);
		gtk_widget_show(viewport4);

		gtk_container_add(GTK_CONTAINER(scroll_4), viewport4);

		gui_attach_gtk3_audioctrls(viewport4);

		GtkWidget *tab_4 = gtk_grid_new();
		gtk_widget_show (tab_4);

		GtkWidget *tab_4_label = gtk_label_new(_("Audio Controls"));
		gtk_widget_show (tab_4_label);
		/** check for files */
		gchar *tab_4_icon_path = g_strconcat (PACKAGE_DATA_DIR,"/pixmaps/guvcview/audio_controls.png",NULL);
		/** don't test for file - use default empty image if load fails */
		/** get icon image*/
		GtkWidget *tab_4_icon = gtk_image_new_from_file(tab_4_icon_path);
		gtk_widget_show (tab_4_icon);

		g_free(tab_4_icon_path);
		gtk_grid_attach (GTK_GRID(tab_4), tab_4_icon, 0, 0, 1, 1);
		gtk_grid_attach (GTK_GRID(tab_4), tab_4_label, 1, 0, 1, 1);

		gtk_notebook_append_page(GTK_NOTEBOOK(tab_box), scroll_4, tab_4);
	}

	/* Attach the notebook (tabs) */
	gtk_box_pack_start(GTK_BOX(maintable), tab_box, TRUE, TRUE, 2);

	/*-------------------------- Status bar ------------------------------------*/
	status_bar = gtk_statusbar_new();
	status_warning_id = gtk_statusbar_get_context_id (GTK_STATUSBAR(status_bar), "warning");

    gtk_widget_show(status_bar);
	/* add the status bar*/
	gtk_box_pack_start(GTK_BOX(maintable), status_bar, FALSE, FALSE, 2);


	/* attach to main window container */
	gtk_container_add (GTK_CONTAINER (main_window), maintable);

	/* add key events*/
	gtk_widget_add_events (GTK_WIDGET (main_window), GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK);
	g_signal_connect (GTK_WINDOW(main_window), "key_press_event", G_CALLBACK(window_key_pressed), NULL);

	/* add update timers:
	 *  devices
	 */
	gtk_devices_timer_id = g_timeout_add( 1000, check_device_events, NULL);
	/*controls*/
	gtk_control_events_timer_id = g_timeout_add(1000, check_control_events, NULL);

	return 0;
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
int gui_run_gtk3()
{

	int ret = 0;

	gtk_main_called = 1;

	gtk_main();

	return ret;
}

/*
 * closes and cleans the GTK3 GUI
 * args:
 *   none
 *
 * asserts:
 *   none
 *
 * returns: none
 */
void gui_close_gtk3()
{
	if(gtk_main_called)
		gtk_main_quit();

	gui_clean_gtk3_control_widgets_list();

	gtk_main_called = 0;
}
