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

#include "gui.h"
/*add this last to avoid redefining _() and N_()*/
#include "gview.h"
#include "gviewrender.h"
#include "video_capture.h"
}

extern int debug_level;

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
int MainWindow::gui_attach_qt5_videoctrls(QWidget *parent)
{
	/*assertions*/
	assert(parent != NULL);

	QGridLayout *grid_layout = new QGridLayout();

	video_controls_grid = new QWidget(parent);
	video_controls_grid->setLayout(grid_layout);
	video_controls_grid->show();

	if(debug_level > 1)
		std::cout << "GUVCVIEW (Qt5): attaching video controls" << std::endl;

	int format_index = v4l2core_get_frame_format_index(
		get_v4l2_device_handler(),
		v4l2core_get_requested_frame_format(get_v4l2_device_handler()));

	if(format_index < 0)
	{
		gui_error("Guvcview error", "invalid pixel format", 0);
		std::cerr << "GUVCVIEW (Qt5): invalid pixel format" << std::endl;
	}

	int resolu_index = v4l2core_get_format_resolution_index(
		get_v4l2_device_handler(),
		format_index,
		v4l2core_get_frame_width(get_v4l2_device_handler()),
		v4l2core_get_frame_height(get_v4l2_device_handler()));

	if(resolu_index < 0)
	{
		gui_error("Guvcview error", "invalid resolution index", 0);
		std::cerr << "GUVCVIEW (Qt5): invalid resolution index" << std::endl;
	}

	int line = 0;
	int i =0;

	/*---- Devices ----*/
	QLabel *label_Device = new QLabel(_("Device:"),video_controls_grid);
	label_Device->show();

	grid_layout->addWidget(label_Device, line, 0, Qt::AlignRight);

	combobox_video_devices = new QComboBox(video_controls_grid);
	combobox_video_devices->show();

	if (v4l2core_get_num_devices() < 1)
	{
		/*use current*/
		combobox_video_devices->addItem(v4l2core_get_videodevice(get_v4l2_device_handler()), 0);
		combobox_video_devices->setCurrentIndex(0);
	}
	else
	{
		for(i = 0; i < v4l2core_get_num_devices(); i++)
		{
			combobox_video_devices->addItem(v4l2core_get_device_sys_data(i)->name, i);
			if(v4l2core_get_device_sys_data(i)->current)
				combobox_video_devices->setCurrentIndex(i);
		}
	}
	grid_layout->addWidget(combobox_video_devices, line, 1);
	/*signals*/
	connect(combobox_video_devices, SIGNAL(currentIndexChanged(int)), this, SLOT(devices_changed(int)));

	/*---- Frame Rate ----*/
	line++;

	QLabel *label_FPS = new QLabel(_("Frame Rate:"),video_controls_grid);
	label_FPS->show();

	grid_layout->addWidget(label_FPS, line, 0, Qt::AlignRight);

	combobox_FrameRate = new QComboBox(video_controls_grid);
	combobox_FrameRate->show();

	int deffps=0;

	v4l2_stream_formats_t *list_stream_formats = v4l2core_get_formats_list(get_v4l2_device_handler());

	if (debug_level > 0)
		std::cout << "GUVCVIEW (Qt5): frame rates of resolution index "
			<< resolu_index+1 << " = "
			<< list_stream_formats[format_index].list_stream_cap[resolu_index].numb_frates
			<< std::endl;
	for ( i = 0 ; i < list_stream_formats[format_index].list_stream_cap[resolu_index].numb_frates ; ++i)
	{
		QString fps_str = QString( "%1/%2 fps").arg(list_stream_formats[format_index].list_stream_cap[resolu_index].framerate_denom[i]).arg(list_stream_formats[format_index].list_stream_cap[resolu_index].framerate_num[i]);

		combobox_FrameRate->addItem(fps_str, i);

		if (( v4l2core_get_fps_num(get_v4l2_device_handler()) == list_stream_formats[format_index].list_stream_cap[resolu_index].framerate_num[i]) &&
			( v4l2core_get_fps_denom(get_v4l2_device_handler()) == list_stream_formats[format_index].list_stream_cap[resolu_index].framerate_denom[i]))
				deffps=i;//set selected
	}
	combobox_FrameRate->setCurrentIndex(deffps);

	grid_layout->addWidget(combobox_FrameRate, line, 1);

	if (deffps==0)
	{
		if (list_stream_formats[format_index].list_stream_cap[resolu_index].framerate_denom)
			v4l2core_define_fps(
				get_v4l2_device_handler(),
				-1,
				list_stream_formats[format_index].list_stream_cap[resolu_index].framerate_denom[0]);

		if (list_stream_formats[format_index].list_stream_cap[resolu_index].framerate_num)
			v4l2core_define_fps(
				get_v4l2_device_handler(),
				list_stream_formats[format_index].list_stream_cap[resolu_index].framerate_num[0],
				-1);
	}
	/*signals*/
	connect(combobox_FrameRate, SIGNAL(currentIndexChanged(int)), this, SLOT(frame_rate_changed(int)));

	/*try to sync the device fps (capture thread must have started by now)*/
	v4l2core_request_framerate_update (get_v4l2_device_handler());

	/*---- Resolution ----*/
	line++;

	QLabel *label_Resol = new QLabel(_("Resolution:"),video_controls_grid);
	label_Resol->show();

	grid_layout->addWidget(label_Resol, line, 0, Qt::AlignRight);

	combobox_resolution = new QComboBox(video_controls_grid);
	combobox_resolution->show();

	int defres=0;

	if (debug_level > 0)
		std::cout << "GUVCVIEW (Qt5): resolutions of format "
			<< format_index+1 << " = "
			<< list_stream_formats[format_index].numb_res
			<< std::endl;
	for(i = 0 ; i < list_stream_formats[format_index].numb_res ; i++)
	{
		if (list_stream_formats[format_index].list_stream_cap[i].width > 0)
		{
			QString res_str = QString( "%1x%2").arg(list_stream_formats[format_index].list_stream_cap[i].width).arg(list_stream_formats[format_index].list_stream_cap[i].height);
			combobox_resolution->addItem(res_str, i);

			if ((v4l2core_get_frame_width(get_v4l2_device_handler()) == list_stream_formats[format_index].list_stream_cap[i].width) &&
				(v4l2core_get_frame_height(get_v4l2_device_handler()) == list_stream_formats[format_index].list_stream_cap[i].height))
					defres=i;//set selected resolution index
		}
	}
	combobox_resolution->setCurrentIndex(defres);

	grid_layout->addWidget(combobox_resolution, line, 1);
	/*signals*/
	connect(combobox_resolution, SIGNAL(currentIndexChanged(int)), this, SLOT(resolution_changed(int)));



	///*---- Input Format ----*/
	line++;

	QLabel *label_InpType = new QLabel(_("Camera Output:"),video_controls_grid);

	label_InpType->show();

	grid_layout->addWidget(label_InpType, line, 0, Qt::AlignRight);

	combobox_InpType = new QComboBox(video_controls_grid);
	QListWidget *contents = new QListWidget(combobox_InpType);
	contents->hide();
	combobox_InpType->setModel(contents->model());
	combobox_InpType->show();

	int fmtind=0;
	for (fmtind=0; fmtind < v4l2core_get_number_formats(get_v4l2_device_handler()); fmtind++)
	{
		QString fourcc(list_stream_formats[fmtind].fourcc);
		if((list_stream_formats[fmtind].format & (1<<31)) != 0)
			fourcc += "(BE)";
		fourcc += " - " + QString(list_stream_formats[fmtind].description);

		combobox_InpType->addItem(fourcc, fmtind);

		if(!list_stream_formats[fmtind].dec_support)
		{
			QListWidgetItem *item = contents->item(fmtind);
			item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
		}
		if(v4l2core_get_requested_frame_format(get_v4l2_device_handler()) == list_stream_formats[fmtind].format)
			combobox_InpType->setCurrentIndex(fmtind); /*set active*/
	}

	grid_layout->addWidget(combobox_InpType, line, 1);
	/*signals*/
	connect(combobox_InpType, SIGNAL(currentIndexChanged(int)), this, SLOT(format_changed(int)));

	/* ----- Filter controls -----*/
	line++;
	QLabel *label_videoFilters = new QLabel(_("---- Video Filters ----"),video_controls_grid);
	label_videoFilters->show();

	grid_layout->addWidget(label_videoFilters, line, 0, 1, 2, Qt::AlignCenter);

	/*filters grid*/
	line++;
	QWidget *table_filt = new QWidget(video_controls_grid);
	QGridLayout *filt_layout = new QGridLayout();
	table_filt->setLayout(filt_layout);
	table_filt->show();
	grid_layout->addWidget(table_filt, line, 0, 1, 2);

	/* Mirror FX */
	QCheckBox *FiltMirrorEnable = new QCheckBox(_(" Mirror"), table_filt);
	FiltMirrorEnable->setProperty("filt_info", REND_FX_YUV_MIRROR);
	FiltMirrorEnable->show();
	filt_layout->addWidget(FiltMirrorEnable, 0, 0);
	FiltMirrorEnable->setChecked((get_render_fx_mask() & REND_FX_YUV_MIRROR) > 0);
	/*connect signal*/
	connect(FiltMirrorEnable, SIGNAL(stateChanged(int)), this, SLOT(render_fx_filter_changed(int)));

        /* Half Mirror FX */
	QCheckBox *FiltHalfMirrorEnable = new QCheckBox(_(" Half Mirror"), table_filt);
	FiltHalfMirrorEnable->setProperty("filt_info", REND_FX_YUV_HALF_MIRROR);
	FiltHalfMirrorEnable->show();
	filt_layout->addWidget(FiltHalfMirrorEnable, 0, 1);
	FiltHalfMirrorEnable->setChecked((get_render_fx_mask() & REND_FX_YUV_HALF_MIRROR) > 0);
	/*connect signal*/
	connect(FiltHalfMirrorEnable, SIGNAL(stateChanged(int)), this, SLOT(render_fx_filter_changed(int)));

	/* Upturn FX */
	QCheckBox *FiltUpturnEnable = new QCheckBox(_(" Invert"), table_filt);
	FiltUpturnEnable->setProperty("filt_info", REND_FX_YUV_UPTURN);
	FiltUpturnEnable->show();

	filt_layout->addWidget(FiltUpturnEnable, 0, 2);
	FiltUpturnEnable->setChecked((get_render_fx_mask() & REND_FX_YUV_UPTURN) > 0);
	/*connect signal*/
	connect(FiltUpturnEnable, SIGNAL(stateChanged(int)), this, SLOT(render_fx_filter_changed(int)));

        /* Half Upturn FX */
	QCheckBox *FiltHalfUpturnEnable = new QCheckBox(_(" Half Invert"), table_filt);
	FiltHalfUpturnEnable->setProperty("filt_info", REND_FX_YUV_HALF_UPTURN);
	FiltHalfUpturnEnable->show();

	filt_layout->addWidget(FiltHalfUpturnEnable, 0, 3);
	FiltHalfUpturnEnable->setChecked((get_render_fx_mask() & REND_FX_YUV_HALF_UPTURN) > 0);
	/*connect signal*/
	connect(FiltHalfUpturnEnable, SIGNAL(stateChanged(int)), this, SLOT(render_fx_filter_changed(int)));

	/* Negate FX */
	QCheckBox *FiltNegateEnable = new QCheckBox(_(" Negative"), table_filt);
	FiltNegateEnable->setProperty("filt_info", REND_FX_YUV_NEGATE);
	FiltNegateEnable->show();

	filt_layout->addWidget(FiltNegateEnable, 0, 4);
	FiltNegateEnable->setChecked((get_render_fx_mask() & REND_FX_YUV_NEGATE) > 0);
	/*connect signal*/
	connect(FiltNegateEnable, SIGNAL(stateChanged(int)), this, SLOT(render_fx_filter_changed(int)));

	/* Mono FX */
	QCheckBox *FiltMonoEnable = new QCheckBox(_(" Mono"), table_filt);
	FiltMonoEnable->setProperty("filt_info", REND_FX_YUV_MONOCR);
	FiltMonoEnable->show();

	filt_layout->addWidget(FiltMonoEnable, 0, 5);
	FiltMonoEnable->setChecked((get_render_fx_mask() & REND_FX_YUV_MONOCR) > 0);
	/*connect signal*/
	connect(FiltMonoEnable, SIGNAL(stateChanged(int)), this, SLOT(render_fx_filter_changed(int)));

	/* Pieces FX */
	QCheckBox *FiltPiecesEnable = new QCheckBox(_(" Pieces"), table_filt);
	FiltPiecesEnable->setProperty("filt_info", REND_FX_YUV_PIECES);
	FiltPiecesEnable->show();

	filt_layout->addWidget(FiltPiecesEnable, 1, 0);
	FiltPiecesEnable->setChecked((get_render_fx_mask() & REND_FX_YUV_PIECES) > 0);
	/*connect signal*/
	connect(FiltPiecesEnable, SIGNAL(stateChanged(int)), this, SLOT(render_fx_filter_changed(int)));

	/* Particles */
	QCheckBox *FiltParticlesEnable = new QCheckBox(_(" Particles"), table_filt);
	FiltParticlesEnable->setProperty("filt_info", REND_FX_YUV_PARTICLES);
	FiltParticlesEnable->show();

	filt_layout->addWidget(FiltParticlesEnable, 1, 1);
	FiltParticlesEnable->setChecked((get_render_fx_mask() & REND_FX_YUV_PARTICLES) > 0);
	/*connect signal*/
	connect(FiltParticlesEnable, SIGNAL(stateChanged(int)), this, SLOT(render_fx_filter_changed(int)));

        /* SQRT Lens Distort */
	QCheckBox *FiltSqrtLensEnable = new QCheckBox(_(" Sqrt Lens"), table_filt);
	FiltSqrtLensEnable->setProperty("filt_info", REND_FX_YUV_SQRT_DISTORT);
	FiltSqrtLensEnable->show();

	filt_layout->addWidget(FiltSqrtLensEnable, 1, 2);
	FiltSqrtLensEnable->setChecked((get_render_fx_mask() & REND_FX_YUV_SQRT_DISTORT) > 0);
	/*connect signal*/
	connect(FiltSqrtLensEnable, SIGNAL(stateChanged(int)), this, SLOT(render_fx_filter_changed(int)));

        /* POW Lens Distort */
	QCheckBox *FiltPowLensEnable = new QCheckBox(_(" Pow Lens"), table_filt);
	FiltPowLensEnable->setProperty("filt_info", REND_FX_YUV_POW_DISTORT);
	FiltPowLensEnable->show();

	filt_layout->addWidget(FiltPowLensEnable, 1, 3);
	FiltPowLensEnable->setChecked((get_render_fx_mask() & REND_FX_YUV_POW_DISTORT) > 0);
	/*connect signal*/
	connect(FiltPowLensEnable, SIGNAL(stateChanged(int)), this, SLOT(render_fx_filter_changed(int)));

        /* POW2 Lens Distort */
	QCheckBox *FiltPow2LensEnable = new QCheckBox(_(" Pow2 Lens"), table_filt);
	FiltPow2LensEnable->setProperty("filt_info", REND_FX_YUV_POW2_DISTORT);
	FiltPow2LensEnable->show();

	filt_layout->addWidget(FiltPow2LensEnable, 1, 4);
	FiltPow2LensEnable->setChecked((get_render_fx_mask() & REND_FX_YUV_POW2_DISTORT) > 0);
	/*connect signal*/
	connect(FiltPow2LensEnable, SIGNAL(stateChanged(int)), this, SLOT(render_fx_filter_changed(int)));

        /* Blur*/
	QCheckBox *FiltBlurEnable = new QCheckBox(_(" Blur"), table_filt);
	FiltBlurEnable->setProperty("filt_info", REND_FX_YUV_BLUR);
	FiltBlurEnable->show();

	filt_layout->addWidget(FiltBlurEnable, 1, 5);
	FiltBlurEnable->setChecked((get_render_fx_mask() & REND_FX_YUV_BLUR) > 0);
	/*connect signal*/
	connect(FiltBlurEnable, SIGNAL(stateChanged(int)), this, SLOT(render_fx_filter_changed(int)));

        /* Blur 2*/
	QCheckBox *FiltBlur2Enable = new QCheckBox(_(" Blur more"), table_filt);
	FiltBlur2Enable->setProperty("filt_info", REND_FX_YUV_BLUR2);
	FiltBlur2Enable->show();

	filt_layout->addWidget(FiltBlur2Enable, 2, 0);
	FiltBlur2Enable->setChecked((get_render_fx_mask() & REND_FX_YUV_BLUR2) > 0);
	/*connect signal*/
	connect(FiltBlur2Enable, SIGNAL(stateChanged(int)), this, SLOT(render_fx_filter_changed(int)));

	/* ----- OSD controls -----*/
	line++;
	QLabel *label_osd = new QLabel(_("---- OSD ----"),video_controls_grid);
	label_osd->show();

	grid_layout->addWidget(label_osd, line, 0, 1, 2, Qt::AlignCenter);

	/*osd grid*/
	line++;
	QWidget *table_osd = new QWidget(video_controls_grid);
	QGridLayout *osd_layout = new QGridLayout();
	table_osd->setLayout(osd_layout);
	table_osd->show();
	grid_layout->addWidget(table_osd, line, 0, 1, 2);

	/* Crosshair OSD */
	QCheckBox *OsdCrosshairEnable = new QCheckBox(_(" Cross-hair"), table_osd);
	OsdCrosshairEnable->setProperty("osd_info", REND_OSD_CROSSHAIR);
	OsdCrosshairEnable->show();

	osd_layout->addWidget(OsdCrosshairEnable, 0, 0);
	OsdCrosshairEnable->setChecked((render_get_osd_mask() & REND_OSD_CROSSHAIR) > 0);
	/*connect signal*/
	connect(OsdCrosshairEnable, SIGNAL(stateChanged(int)), this, SLOT(render_osd_changed(int)));


	line++;
	QSpacerItem *spacer = new QSpacerItem(40, 20, QSizePolicy::Minimum, QSizePolicy::Expanding);
	grid_layout->addItem(spacer, line, 0);
	QSpacerItem *hspacer = new QSpacerItem(40, 20, QSizePolicy::Expanding);
	grid_layout->addItem(hspacer, line, 1);

	return 0;
}
