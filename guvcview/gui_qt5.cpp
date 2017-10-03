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

#include <QApplication>
#include <iostream>
#include <string>

#include "gui_qt5.hpp"

extern "C"{
#include <errno.h>
#include <assert.h>
/* support for internationalization - i18n */
#include <locale.h>
#include <libintl.h>

#include "video_capture.h"
#include "gui_qt5.h"
#include "gui.h"
/*add this last to avoid redefining _() and N_()*/
#include "gview.h"
#include "gviewencoder.h"
#include "gviewv4l2core.h"
}

extern int debug_level;
extern int is_control_panel;

ControlWidgets::ControlWidgets()
{
	id = -1;
	label = NULL;
	widget = NULL;
	widget2 = NULL;
}

MainWindow::MainWindow()
{
	webm_vcodec_action = NULL;
	webm_acodec_action = NULL;

	QWidget *widget = new QWidget;
	widget->show();
    setCentralWidget(widget);

    QVBoxLayout *layout = new QVBoxLayout;
    widget->setLayout(layout);

    /*-------------------------------menu-------------------------------------*/
    gui_attach_qt5_menu(this);
    setMenuBar(menubar);

    /*-----------------------------buttons------------------------------------*/
    QHBoxLayout *button_box_layout = new QHBoxLayout;
    QWidget *button_box = new QWidget;
    button_box->setLayout(button_box_layout);
    button_box->show();

    layout->addWidget(button_box);

    /*control panel mode exclusions */
	if(!is_control_panel)
	{
		/*Photo capture*/
		cap_img_button = new QToolButton;
		QIcon cap_img_icon(QString(PACKAGE_DATA_DIR).append("/pixmaps/guvcview/camera.png"));
		cap_img_button->setIcon(cap_img_icon);
		cap_img_button->setIconSize(QSize(64,64));
		cap_img_button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
		cap_img_button->setFixedSize(128,80);

		if(check_photo_timer())
		{
			cap_img_button->setText(_("Stop Cap. (I)"));
			cap_img_button->setProperty("control_info", 1);
		}
		else
		{
			cap_img_button->setText(_("Cap. Image (I)"));
			cap_img_button->setProperty("control_info", 0);
		}
		cap_img_button->show();

		/*signals*/
		connect(cap_img_button, SIGNAL(clicked()), this, SLOT(capture_image_clicked()));	

		button_box_layout->addWidget(cap_img_button);

		/*video capture*/
		cap_video_button = new QToolButton;
		QIcon cap_video_icon(QString(PACKAGE_DATA_DIR).append("/pixmaps/guvcview/movie.png"));
		cap_video_button->setIcon(cap_video_icon);
		cap_video_button->setIconSize(QSize(64,64));
		cap_video_button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
		cap_video_button->setFixedSize(128,80);

		if(check_video_timer())
		{
			cap_video_button->setText(_("Stop Video (V)"));
			cap_video_button->setProperty("control_info", 1);
		}
		else
		{
			cap_video_button->setText(_("Cap. Video (V)"));
			cap_video_button->setProperty("control_info", 0);
		}

		cap_video_button->show();

		/*signals*/
		connect(cap_video_button, SIGNAL(clicked()), this, SLOT(capture_video_clicked()));	

		button_box_layout->addWidget(cap_video_button);
	}
	/*quit*/
	QToolButton *quit_button = new QToolButton;
	QIcon quit_icon(QString(PACKAGE_DATA_DIR).append("/pixmaps/guvcview/close.png"));
	quit_button->setIcon(quit_icon);
	quit_button->setIconSize(QSize(64,64));
	quit_button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
	quit_button->setFixedSize(128,80);

	quit_button->setText(_("Quit"));

	quit_button->show();

	/*signals*/
	connect(quit_button, SIGNAL(clicked()), this, SLOT(quit_button_clicked()));	

	button_box_layout->addWidget(quit_button);

    /*------------------------------------Tabs--------------------------------*/
	QTabWidget *control_tab = new QTabWidget;
	layout->addWidget(control_tab);
	control_tab->setIconSize(QSize(64,64));

	/*----------------------------V4l2 Controls Tab --------------------------*/
	QScrollArea *scroll_ctrls = new QScrollArea(control_tab);

	gui_attach_qt5_v4l2ctrls(scroll_ctrls);
	scroll_ctrls->setWidget(img_controls_grid);
	scroll_ctrls->setWidgetResizable(true);

	int tab_ind = control_tab->addTab(scroll_ctrls, _("Image Controls"));
	QIcon image_tab_icon(QString(PACKAGE_DATA_DIR).append("/pixmaps/guvcview/image_controls.png"));
	control_tab->setTabIcon(tab_ind, image_tab_icon);

	/*----------------------------H264 Controls Tab --------------------------*/
	if(v4l2core_get_h264_unit_id(get_v4l2_device_handler()) > 0)
	{
		QScrollArea *scroll_h264ctrls = new QScrollArea(control_tab);

		gui_attach_qt5_h264ctrls(scroll_h264ctrls);
		scroll_h264ctrls->setWidget(h264_controls_grid);
		scroll_h264ctrls->setWidgetResizable(true);

		tab_ind = control_tab->addTab(scroll_h264ctrls, _("H264 Controls"));
		QIcon h264_tab_icon(QString(PACKAGE_DATA_DIR).append("/pixmaps/guvcview/image_controls.png"));
		control_tab->setTabIcon(tab_ind, h264_tab_icon);

	}
	/*control panel mode exclusions */
	if(!is_control_panel)
	{
		/*------------------------Video Controls Tab -------------------------*/
		QScrollArea *scroll_video = new QScrollArea(control_tab);

		gui_attach_qt5_videoctrls(scroll_video);
		scroll_video->setWidget(video_controls_grid);
		scroll_video->setWidgetResizable(true);

		tab_ind = control_tab->addTab(scroll_video, _("Video Controls"));
		QIcon video_tab_icon(QString(PACKAGE_DATA_DIR).append("/pixmaps/guvcview/video_controls.png"));
		control_tab->setTabIcon(tab_ind, video_tab_icon);

		/*------------------------Audio Controls Tab -------------------------*/
		QScrollArea *scroll_audio = new QScrollArea(control_tab);

		gui_attach_qt5_audioctrls(scroll_audio);
		scroll_audio->setWidget(audio_controls_grid);
		scroll_audio->setWidgetResizable(true);

		tab_ind = control_tab->addTab(scroll_audio, _("Audio Controls"));
		QIcon audio_tab_icon(QString(PACKAGE_DATA_DIR).append("/pixmaps/guvcview/audio_controls.png"));
		control_tab->setTabIcon(tab_ind, audio_tab_icon);
	}

	/*-------------------------- Status Bar ----------------------------------*/
	statusbar = statusBar();
	statusbar->show();

	timer_check_device = new QTimer(this);
    connect(timer_check_device, SIGNAL(timeout()), 
		this, SLOT(check_device_events()));
    timer_check_device->start(1000);
	
	timer_check_control_events = new QTimer(this);
	connect(timer_check_control_events, SIGNAL(timeout()), 
		this, SLOT(check_control_events()));
	timer_check_control_events->start(1000);
}

