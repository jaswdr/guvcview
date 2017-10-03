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

#ifndef GUI_GTK3_CALLBACKS_H
#define GUI_GTK3_CALLBACKS_H

#include <gtk/gtk.h>
#include <glib.h>
/* support for internationalization - i18n */
#include <glib/gi18n.h>

#include "gviewv4l2core.h"
#include "gviewencoder.h"

/*
 * delete event (close window)
 * args:
 *   widget - pointer to event widget
 *   event - pointe to event data
 *   data - pointer to user data
 *
 * asserts:
 *   none
 *
 * returns: error code
 */
int delete_event (GtkWidget *widget, GdkEventConfigure *event, void *data);

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
void quit_button_clicked(GtkWidget *widget, void *data);

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
void camera_button_menu_changed (GtkWidget *item, void *data);

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
void control_defaults_clicked (GtkWidget *item, void *data);

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
void controls_profile_clicked (GtkWidget *item, void *data);

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
void photo_sufix_toggled (GtkWidget *item, void *data);

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
void video_sufix_toggled (GtkWidget *item, void *data);

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
void video_codec_changed (GtkRadioMenuItem *item, void *data);

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
void audio_codec_changed (GtkRadioMenuItem *item, void *data);
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
void photo_file_clicked (GtkWidget *item, void *data);

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
void video_file_clicked (GtkWidget *item, void *data);

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
void capture_image_clicked (GtkButton *button, void *data);

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
void capture_video_clicked(GtkToggleButton *button, void *data);

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
void pan_tilt_step_changed (GtkSpinButton *spin, void *data);

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
void button_PanTilt1_clicked (GtkButton * Button, void *data);

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
void button_PanTilt2_clicked (GtkButton * Button, void *data);

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
void button_clicked (GtkButton * Button, void *data);

/*
 * a string control apply button clicked
 * args:
 *    button - button that generated the event
 *    data - pointer to user data
 *
 * asserts:
 *    control->string is not null
 *
 * returns: none
 */
void string_button_clicked(GtkButton * Button, void *data);

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
void int64_button_clicked(GtkButton * Button, void *data);

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
void bitmask_button_clicked(GtkButton * Button, void *data);

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
void slider_changed (GtkRange * range, void *data);

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
void spin_changed (GtkSpinButton * spin, void *data);

/*
 * combo box chaged event
 * args:
 *    combo - widget that generated the event
 *    data - pointer to user data
 *
 * asserts:
 *    none
 *
 * returns: none
 */
void combo_changed (GtkComboBox * combo, void *data);

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
void bayer_pix_ord_changed (GtkComboBox * combo, void *data);

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
void check_changed (GtkToggleButton *toggle, void *data);

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
void devices_changed (GtkComboBox *wgtDevices, void *data);

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
void frame_rate_changed (GtkComboBox *wgtFrameRate, void *data);

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
void resolution_changed (GtkComboBox *wgtResolution, void *data);

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
void format_changed(GtkComboBox *wgtInpType, void *data);

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
void render_fx_filter_changed(GtkToggleButton *toggle, void *data);

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
void audio_fx_filter_changed(GtkToggleButton *toggle, void *data);

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
void render_osd_changed(GtkToggleButton *toggle, void *data);

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
void autofocus_changed (GtkToggleButton * toggle, void *data);

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
void setfocus_clicked (GtkButton * button, void *data);

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
void audio_api_changed(GtkComboBox *combo, void *data);

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
void audio_device_changed(GtkComboBox *combo, void *data);

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
void audio_samplerate_changed(GtkComboBox *combo, void *data);

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
void audio_channels_changed(GtkComboBox *combo, void *data);

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
void audio_latency_changed(GtkRange *range, void *data);

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
void encoder_video_properties(GtkMenuItem *item, void *data);

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
void encoder_audio_properties(GtkMenuItem *item, void *data);

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
gboolean entry_focus (GtkWidget *entry, GdkEventKey *event, void *data);

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
gboolean window_key_pressed (GtkWidget *win, GdkEventKey *event, void *data);


/***** TIMERS *******/
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
gboolean check_device_events(gpointer data);

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
gboolean check_control_events(gpointer data);

#endif
