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

#include <iostream>
#include <string>

#include "gui_qt5.hpp"

extern "C" {
#include <errno.h>
#include <assert.h>
/* support for internationalization - i18n */
#include <locale.h>
#include <libintl.h>

#include "gui.h"
/*add this last to avoid redefining _() and N_()*/
#include "gview.h"
#include "gviewrender.h"
#include "gviewencoder.h"
#include "video_capture.h"
}


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
int MainWindow::gui_attach_qt5_menu(QWidget *parent)
{
	/*assertions*/
	assert(parent != NULL);
	
	menubar = menuBar();
	menubar->show();

	QAction *menu_action;
	
	QMenu *controls_menu = new QMenu(_("Settings"), menubar);
	menu_action = controls_menu->addAction(_("Load Profile"), this,  SLOT(load_save_profile_clicked()));
	menu_action->setProperty("profile_dialog", 0);
	menu_action = controls_menu->addAction(_("Save Profile"), this, SLOT(load_save_profile_clicked()));
	menu_action->setProperty("profile_dialog", 1);
	controls_menu->addAction(_("Hardware Defaults"), this, SLOT(control_defaults_clicked()));
	
	controls_menu->show();
	
	menubar->addMenu(controls_menu);

	
	QMenu *camera_button_menu = new QMenu(_("Camera Button"), controls_menu);
	controls_menu->addMenu(camera_button_menu);
	
	QActionGroup* camera_group = new QActionGroup(camera_button_menu);
	camera_group->setExclusive(true);
	
	menu_action = camera_button_menu->addAction(_("Capture Image"), this,  SLOT(menu_camera_button_clicked()));
	menu_action->setProperty("camera_button", DEF_ACTION_IMAGE);
	menu_action->setCheckable(true);
	if (get_default_camera_button_action() == DEF_ACTION_IMAGE)
		menu_action->setChecked(true);
	camera_group->addAction(menu_action);
	
	menu_action = camera_button_menu->addAction(_("Capture Video"), this, SLOT(menu_camera_button_clicked()));
	menu_action->setProperty("camera_button", DEF_ACTION_VIDEO);
	menu_action->setCheckable(true);
	if (get_default_camera_button_action() == DEF_ACTION_VIDEO)
		menu_action->setChecked(true);
	camera_group->addAction(menu_action);
	
	camera_button_menu->show();

	/*control panel mode exclusions */
	if(!is_control_panel)
	{
		/*photo menu*/
		QMenu *photo_menu = new QMenu(_("Photo"), menubar);
		
		menubar->addMenu(photo_menu);
		
		photo_menu->addAction(_("File"), this,  SLOT(photo_file_clicked()));
		menu_action = photo_menu->addAction(_("Increment Filename"), this,  SLOT(photo_sufix_clicked()));
		menu_action->setCheckable(true);
		menu_action->setChecked(get_photo_sufix_flag() > 0);

		/*video menu*/
		QMenu *video_menu = new QMenu(_("Video"), menubar);
		
		menubar->addMenu(video_menu);
		
		video_menu->addAction(_("File"), this,  SLOT(video_file_clicked()));
		menu_action = video_menu->addAction(_("Increment Filename"), this,  SLOT(video_sufix_clicked()));
		menu_action->setCheckable(true);
		menu_action->setChecked(get_video_sufix_flag() > 0);

		/*video codecs*/
		QMenu *video_codec_menu = new QMenu(_("Video Codec"), video_menu);
		
		QActionGroup* video_codec_group = new QActionGroup(video_codec_menu);
		video_codec_group->setExclusive(true);
		
		video_menu->addMenu(video_codec_menu);
		video_codec_menu->show();
		
		int num_vcodecs = encoder_get_valid_video_codecs();
		int vcodec_ind =0;
		for (vcodec_ind =0; vcodec_ind < num_vcodecs; vcodec_ind++)
		{
			menu_action = video_codec_menu->addAction(
				gettext(encoder_get_video_codec_description(vcodec_ind)),
				this,  SLOT(video_codec_clicked()));
			menu_action->setProperty("video_codec", vcodec_ind);
			menu_action->setCheckable(true);
			if (vcodec_ind == get_video_codec_ind())
				menu_action->setChecked(true);
			video_codec_group->addAction(menu_action);
			
			/*store webm video codec action*/
			if(encoder_get_webm_video_codec_index() == vcodec_ind)
				webm_vcodec_action = menu_action;
		}
		
		video_menu->addAction(_("Video Codec Properties"), this,  SLOT(video_codec_properties()));

		/*audio codec menu*/
		QMenu *audio_codec_menu = new QMenu(_("Audio Codec"), video_menu);
		video_menu->addMenu(audio_codec_menu);
		audio_codec_menu->show();
		
		QActionGroup* audio_codec_group = new QActionGroup(audio_codec_menu);
		audio_codec_group->setExclusive(true);
		
		int num_acodecs = encoder_get_valid_audio_codecs();
		int acodec_ind = 0;
		for (acodec_ind = 0; acodec_ind < num_acodecs; acodec_ind++)
		{
			menu_action = audio_codec_menu->addAction(
				gettext(encoder_get_audio_codec_description(acodec_ind)),
				this,  SLOT(audio_codec_clicked()));
			menu_action->setProperty("audio_codec", acodec_ind);
			menu_action->setCheckable(true);
			if (acodec_ind == get_audio_codec_ind())
				menu_action->setChecked(true);
			audio_codec_group->addAction(menu_action);
			
			/*store webm audio codec action*/
			if(encoder_get_webm_audio_codec_index() == acodec_ind)
				webm_acodec_action = menu_action;
		}
		
		video_menu->addAction(_("Audio Codec Properties"), this,  SLOT(audio_codec_properties()));
	}

	return 0;
}
