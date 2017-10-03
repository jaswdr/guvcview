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

#include "video_capture.h"
#include "gui_gtk3.h"
#include "gui_gtk3_callbacks.h"
#include "gui.h"
/*add this last to avoid redefining _() and N_()*/
#include "gview.h"


extern int debug_level;
extern int is_control_panel;

/*
 * NULL terminated list won't work here
 * since we use realloc memory location will change
 */
static control_widgets_t *control_widgets_list = NULL; /*control widgets list*/
static int widget_list_size = 0; /*list size*/

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
void gui_clean_gtk3_control_widgets_list()
{
	if(control_widgets_list)
		free(control_widgets_list);
}

/*
 * get gtk control widgets for control id
 * args:
 *   id - v4l2 control id
 *
 * asserts:
 *   none
 *
 * returns: pointer to control_widgets_t or null
 */
control_widgets_t *gui_gtk3_get_widgets_by_id(int id)
{
	int i = 0;
	for(i = 0; i < widget_list_size; ++i )
    {
		if(control_widgets_list[i].id == id)
			return &control_widgets_list[i];
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
int gui_attach_gtk3_v4l2ctrls(GtkWidget *parent)
{
	/*assertions*/
	assert(parent != NULL);

	if(debug_level > 1)
		printf("GUVCVIEW (Gtk3): attaching v4l2 controls\n");

	GtkWidget *img_controls_grid = gtk_grid_new();
	gtk_widget_show (img_controls_grid);

	gtk_grid_set_column_homogeneous (GTK_GRID(img_controls_grid), FALSE);
	gtk_widget_set_hexpand (img_controls_grid, TRUE);
	gtk_widget_set_halign (img_controls_grid, GTK_ALIGN_FILL);

	gtk_grid_set_row_spacing (GTK_GRID(img_controls_grid), 4);
	gtk_grid_set_column_spacing (GTK_GRID (img_controls_grid), 4);
	gtk_container_set_border_width (GTK_CONTAINER (img_controls_grid), 2);

	int i = 0;
	int n = 0;
	v4l2_ctrl_t *current = v4l2core_get_control_list(get_v4l2_device_handler());

    for(; current != NULL; current = current->next, ++n)
    {
		if(current == NULL)
		{
			fprintf(stderr, "GUVCVIEW: ERROR (attach gtk3 controls) empty control in list\n");
			break;
		}

		if(!is_control_panel &&
		   (current->control.id == V4L2_CID_FOCUS_LOGITECH ||
		    current->control.id == V4L2_CID_FOCUS_ABSOLUTE))
		{
			++n; /*add a virtual software autofocus control*/
		}

		widget_list_size = n + 1;

		control_widgets_list = realloc(control_widgets_list, sizeof(control_widgets_t) * widget_list_size);
		if(control_widgets_list == NULL)
		{
			fprintf(stderr,"GUVCVIEW: FATAL memory allocation failure (gui_attach_gtk3_v4l2ctrls): %s\n", strerror(errno));
			exit(-1);
		}
		/*label*/
		char *tmp;
        tmp = g_strdup_printf ("%s:", current->name);
        control_widgets_list[widget_list_size - 1].label = gtk_label_new (tmp);
        g_free(tmp);
        gtk_widget_show (control_widgets_list[widget_list_size - 1].label);
#if GTK_VER_AT_LEAST(3,15)
				gtk_label_set_xalign(GTK_LABEL(control_widgets_list[widget_list_size - 1].label), 1);
				gtk_label_set_yalign(GTK_LABEL(control_widgets_list[widget_list_size - 1].label), 0.5);
#else
				gtk_misc_set_alignment (GTK_MISC (control_widgets_list[widget_list_size - 1].label), 1, 0.5);
#endif

		control_widgets_list[widget_list_size - 1].id = current->control.id;
        control_widgets_list[widget_list_size - 1].widget = NULL;
        control_widgets_list[widget_list_size - 1].widget2 = NULL; /*usually a spin button*/

		switch (current->control.type)
		{
			case V4L2_CTRL_TYPE_INTEGER:

				switch (current->control.id)
				{
					//special cases
					case V4L2_CID_PAN_RELATIVE:
					case V4L2_CID_TILT_RELATIVE:
					{
						control_widgets_list[n].widget = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 1);

						GtkWidget *PanTilt1 = NULL;
						GtkWidget *PanTilt2 = NULL;
						if(current->control.id == V4L2_CID_PAN_RELATIVE)
						{
							PanTilt1 = gtk_button_new_with_label(_("Left"));
							PanTilt2 = gtk_button_new_with_label(_("Right"));
						}
						else
						{
							PanTilt1 = gtk_button_new_with_label(_("Down"));
							PanTilt2 = gtk_button_new_with_label(_("Up"));
						}

						gtk_widget_show (PanTilt1);
						gtk_widget_show (PanTilt2);
						gtk_box_pack_start(GTK_BOX(control_widgets_list[n].widget),PanTilt1,TRUE,TRUE,2);
						gtk_box_pack_start(GTK_BOX(control_widgets_list[n].widget),PanTilt2,TRUE,TRUE,2);

						g_object_set_data (G_OBJECT (PanTilt1), "control_info",
							GINT_TO_POINTER(current->control.id));
						g_object_set_data (G_OBJECT (PanTilt2), "control_info",
							GINT_TO_POINTER(current->control.id));

						/*connect signals*/
						g_signal_connect (GTK_BUTTON(PanTilt1), "clicked",
							G_CALLBACK (button_PanTilt1_clicked), NULL);
						g_signal_connect (GTK_BUTTON(PanTilt2), "clicked",
							G_CALLBACK (button_PanTilt2_clicked), NULL);

						gtk_widget_show (control_widgets_list[n].widget);

						control_widgets_list[n].widget2 = gtk_spin_button_new_with_range(-256, 256, 64);

						gtk_editable_set_editable(GTK_EDITABLE(control_widgets_list[n].widget2), TRUE);

						if(current->control.id == V4L2_CID_PAN_RELATIVE)
							gtk_spin_button_set_value (GTK_SPIN_BUTTON(control_widgets_list[n].widget2), v4l2core_get_pan_step(get_v4l2_device_handler()));
						else
							gtk_spin_button_set_value (GTK_SPIN_BUTTON(control_widgets_list[n].widget2), v4l2core_get_tilt_step(get_v4l2_device_handler()));

						/*connect signal*/
						g_object_set_data (G_OBJECT (control_widgets_list[n].widget2), "control_info",
							GINT_TO_POINTER(current->control.id));
						g_signal_connect(GTK_SPIN_BUTTON(control_widgets_list[n].widget2),"value-changed",
							G_CALLBACK (pan_tilt_step_changed), NULL);

						gtk_widget_show (control_widgets_list[n].widget2);

						break;
					}

					case V4L2_CID_PAN_RESET:
					case V4L2_CID_TILT_RESET:
					{
						control_widgets_list[n].widget = gtk_button_new_with_label(" ");
						gtk_widget_show (control_widgets_list[n].widget);

						g_object_set_data (G_OBJECT (control_widgets_list[n].widget), "control_info",
							GINT_TO_POINTER(current->control.id));

						/*connect signal*/
						g_signal_connect (GTK_BUTTON(control_widgets_list[n].widget), "clicked",
							G_CALLBACK (button_clicked), NULL);

						break;
					};

					case V4L2_CID_LED1_MODE_LOGITECH:
					{
						char* LEDMenu[4] = {_("Off"),_("On"),_("Blinking"),_("Auto")};
						/*turn it into a menu control*/
						if(!current->menu)
                    					current->menu = calloc(4+1, sizeof(struct v4l2_querymenu));
                    				else
                    					current->menu = realloc(current->menu,  (4+1) * sizeof(struct v4l2_querymenu));
						if(current->menu == NULL)
						{
							fprintf(stderr,"GUVCVIEW: FATAL memory allocation failure (gui_attach_gtk3_v4l2ctrls): %s\n", strerror(errno));
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

						control_widgets_list[n].widget = gtk_combo_box_text_new ();
						for (j = 0; current->menu[j].index <= current->control.maximum; j++)
						{
							gtk_combo_box_text_append_text (
								GTK_COMBO_BOX_TEXT (control_widgets_list[n].widget),
								(char *) LEDMenu[j]);
							if(current->value == current->menu[j].index)
								def = j;
						}

						gtk_combo_box_set_active (GTK_COMBO_BOX(control_widgets_list[n].widget), def);
						gtk_widget_show (control_widgets_list[n].widget);

						g_object_set_data (G_OBJECT (control_widgets_list[n].widget), "control_info",
                           	GINT_TO_POINTER(current->control.id));

						/*connect signal*/
						g_signal_connect (GTK_COMBO_BOX_TEXT(control_widgets_list[n].widget), "changed",
							G_CALLBACK (combo_changed), NULL);

						break;
					}

					case V4L2_CID_RAW_BITS_PER_PIXEL_LOGITECH:
					{
						/*turn it into a menu control*/
						char* BITSMenu[2] = {_("8 bit"),_("12 bit")};
						/*turn it into a menu control*/
						if(!current->menu)
							current->menu = calloc(2+1, sizeof(struct v4l2_querymenu));
						else
							current->menu = realloc(current->menu, (2+1) * sizeof(struct v4l2_querymenu));
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
						control_widgets_list[n].widget = gtk_combo_box_text_new ();
						for (j = 0; current->menu[j].index <= current->control.maximum; j++)
						{
							//if (debug_level > 0)
							//	printf("GUVCVIEW: adding menu entry %d: %d, %s\n",j, current->menu[j].index, current->menu[j].name);
							gtk_combo_box_text_append_text (
								GTK_COMBO_BOX_TEXT (control_widgets_list[n].widget),
								(char *) BITSMenu[j]);
							if(current->value == current->menu[j].index)
								def = j;
						}

						gtk_combo_box_set_active (GTK_COMBO_BOX(control_widgets_list[n].widget), def);
						gtk_widget_show (control_widgets_list[n].widget);

						g_object_set_data (G_OBJECT (control_widgets_list[n].widget), "control_info",
							GINT_TO_POINTER(current->control.id));
						/*connect signal*/
						g_signal_connect (GTK_COMBO_BOX_TEXT(control_widgets_list[n].widget), "changed",
							G_CALLBACK (combo_changed), NULL);
						break;
					}

					case V4L2_CID_FOCUS_LOGITECH:
					case V4L2_CID_FOCUS_ABSOLUTE:

						if(!is_control_panel)
						{
							/*add a virtual control for software autofocus*/
							control_widgets_list[n-1].widget = gtk_check_button_new_with_label (_("Auto Focus (continuous)"));
							control_widgets_list[n-1].widget2 = gtk_button_new_with_label (_("set Focus"));

							gtk_widget_show (control_widgets_list[n-1].widget);
							gtk_widget_show (control_widgets_list[n-1].widget2);


							gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (control_widgets_list[n-1].widget), FALSE);

							g_signal_connect (G_OBJECT (control_widgets_list[n-1].widget), "toggled",
								G_CALLBACK (autofocus_changed), NULL);
							g_signal_connect (G_OBJECT (control_widgets_list[n-1].widget2), "clicked",
								G_CALLBACK (setfocus_clicked), NULL);

							gtk_grid_attach(GTK_GRID(img_controls_grid), control_widgets_list[n-1].widget, 1, i, 1 , 1);
							gtk_widget_set_halign (control_widgets_list[n-1].widget, GTK_ALIGN_FILL);
							gtk_widget_set_hexpand (control_widgets_list[n-1].widget, TRUE);
							gtk_grid_attach(GTK_GRID(img_controls_grid), control_widgets_list[n-1].widget2, 2, i, 1 , 1);

							i++;
						}

					default: /*standard case - hscale + spin*/
					{
						/* check for valid range */
						if((current->control.maximum > current->control.minimum) && (current->control.step != 0))
						{
							GtkAdjustment *adjustment =  gtk_adjustment_new (
								current->value,
								current->control.minimum,
								current->control.maximum,
								current->control.step,
								current->control.step*10,
								0);

							control_widgets_list[n].widget = gtk_scale_new (GTK_ORIENTATION_HORIZONTAL, adjustment);

							gtk_scale_set_draw_value (GTK_SCALE (control_widgets_list[n].widget), FALSE);
							gtk_scale_set_digits(GTK_SCALE(control_widgets_list[n].widget), 0);

							gtk_widget_show (control_widgets_list[n].widget);

							control_widgets_list[n].widget2= gtk_spin_button_new(adjustment, current->control.step, 0);

							gtk_editable_set_editable(GTK_EDITABLE(control_widgets_list[n].widget2),TRUE);

							gtk_widget_show (control_widgets_list[n].widget2);

							g_object_set_data (G_OBJECT (control_widgets_list[n].widget), "control_info",
								GINT_TO_POINTER(current->control.id));
							g_object_set_data (G_OBJECT (control_widgets_list[n].widget2), "control_info",
								GINT_TO_POINTER(current->control.id));

							if(!is_control_panel &&
							   (current->control.id == V4L2_CID_FOCUS_LOGITECH ||
							    current->control.id == V4L2_CID_FOCUS_ABSOLUTE))
							{
								g_object_set_data (G_OBJECT (control_widgets_list[n-1].widget), "control_entry",
									control_widgets_list[n].widget);
								g_object_set_data (G_OBJECT (control_widgets_list[n-1].widget), "control2_entry",
									control_widgets_list[n].widget2);
							}

							/*connect signals*/
							g_signal_connect (GTK_SCALE(control_widgets_list[n].widget), "value-changed",
								G_CALLBACK (slider_changed), NULL);
							g_signal_connect(GTK_SPIN_BUTTON(control_widgets_list[n].widget2),"value-changed",
								G_CALLBACK (spin_changed), NULL);
						}
						else
                          fprintf(stderr, "GUVCVIEW: (Invalid range) [MAX <= MIN] for control id: 0x%08x \n", current->control.id);

						break;
					}
				}
				break;

			case V4L2_CTRL_TYPE_INTEGER64:

				control_widgets_list[n].widget = gtk_entry_new();
				gtk_entry_set_max_length(GTK_ENTRY(control_widgets_list[n].widget), 19);

				//control_widgets_list[n].widget2 = gtk_button_new_from_stock(GTK_STOCK_APPLY);
				control_widgets_list[n].widget2 = gtk_button_new_with_mnemonic (_("_Apply"));

				gtk_widget_show (control_widgets_list[n].widget);
				gtk_widget_show (control_widgets_list[n].widget2);

				g_object_set_data (G_OBJECT (control_widgets_list[n].widget2), "control_info",
					GINT_TO_POINTER(current->control.id));
				g_object_set_data (G_OBJECT (control_widgets_list[n].widget2), "control_entry",
					control_widgets_list[n].widget);

				/*connect signal*/
				g_signal_connect (GTK_BUTTON(control_widgets_list[n].widget2), "clicked",
					G_CALLBACK (int64_button_clicked), NULL);

				break;

			case V4L2_CTRL_TYPE_STRING:

				control_widgets_list[n].widget = gtk_entry_new();
				gtk_entry_set_max_length(GTK_ENTRY(control_widgets_list[n].widget), current->control.maximum);

				//control_widgets_list[n].widget2= gtk_button_new_from_stock(GTK_STOCK_APPLY);
				control_widgets_list[n].widget2 = gtk_button_new_with_mnemonic (_("_Apply"));

				gtk_widget_show (control_widgets_list[n].widget);
				gtk_widget_show (control_widgets_list[n].widget2);

				g_object_set_data (G_OBJECT (control_widgets_list[n].widget2), "control_info",
					GINT_TO_POINTER(current->control.id));
				g_object_set_data (G_OBJECT (control_widgets_list[n].widget2), "control_entry",
					control_widgets_list[n].widget);

				/*connect signal*/
				g_signal_connect (GTK_BUTTON(control_widgets_list[n].widget2), "clicked",
					G_CALLBACK (string_button_clicked), NULL);
				g_signal_connect(GTK_ENTRY(control_widgets_list[n].widget),"focus-in-event",
					G_CALLBACK (entry_focus), GINT_TO_POINTER(1));
				g_signal_connect(GTK_ENTRY(control_widgets_list[n].widget),"focus-out-event",
					G_CALLBACK (entry_focus), GINT_TO_POINTER(0));

				break;

			case V4L2_CTRL_TYPE_BITMASK:

					control_widgets_list[n].widget = gtk_entry_new();

					//control_widgets_list[n].widget2 = gtk_button_new_from_stock(GTK_STOCK_APPLY);
					control_widgets_list[n].widget2 = gtk_button_new_with_mnemonic (_("_Apply"));

					gtk_widget_show (control_widgets_list[n].widget);
					gtk_widget_show (control_widgets_list[n].widget2);

					g_object_set_data (G_OBJECT (control_widgets_list[n].widget2), "control_info",
                        GINT_TO_POINTER(current->control.id));
					g_object_set_data (G_OBJECT (control_widgets_list[n].widget2), "control_entry",
						control_widgets_list[n].widget);

                    g_signal_connect (GTK_BUTTON(control_widgets_list[n].widget2), "clicked",
                        G_CALLBACK (bitmask_button_clicked), NULL);

				break;

			case V4L2_CTRL_TYPE_INTEGER_MENU:
            case V4L2_CTRL_TYPE_MENU:

				if(current->menu)
				{
					int j = 0;
					int def = 0;
					control_widgets_list[n].widget = gtk_combo_box_text_new ();

					for (j = 0; current->menu[j].index <= current->control.maximum; j++)
					{
						if(current->control.type == V4L2_CTRL_TYPE_MENU)
						{
							gtk_combo_box_text_append_text (
								GTK_COMBO_BOX_TEXT (control_widgets_list[n].widget),
								(char *) current->menu_entry[j]);
						}
						else
						{
							char buffer[30]="0";
							snprintf(buffer, 29, "%" PRId64 "", (int64_t) current->menu[j].value);
							gtk_combo_box_text_append_text (
								GTK_COMBO_BOX_TEXT (control_widgets_list[n].widget), buffer);
						}
						if(current->value == current->menu[j].index)
							def = j;
					}

					gtk_combo_box_set_active (GTK_COMBO_BOX(control_widgets_list[n].widget), def);
					gtk_widget_show (control_widgets_list[n].widget);

					g_object_set_data (G_OBJECT (control_widgets_list[n].widget), "control_info",
						GINT_TO_POINTER(current->control.id));

					/*connect signal*/
					g_signal_connect (GTK_COMBO_BOX_TEXT(control_widgets_list[n].widget), "changed",
						G_CALLBACK (combo_changed), NULL);
				}
                break;

			case V4L2_CTRL_TYPE_BUTTON:

				control_widgets_list[n].widget = gtk_button_new_with_label(" ");
				gtk_widget_show (control_widgets_list[n].widget);

				g_object_set_data (G_OBJECT (control_widgets_list[n].widget), "control_info",
					GINT_TO_POINTER(current->control.id));

				g_signal_connect (GTK_BUTTON(control_widgets_list[n].widget), "clicked",
					G_CALLBACK (button_clicked), NULL);
                break;

            case V4L2_CTRL_TYPE_BOOLEAN:

				if(current->control.id ==V4L2_CID_DISABLE_PROCESSING_LOGITECH)
				{
					control_widgets_list[n].widget2 = gtk_combo_box_text_new ();

					gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(control_widgets_list[n].widget2),
						"GBGB... | RGRG...");
					gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(control_widgets_list[n].widget2),
						"GRGR... | BGBG...");
					gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(control_widgets_list[n].widget2),
						"BGBG... | GRGR...");
					gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(control_widgets_list[n].widget2),
						"RGRG... | GBGB...");

					v4l2core_set_bayer_pix_order(get_v4l2_device_handler(), 0);

					gtk_combo_box_set_active(GTK_COMBO_BOX(control_widgets_list[n].widget2), v4l2core_get_bayer_pix_order(get_v4l2_device_handler()));

					gtk_widget_show (control_widgets_list[n].widget2);

					/*connect signal*/
					g_signal_connect (GTK_COMBO_BOX_TEXT (control_widgets_list[n].widget2), "changed",
						G_CALLBACK (bayer_pix_ord_changed), NULL);

					uint8_t isbayer = (current->value ? TRUE : FALSE);
					v4l2core_set_isbayer(get_v4l2_device_handler(), isbayer);
				}

				control_widgets_list[n].widget = gtk_check_button_new();
				gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (control_widgets_list[n].widget),
					current->value ? TRUE : FALSE);
				gtk_widget_show (control_widgets_list[n].widget);

				g_object_set_data (G_OBJECT (control_widgets_list[n].widget), "control_info",
					GINT_TO_POINTER(current->control.id));

				/*connect signal*/
				g_signal_connect (GTK_TOGGLE_BUTTON(control_widgets_list[n].widget), "toggled",
					G_CALLBACK (check_changed), NULL);

                break;

			default:
				printf("control[%d]:(unknown - 0x%x) 0x%x '%s'\n",i ,current->control.type,
					current->control.id, current->control.name);
				break;
		}

		/*attach widgets to grid*/
		gtk_grid_attach(GTK_GRID(img_controls_grid), control_widgets_list[n].label, 0, i, 1 , 1);

		if(control_widgets_list[n].widget)
		{
			gtk_grid_attach(GTK_GRID(img_controls_grid), control_widgets_list[n].widget, 1, i, 1 , 1);
            gtk_widget_set_halign (control_widgets_list[n].widget, GTK_ALIGN_FILL);
			gtk_widget_set_hexpand (control_widgets_list[n].widget, TRUE);
		}

		if(control_widgets_list[n].widget2)
		{
			gtk_grid_attach(GTK_GRID(img_controls_grid), control_widgets_list[n].widget2, 2, i, 1 , 1);
		}

        i++;
    }

	/*add control grid to parent container*/
	gtk_container_add(GTK_CONTAINER(parent), img_controls_grid);

	gui_gtk3_update_controls_state();

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
void gui_gtk3_update_controls_state()
{
	v4l2_ctrl_t *current = v4l2core_get_control_list(get_v4l2_device_handler());

    for(; current != NULL; current = current->next)
    {
		if(current == NULL)
		{
			fprintf(stderr, "GUVCVIEW: ERROR (update controls state) empty control in list\n");
			break;
		}

		control_widgets_t *cur_widget = gui_gtk3_get_widgets_by_id(current->control.id);

		if(!cur_widget)
		{
			fprintf(stderr, "GUVCVIEW: (update widget state): control %x doesn't have a widget set\n", current->control.id);
			continue;
		}

		/*update controls values*/
		switch(current->control.type)
		{
			case V4L2_CTRL_TYPE_STRING:
			{
				char *text_input = g_strescape(current->string, "");
				gtk_entry_set_text (GTK_ENTRY(cur_widget->widget), text_input);
				g_free(text_input);
				break;
			}
			case V4L2_CTRL_TYPE_BOOLEAN:
				/*disable widget signals*/
				g_signal_handlers_block_by_func(GTK_TOGGLE_BUTTON(cur_widget->widget),
					G_CALLBACK (check_changed), NULL);
				gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cur_widget->widget),
					current->value ? TRUE : FALSE);
				/*enable widget signals*/
				g_signal_handlers_unblock_by_func(GTK_TOGGLE_BUTTON(cur_widget->widget),
					G_CALLBACK (check_changed), NULL);
				break;

			case V4L2_CTRL_TYPE_BITMASK:
			{
				char *text_input = g_strdup_printf("0x%x", current->value);
				gtk_entry_set_text (GTK_ENTRY(cur_widget->widget), text_input);
				g_free(text_input);
				break;
			}

			case V4L2_CTRL_TYPE_INTEGER64:
			{
				char *text_input = g_strdup_printf("%" PRId64 "", current->value64);
				gtk_entry_set_text (GTK_ENTRY(cur_widget->widget), text_input);
				g_free(text_input);
				break;
			}

			case V4L2_CTRL_TYPE_INTEGER:
				if( current->control.id != V4L2_CID_PAN_RELATIVE &&
				    current->control.id != V4L2_CID_TILT_RELATIVE &&
				    current->control.id != V4L2_CID_PAN_RESET &&
				    current->control.id != V4L2_CID_TILT_RESET &&
				    current->control.id != V4L2_CID_LED1_MODE_LOGITECH &&
				    current->control.id != V4L2_CID_RAW_BITS_PER_PIXEL_LOGITECH )
				{
					/*disable widget signals*/
					g_signal_handlers_block_by_func(GTK_SCALE (cur_widget->widget),
						G_CALLBACK (slider_changed), NULL);
					gtk_range_set_value (GTK_RANGE (cur_widget->widget), current->value);
					/*enable widget signals*/
					g_signal_handlers_unblock_by_func(GTK_SCALE (cur_widget->widget),
						G_CALLBACK (slider_changed), NULL);

					if(cur_widget->widget2)
					{
						/*disable widget signals*/
						g_signal_handlers_block_by_func(GTK_SPIN_BUTTON(cur_widget->widget2),
							G_CALLBACK (spin_changed), NULL);
						gtk_spin_button_set_value (GTK_SPIN_BUTTON(cur_widget->widget2), current->value);
						/*enable widget signals*/
						g_signal_handlers_unblock_by_func(GTK_SPIN_BUTTON(cur_widget->widget2),
							G_CALLBACK (spin_changed), NULL);
					}
				}
				break;

			case V4L2_CTRL_TYPE_INTEGER_MENU:
			case V4L2_CTRL_TYPE_MENU:
			{
				/*disable widget signals*/
				g_signal_handlers_block_by_func(GTK_COMBO_BOX_TEXT(cur_widget->widget),
					G_CALLBACK (combo_changed), NULL);
				/*get new index*/
				int j = 0;
				int def = 0;
				for (j = 0; current->menu[j].index <= current->control.maximum; j++)
				{
					if(current->value == current->menu[j].index)
						def = j;
				}

				gtk_combo_box_set_active(GTK_COMBO_BOX(cur_widget->widget), def);
				/*enable widget signals*/
				g_signal_handlers_unblock_by_func(GTK_COMBO_BOX_TEXT(cur_widget->widget),
					G_CALLBACK (combo_changed), NULL);
				break;
			}

			default:
				break;

		}

		/*update flags (enable disable)*/
		if((current->control.flags & V4L2_CTRL_FLAG_GRABBED) ||
            (current->control.flags & V4L2_CTRL_FLAG_DISABLED))
        {
			if(cur_widget->label)
                gtk_widget_set_sensitive (cur_widget->label, FALSE);
            if(cur_widget->widget)
                gtk_widget_set_sensitive (cur_widget->widget, FALSE);
            if(cur_widget->widget2)
                gtk_widget_set_sensitive (cur_widget->widget2, FALSE);
        }
        else
        {
			if(cur_widget->label)
                gtk_widget_set_sensitive (cur_widget->label, TRUE);
            if(cur_widget->widget)
                gtk_widget_set_sensitive (cur_widget->widget, TRUE);
            if(cur_widget->widget2)
                gtk_widget_set_sensitive (cur_widget->widget2, TRUE);
        }
	}
}
