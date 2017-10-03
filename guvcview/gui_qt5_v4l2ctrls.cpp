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
#include <vector>
#include <string>

#include "gui_qt5.hpp"

extern "C" {
#include <errno.h>
#include <assert.h>
/* support for internationalization - i18n */
#include <locale.h>
#include <libintl.h>

#include "video_capture.h"
#include "gui.h"
/*add this last to avoid redefining _() and N_()*/
#include "gview.h"
}

extern int debug_level;
extern int is_control_panel;

/*
 * get Qt control widgets for control id
 * args:
 *   id - v4l2 control id
 *
 * asserts:
 *   none
 *
 * returns: pointer to ControlWidgets or null
 */
ControlWidgets *MainWindow::gui_qt5_get_widgets_by_id(int id)
{
	unsigned int i = 0;
	for(i = 0; i < control_widgets_list.size(); ++i )
    {
		if(control_widgets_list[i]->id == id)
			return control_widgets_list[i];
	}

	return NULL;
}

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
int MainWindow::gui_attach_qt5_v4l2ctrls(QWidget *parent)
{
	/*assertions*/
	assert(parent != NULL);

	if(debug_level > 1)
		std::cout << "GUVCVIEW (Qt5): attaching v4l2 controls" << std::endl;

	QGridLayout *grid_layout = new QGridLayout();

	img_controls_grid = new QWidget(parent);
	img_controls_grid->setLayout(grid_layout);


	int i = 0;
	int n = 0;
	v4l2_ctrl_t *current = v4l2core_get_control_list(get_v4l2_device_handler());

    for(; current != NULL; current = current->next, ++n)
    {
		if(current == NULL)
		{
			std::cerr << "GUVCVIEW: ERROR (attach qt5 controls) empty control in list" << std::endl;
			break;
		}

		ControlWidgets *thisone = new ControlWidgets();
		control_widgets_list.push_back(thisone);

		/* id */
		thisone->id = current->control.id;

		/* label */
		thisone->label = new QLabel(current->name);
		thisone->label->show();


		switch (current->control.type)
		{
			case V4L2_CTRL_TYPE_INTEGER:

				switch (current->control.id)
				{
					//special cases
					case V4L2_CID_PAN_RELATIVE:
					case V4L2_CID_TILT_RELATIVE:
					{
						thisone->widget = new QWidget(img_controls_grid);
						QHBoxLayout *hbox_layout = new QHBoxLayout();

						QPushButton *PanTilt1 = new QPushButton(thisone->widget);
						QPushButton *PanTilt2 = new QPushButton(thisone->widget);
						if(current->control.id == V4L2_CID_PAN_RELATIVE)
						{
							PanTilt1->setText( _("Left"));
							PanTilt2->setText(_("Right"));
						}
						else
						{
							PanTilt1->setText(_("Down"));
							PanTilt2->setText(_("Up"));
						}

						PanTilt1->show();
						PanTilt2->show();

						thisone->widget->setLayout(hbox_layout);
						hbox_layout->addWidget(PanTilt1);
						hbox_layout->addWidget(PanTilt2);
						thisone->widget->show();
						
						/*set properties*/
						PanTilt1->setProperty("control_info", current->control.id);
						PanTilt2->setProperty("control_info", current->control.id);
						
						/*connect signals*/
						connect(PanTilt1,SIGNAL(released()),this, SLOT(button_PanTilt1_clicked ()));
						connect(PanTilt2,SIGNAL(released()),this, SLOT(button_PanTilt2_clicked ()));

						QSpinBox * spinbox = new QSpinBox(img_controls_grid);
						thisone->widget2 = spinbox;
						
						spinbox->setRange(-256,256);
						spinbox->setSingleStep(64);

						if(current->control.id == V4L2_CID_PAN_RELATIVE)
							spinbox->setValue(v4l2core_get_pan_step(get_v4l2_device_handler()));
						else
							spinbox->setValue(v4l2core_get_tilt_step(get_v4l2_device_handler()));
						
						/*set properties*/
						spinbox->setProperty("control_info", current->control.id);
						
						/*connect signals*/
						connect(spinbox, SIGNAL(valueChanged(int)), this, SLOT(pan_tilt_step_changed (int)));

						thisone->widget2->show();

						break;
					}

					case V4L2_CID_PAN_RESET:
					case V4L2_CID_TILT_RESET:
					{
						QPushButton *pushbutton = new QPushButton(img_controls_grid); 
						thisone->widget = pushbutton;
						pushbutton->setText(" ");
						pushbutton->show();

						/*set properties*/
						pushbutton->setProperty("control_info", current->control.id);

						/*connect signal*/
						connect(pushbutton,SIGNAL(released()),this, SLOT(button_clicked()));
						break;
					};

					case V4L2_CID_LED1_MODE_LOGITECH:
					{
						std::string LEDMenu[4] = {_("Off"),_("On"),_("Blinking"),_("Auto")};
						/*turn it into a menu control*/
						if(!current->menu)
							current->menu = (v4l2_querymenu *) calloc(4+1, sizeof(struct v4l2_querymenu));
						else
                    		current->menu = (v4l2_querymenu *) realloc(current->menu,  (4+1) * sizeof(struct v4l2_querymenu));

						if(current->menu == NULL)
						{
							std::cerr << "GUVCVIEW: FATAL memory allocation failure (gui_attach_qt_v4l2ctrls):" << strerror(errno) << std::endl;
							exit(-1);
						}

						current->menu[0].id = current->control.id;
						current->menu[0].index = 0;
						current->menu[0].name[0] = 'N'; /*just set something here*/
						current->menu[1].id = current->control.id;
						current->menu[1].index = 1;
						current->menu[1].name[0] = 'O';
						current->menu[2].id = current->control.id;
						current->menu[2].index = 2;
						current->menu[2].name[0] = 'B';
						current->menu[3].id = current->control.id;
						current->menu[3].index = 3;
						current->menu[3].name[0] = 'A';
						current->menu[4].id = current->control.id;
						current->menu[4].index = current->control.maximum+1;
						current->menu[4].name[0] = '\0';

						int j = 0;
						int def = 0;

						QComboBox *combobox = new QComboBox(img_controls_grid); 
						thisone->widget = combobox;

						for (j = 0; (int) current->menu[j].index <= current->control.maximum; j++)
						{
							combobox->addItem(LEDMenu[j].c_str(), current->menu[j].index);

							if(current->value == (int) current->menu[j].index)
								def = j;
						}

						combobox->setCurrentIndex(def);
						thisone->widget->show();

						/*set properties*/
						combobox->setProperty("control_info", current->control.id);

						/*signals*/
						connect(combobox, SIGNAL(currentIndexChanged(int)), this, SLOT(combo_changed(int)));

						break;
					}

					case V4L2_CID_RAW_BITS_PER_PIXEL_LOGITECH:
					{
						/*turn it into a menu control*/
						char* BITSMenu[2] = {_("8 bit"),_("12 bit")};
						/*turn it into a menu control*/
						if(!current->menu)
							current->menu = (v4l2_querymenu *) calloc(2+1, sizeof(struct v4l2_querymenu));
						else
							current->menu = (v4l2_querymenu *) realloc(current->menu, (2+1) * sizeof(struct v4l2_querymenu));
						if(current->menu == NULL)
						{
							fprintf(stderr,"GUVCVIEW: FATAL memory allocation failure (gui_attach_gtk3_v4l2ctrls): %s\n", strerror(errno));
							exit(-1);
						}
						current->menu[0].id = current->control.id;
						current->menu[0].index = 0;
						current->menu[0].name[0] = 'o'; /*just set something here*/
						current->menu[1].id = current->control.id;
						current->menu[1].index = 1;
						current->menu[1].name[0] = 'd';
						current->menu[2].id = current->control.id;
						current->menu[2].index = 2;
						current->menu[2].name[0] = '\0';

						int j = 0;
						int def = 0;
						QComboBox * combobox = new QComboBox(img_controls_grid);
						thisone->widget = combobox;

						for (j = 0; (int) current->menu[j].index <= current->control.maximum; j++)
						{
							combobox->addItem((char *) BITSMenu[j], current->menu[j].index);
							if(current->value == (int) current->menu[j].index)
								def = j;
						}

						combobox->setCurrentIndex(def);
						thisone->widget->show();

						/*set properties*/
						combobox->setProperty("control_info", current->control.id);

						/*signals*/
						connect(combobox, SIGNAL(currentIndexChanged(int)), this, SLOT(combo_changed(int)));
						break;
					}

					case V4L2_CID_FOCUS_LOGITECH:
					case V4L2_CID_FOCUS_ABSOLUTE:

						if(!is_control_panel)
						{
							//add a virtual control for software autofocus
							ControlWidgets *thisto = new ControlWidgets();
							control_widgets_list.push_back(thisto);

							QCheckBox *checkbox = new QCheckBox(_("Auto Focus (continuous)"), img_controls_grid);
							thisto->widget = checkbox;
							checkbox->setChecked(false);
							thisto->widget->show();

							QPushButton *pushbutton = new QPushButton(img_controls_grid); 
							thisto->widget2 = pushbutton;
							pushbutton->setText(_("set Focus"));
							thisto->widget2->show();

							/*connect signals*/
							connect(checkbox, SIGNAL(stateChanged(int)), this, SLOT(autofocus_changed(int)));
							connect(pushbutton, SIGNAL(released()), this, SLOT(setfocus_clicked()));

							//QGridLayout *layout = (QGridLayout *) img_controls_grid->layout();	
							grid_layout->addWidget(thisto->widget, n, 1);
							grid_layout->addWidget(thisto->widget2, n, 2);

							++n;
						}

					default: /*standard case - slider + spin*/
					{
						/* check for valid range */
						if((current->control.maximum > current->control.minimum) && (current->control.step != 0))
						{	
							QSlider *slider = new QSlider(Qt::Horizontal, img_controls_grid);
							thisone->widget = slider;
							slider->setRange(current->control.minimum, current->control.maximum);
							slider->setSingleStep(current->control.step);
							slider->setPageStep(current->control.step*10);
							slider->setValue(current->value);
							thisone->widget->show();

							QSpinBox *spinbox = new QSpinBox(img_controls_grid);
							thisone->widget2 = spinbox;

							spinbox->setRange(current->control.minimum, current->control.maximum);
							spinbox->setSingleStep(current->control.step);
							spinbox->setValue(current->value);
							thisone->widget2->show();

							/*set properties*/
							slider->setProperty("control_info", current->control.id);
							spinbox->setProperty("control_info", current->control.id);

							/*connect slider and spinbox*/
							connect(slider,SIGNAL(valueChanged(int)),spinbox,SLOT(setValue(int)) );
							connect(spinbox,SIGNAL(valueChanged(int)),slider,SLOT(setValue(int)) );

							/*signals*/
							connect(slider,SIGNAL(valueChanged(int)),this, SLOT(slider_value_changed(int)));
							//connect(spinbox,SIGNAL(valueChanged(int)),this, SLOT(spin_value_changed(int)));


							//if(!is_control_panel &&
							//   (current->control.id == V4L2_CID_FOCUS_LOGITECH ||
							//    current->control.id == V4L2_CID_FOCUS_ABSOLUTE))
							//{
							//	g_object_set_data (G_OBJECT (control_widgets_list[n-1].widget), "control_entry",
							//		control_widgets_list[n].widget);
							//	g_object_set_data (G_OBJECT (control_widgets_list[n-1].widget), "control2_entry",
							//		control_widgets_list[n].widget2);
							//}

						}
						else
                          std::cerr << "GUVCVIEW (Qt5): (Invalid range) [MAX <= MIN] for control id:" 
							<< std::hex << current->control.id << std::dec << std::endl;

						break;
					}
					break;
				}
				break;

			case V4L2_CTRL_TYPE_INTEGER64:
			{
				QLineEdit *entry = new QLineEdit(img_controls_grid);
				thisone->widget = entry;
				entry->setMaxLength(19);

				QPushButton *pushbutton = new QPushButton(img_controls_grid); 
				thisone->widget2 = pushbutton;
				pushbutton->setText(_("Apply"));
				thisone->widget2->show();

				/*set properties*/
				pushbutton->setProperty("control_info", current->control.id);
				pushbutton->setProperty("control_entry", QVariant::fromValue(entry));

				/*connect signal*/
				connect(pushbutton, SIGNAL(released()), this, SLOT(int64_button_clicked()));
			}	
			break;

			case V4L2_CTRL_TYPE_STRING:
			{
				QLineEdit *entry = new QLineEdit(img_controls_grid);
				thisone->widget = entry;
				entry->setMaxLength(current->control.maximum);

				QPushButton *pushbutton = new QPushButton(img_controls_grid); 
				thisone->widget2 = pushbutton;
				pushbutton->setText(_("Apply"));
				thisone->widget2->show();

				/*set properties*/
				pushbutton->setProperty("control_info", current->control.id);
				pushbutton->setProperty("control_entry",  QVariant::fromValue(entry));

				/*connect signal*/
				connect(pushbutton, SIGNAL(released()), this, SLOT(string_button_clicked()));
			}
			break;

			case V4L2_CTRL_TYPE_BITMASK:
			{
				QLineEdit *entry = new QLineEdit(img_controls_grid);
				thisone->widget = entry;
				entry->setMaxLength(current->control.maximum);

				QPushButton *pushbutton = new QPushButton(img_controls_grid); 
				thisone->widget2 = pushbutton;
				pushbutton->setText(_("Apply"));
				thisone->widget2->show();

				/*set properties*/
				pushbutton->setProperty("control_info", current->control.id);
				pushbutton->setProperty("control_entry",  QVariant::fromValue(entry));

				/*connect signal*/
				connect(pushbutton, SIGNAL(released()), this, SLOT(bitmask_button_clicked()));
			}
			break;

			case V4L2_CTRL_TYPE_INTEGER_MENU:
            case V4L2_CTRL_TYPE_MENU:

				if(current->menu)
				{
					int j = 0;
					int def = 0;

					QComboBox *combobox = new QComboBox(img_controls_grid); 
					thisone->widget = combobox;

					for (j = 0; (int) current->menu[j].index <= current->control.maximum; j++)
					{
						if(current->control.type == V4L2_CTRL_TYPE_MENU)
						{
							combobox->addItem((char *) current->menu_entry[j], current->menu[j].index);
						}
						else
						{
							QString text_input = QString("%1").arg(current->menu[j].value, 4, 10, QChar('0'));
							combobox->addItem(text_input, current->menu[j].value);
						}
						if(current->value == (int) current->menu[j].index)
							def = j;
					}

					combobox->setCurrentIndex(def);
					thisone->widget->show();
					
					/*set properties*/
					combobox->setProperty("control_info", current->control.id);
						
					/*signals*/
					connect(combobox, SIGNAL(currentIndexChanged(int)), this, SLOT(combo_changed(int)));
				}
                break;

			case V4L2_CTRL_TYPE_BUTTON:
				{
					QPushButton *pushbutton = new QPushButton(img_controls_grid); 
					thisone->widget = pushbutton;
					pushbutton->setText(" ");
					thisone->widget->show();

					/*set properties*/
					pushbutton->setProperty("control_info", current->control.id);
						
					/*signals*/
					connect(pushbutton, SIGNAL(released()), this, SLOT(button_clicked()));
				}
				break;
					
            case V4L2_CTRL_TYPE_BOOLEAN:
				{
					if(current->control.id == V4L2_CID_DISABLE_PROCESSING_LOGITECH)
					{
						QComboBox *combobox = new QComboBox(img_controls_grid); 
						thisone->widget2 = combobox;
					
						combobox->addItem("GBGB... | RGRG...", 0);
						combobox->addItem("GRGR... | BGBG...", 1);
						combobox->addItem("BGBG... | GRGR...", 2);
						combobox->addItem("RGRG... | GBGB...", 3);

						v4l2core_set_bayer_pix_order(get_v4l2_device_handler(), 0);
					
						combobox->setCurrentIndex(v4l2core_get_bayer_pix_order(get_v4l2_device_handler()));
						thisone->widget2->show();

						/*connect signal*/
						connect(combobox, SIGNAL(currentIndexChanged(int)), this, SLOT(bayer_pix_ord_changed(int)));

						uint8_t isbayer = (current->value ? true : false);
						v4l2core_set_isbayer(get_v4l2_device_handler(), isbayer);
					}

					QCheckBox *checkbox = new QCheckBox(img_controls_grid);
					thisone->widget = checkbox;
					checkbox->setChecked(current->value ? true : false);
					thisone->widget->show();

					/*set properties*/
					checkbox->setProperty("control_info", current->control.id);
					
					/*connect signal*/
					connect(checkbox, SIGNAL(stateChanged(int)), this, SLOT(check_changed(int)));
				}
                break;

			default:
				printf("control[%d]:(unknown - 0x%x) 0x%x '%s'\n",i ,current->control.type,
					current->control.id, current->control.name);
				break;
		}
		
		grid_layout->addWidget(thisone->label, n, 0, Qt::AlignRight);	
		if(thisone->widget)
			grid_layout->addWidget(thisone->widget, n, 1);
		if(thisone->widget2)
			grid_layout->addWidget(thisone->widget2, n, 2);
	}
	
	QSpacerItem *spacer = new QSpacerItem(40, 20, QSizePolicy::Minimum, QSizePolicy::Expanding);
	grid_layout->addItem(spacer, n+1, 0);
	
	gui_qt5_update_controls_state();
	
	return 0;
}

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
void MainWindow::gui_qt5_update_controls_state()
{
	v4l2_ctrl_t *current = v4l2core_get_control_list(get_v4l2_device_handler());

    for(; current != NULL; current = current->next)
    {
		if(current == NULL)
		{
			fprintf(stderr, "GUVCVIEW: ERROR (update controls state) empty control in list\n");
			break;
		}

		ControlWidgets *thisone =  gui_qt5_get_widgets_by_id(current->control.id);

		if(!thisone)
		{
			std::cerr << "GUVCVIEW (Qt5): (update widget state): control " 
				<< std::hex << current->control.id << std::dec
				<< "doesn't have a widget set" << std::endl;
			continue;
		}

		/*update controls values*/
		switch(current->control.type)
		{
			case V4L2_CTRL_TYPE_STRING:
			{
				if(current->string != NULL)
				{
					QString text_input(current->string);
					QLineEdit *entry = (QLineEdit *) thisone->widget;
					entry->setText(text_input);
				}
				else
					std::cerr << "GUVCVIEW (Qt5): control "
						<< std::hex << current->control.id << std::dec
						<<  "of type string has null value" << std::endl;
			}
			break;

			case V4L2_CTRL_TYPE_BOOLEAN:
			{
				/*disable widget signals*/
				thisone->widget->blockSignals(true);
				QCheckBox *checkbox = (QCheckBox *) thisone->widget;
				checkbox->setChecked(current->value ? true : false);
				/*enable widget signals*/
				thisone->widget->blockSignals(false);
			}
			break;

			case V4L2_CTRL_TYPE_BITMASK:
			{
				QString text_input("0x");
				text_input.append(QString::number((uint32_t)current->value, 16).toUpper());
				QLineEdit *entry = (QLineEdit *) thisone->widget;
				entry->setText(text_input);
			}
			break;

			case V4L2_CTRL_TYPE_INTEGER64:
			{
				QString text_input("0x");
				text_input.append(QString::number(current->value64, 16).toUpper());
				QLineEdit *entry = (QLineEdit *) thisone->widget;
				entry->setText(text_input);
			}
			break;

			case V4L2_CTRL_TYPE_INTEGER:
				if( current->control.id != V4L2_CID_PAN_RELATIVE &&
				    current->control.id != V4L2_CID_TILT_RELATIVE &&
				    current->control.id != V4L2_CID_PAN_RESET &&
				    current->control.id != V4L2_CID_TILT_RESET &&
				    current->control.id != V4L2_CID_LED1_MODE_LOGITECH &&
				    current->control.id != V4L2_CID_RAW_BITS_PER_PIXEL_LOGITECH )
				{
					/*disable spinbox signals*/
					thisone->widget2->blockSignals(true);
					/*updating slider will also update spinbox*/
					QSlider *slider = (QSlider *) thisone->widget;
					slider->setValue(current->value);
					/*enable spinbox signals*/
					thisone->widget2->blockSignals(false);
				}
				break;

			case V4L2_CTRL_TYPE_INTEGER_MENU:
			case V4L2_CTRL_TYPE_MENU:
			{
				/*disable widget signals*/
				thisone->widget->blockSignals(true);
				/*get new index*/
				int j = 0;
				int def = 0;
				for (j = 0; (int) current->menu[j].index <= current->control.maximum; j++)
				{
					if(current->value == (int) current->menu[j].index)
						def = j;
				}
				QComboBox *combobox = (QComboBox *) thisone->widget;
				combobox->setCurrentIndex(def);
				/*enable widget signals*/
				thisone->widget->blockSignals(false);
			}
				break;

			default:
				break;

		}

		/*update flags (enable disable)*/
		if((current->control.flags & V4L2_CTRL_FLAG_GRABBED) ||
            (current->control.flags & V4L2_CTRL_FLAG_DISABLED))
        {
			if(thisone->label)
                thisone->label->setDisabled(true);
            if(thisone->widget)
                thisone->widget->setDisabled(true);
            if(thisone->widget2)
                thisone->widget2->setDisabled(true);
        }
        else
        {
			if(thisone->label)
                thisone->label->setDisabled(false);
            if(thisone->widget)
                thisone->widget->setDisabled(false);
            if(thisone->widget2)
                thisone->widget2->setDisabled(false);
        }
	}
}