MainWindow::~MainWindow()
{
	if(debug_level > 1)
		std::cout <<"GUVCVIEW (Qt5): cleaning MainWindow" << std::endl;
	for (std::vector<ControlWidgets *>::iterator it = control_widgets_list.begin() ; it != control_widgets_list.end(); ++it)
		delete(*it);
	control_widgets_list.clear();
}

void MainWindow::set_statusbar_message(QString message)
{
	//displays the message for 5 seconds
	statusbar->showMessage(message, 5000);
}

/******************************* C wrapper functions ********************************/

QApplication *my_app = NULL;
MainWindow *mainWin = NULL;

/*we need global argc and argv for QApplication*/
int argc = 1;
QString argv1 = "guvcview";
char *argv = (char *) argv1.toStdString().c_str();

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
	int fatal)
{
	if(!QCoreApplication::instance())
	{
		my_app = new QApplication(argc, &argv);
		if(!my_app)
		{
			std::cerr <<"GUVCVIEW (Qt5): app creation failed" << std::endl;
			return;
		}
		else if(debug_level > 1)
			std::cout <<"GUVCVIEW (Qt5): app created" << std::endl;
	
		my_app->setOrganizationName("Guvcview Video Capture");
		my_app->setApplicationName("Guvcview");
	}
	
	/*simple warning message - not fatal and no device selection*/
	if(!fatal)
	{
		QMessageBox::warning(mainWin, gettext(title),
                             gettext(message),
                             QMessageBox::Ok,
                             QMessageBox::Ok);
		return;
	}

	/*fatal error message*/

	/*add device list (more than one device)*/
	int show_dev_list = (v4l2core_get_num_devices() >= 1) ? 1: 0;

	std::cerr << "GUVCVIEW (Qt5): fatal error (" << v4l2core_get_num_devices() << " devices detected)" << std::endl;
	
	if(show_dev_list)
	{
		std::cerr << "GUVCVIEW (Qt5): creating input dialog" << std::endl;
		
		QStringList items;
		int i = 0;
		for(i = 0; i < v4l2core_get_num_devices(); i++)
		{
			items << v4l2core_get_device_sys_data(i)->name;
		}
		bool ok;
		
		QString dialog_text = gettext(message);
		dialog_text.append(_("\nYou seem to have video devices installed.\n"
							 "Do you want to try one ?\n"));
		QString item = QInputDialog::getItem(mainWin, gettext(title), dialog_text, 
			items, v4l2core_get_num_devices() - 1, 
			false, &ok);
			
		if (ok && !item.isEmpty())
		{
			QStringList args;
			QString dev_arg = "--device=";
			int i = 0;
			foreach (const QString &str, items) 
			{
				if(str == item)
					break;
				i++;
			}

			dev_arg.append(v4l2core_get_device_sys_data(i)->device);
			args << dev_arg;
			QProcess process;
			process.startDetached("guvcview", args);
		}
		
	}	
	else
	{
		std::cerr << "GUVCVIEW (Qt5): creating error dialog" << std::endl;
		QMessageBox::critical(mainWin, gettext(title),
                               gettext(message),
                               QMessageBox::Ok,
                               QMessageBox::Ok);
	}

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
int gui_attach_qt5(int width, int height)
{
	if(debug_level > 1)
		std::cout << "GUVCVIEW (Qt5): attaching GUI\n" << std::endl;
	
	//Q_INIT_RESOURCE(application);
	if(!QCoreApplication::instance())
	{
		my_app = new QApplication(argc, &argv);
		if(!my_app)
		{
			std::cerr <<"GUVCVIEW (Qt5): app creation failed" << std::endl;
			return -1;
		}
		else if(debug_level > 1)
			std::cout <<"GUVCVIEW (Qt5): app created" << std::endl;
	
		my_app->setOrganizationName("Guvcview Video Capture");
		my_app->setApplicationName("Guvcview");
	}

	if(debug_level > 2)
		std::cout <<"GUVCVIEW (Qt5): creating window" << std::endl;
	mainWin = new MainWindow();
	if(width <=0 || height <= 0)
	{
		width = 800;
		height = 600;
	}
	mainWin->resize(width, height);
    mainWin->show();

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
int gui_run_qt5()
{
	if(debug_level > 2)
		std::cout <<"GUVCVIEW (Qt5): executing GUI app" << std::endl;
	if(my_app)
		return my_app->exec();
	else
	{
		std::cout <<"GUVCVIEW (Qt5): application not created" << std::endl;
		return -1;
	}
}

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
void gui_close_qt5()
{
	//my_app->closeAllWindows();
	if(debug_level > 1)
		std::cerr << "GUVCVIEW (Qt5): closing Gui" << std::endl;
	
	if(my_app)
		my_app->quit();
	
	delete(mainWin);

	if(debug_level > 2)
		std::cerr << "GUVCVIEW (Qt5): all done" << std::endl;
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
void gui_status_message_qt5(const char *message)
{
	if(mainWin)
	{
		QCoreApplication::postEvent(mainWin, new QEvent(QEvent::UpdateRequest),
                            Qt::LowEventPriority);
		QMetaObject::invokeMethod(mainWin, "set_statusbar_message", Q_ARG(QString, QString(message)));

		//mainWin->set_statusbar_message(message);
	}
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
void set_webm_codecs_qt5()
{
	/*force webm codecs*/
	int video_codec_ind = encoder_get_webm_video_codec_index();
	set_video_codec_ind(video_codec_ind);
	int audio_codec_ind = encoder_get_webm_audio_codec_index();
	set_audio_codec_ind(audio_codec_ind);
}

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
void gui_click_image_capture_button_qt5()
{
	if(mainWin)
		mainWin->capture_image_clicked();
}

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
void gui_click_video_capture_button_qt5()
{
	if(mainWin)
		mainWin->capture_video_clicked();
}
