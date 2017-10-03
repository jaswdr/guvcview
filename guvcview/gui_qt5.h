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

#ifndef GUI_QT5_H
#define GUI_QT5_H

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
int gui_attach_qt5(int width, int height);

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
int gui_run_qt5();

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
void gui_close_qt5();

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
void gui_error_qt5(
	const char *title,
	const char *message,
	int fatal);

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
void gui_status_message_qt5(const char *message);

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
void set_webm_codecs_qt5();

/*
 * emit a click event for image capture button
 * args:
 *   none
 *
 * asserts:
 *   none
 *
 * returns: none
 */
void gui_click_image_capture_button_qt5();

/*
 * emit a click event for video capture button
 * args:
 *   none
 *
 * asserts:
 *   none
 *
 * returns: none
 */
void gui_click_video_capture_button_qt5();

#endif
