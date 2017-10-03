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

#ifndef GUI_GTK3_H
#define GUI_GTK3_H

#include <gtk/gtk.h>
#include <glib.h>
/* support for internationalization - i18n */
//#include <glib/gi18n.h>

#define GTK_VER_AT_LEAST(major,minor)  ( GTK_MAJOR_VERSION > major || \
                                        (GTK_MAJOR_VERSION == major && \
                                         GTK_MINOR_VERSION >= minor))

#include "gviewv4l2core.h"

typedef struct _control_widgets_t
{
	int id;                         /*control id*/
	GtkWidget *label;               /*control label widget*/
	GtkWidget *widget;              /*control widget 1*/
	GtkWidget *widget2;             /*control widget 2*/
} control_widgets_t;

typedef struct _audio_widgets_t
{
	GtkWidget *api;          /* api control      */
	GtkWidget *device;       /* device control   */
	GtkWidget *channels;     /* channels control */
	GtkWidget *samprate;     /* samprate control */
	GtkWidget *latency;      /* latency control  */
} audio_widgets_t;

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
GSList *get_video_codec_group_list_gtk3();

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
void set_video_codec_group_list_gtk3(GSList *list);

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
GSList *get_audio_codec_group_list_gtk3();

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
void set_audio_codec_group_list_gtk3(GSList *list);

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
void set_webm_codecs_gtk3();

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
	int fatal);

/*
 * GUI initialization
 * args:
 *   width - window width
 *   height - window height
 *
 * asserts:
 *   none
 *
 * returns: error code
 */
int gui_attach_gtk3(int width, int height);

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
int gui_run_gtk3();

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
void gui_close_gtk3();

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
int gui_attach_gtk3_menu(GtkWidget *parent);

/*
 * attach v4l2 controls tab widget
 * args:
 *   parent - tab parent widget
 *
 * asserts:
 *   parent is not null
 *
 * returns: error code (0 -OK)
 */
int gui_attach_gtk3_v4l2ctrls(GtkWidget *parent);

/*
 * attach h264 controls tab widget
 * args:
 *   parent - tab parent widget
 *
 * asserts:
 *   parent is not null
 *
 * returns: error code (0 -OK)
 */
int gui_attach_gtk3_h264ctrls (GtkWidget *parent);

/*
 * attach v4l2 video controls tab widget
 * args:
 *   parent - tab parent widget
 *
 * asserts:
 *   parent is not null
 *
 * returns: error code (0 -OK)
 */
int gui_attach_gtk3_videoctrls(GtkWidget *parent);

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
int gui_attach_gtk3_audioctrls(GtkWidget *parent);

/*
 * get gtk control widgets for v4l2 control id
 * args:
 *   id - v4l2 control id
 *
 * asserts:
 *   none
 *
 * returns: pointer to control_widgets_t or null
 */
control_widgets_t *gui_gtk3_get_widgets_by_id(int id);

/*
 * update the controls widgets state
 * args:
 *   none
 *
 * asserts:
 *   none
 *
 * returns: none
 */
void gui_gtk3_update_controls_state();

/*
 * clean gtk3 control widgets list
 * args:
 *   none
 *
 * asserts:
 *   none
 *
 * returns: none
 */
void gui_clean_gtk3_control_widgets_list();

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
void gui_status_message_gtk3(const char *message);

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
void gui_click_image_capture_button_gtk3();

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
void gui_set_image_capture_button_label_gtk3(const char *label);

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
void gui_click_video_capture_button_gtk3();

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
void gui_set_video_capture_button_status_gtk3(int flag);

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
GtkWidget *get_main_window_gtk3();

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
GtkWidget *get_wgtDevices_gtk3();

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
void set_wgtDevices_gtk3(GtkWidget *widget);


#endif
