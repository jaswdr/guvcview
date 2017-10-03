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

#include "../config.h"
#include "gui.h"
#include "gviewrender.h"
#include "video_capture.h"
/*add this last to avoid redefining _() and N_()*/
#include "gview.h"
}

extern int debug_level;
extern int is_control_panel;

/*
 * update controls from commit probe data
 * args:
 *    none
 *
 * asserts:
 *    none
 *
 * returns: none
 */
void MainWindow::update_h264_controls()
{
	uvcx_video_config_probe_commit_t *config_probe_req = v4l2core_get_h264_config_probe_req(get_v4l2_device_handler());

	//dwBitRate
	BitRate->setValue(config_probe_req->dwBitRate);
	//bmHints
	uint16_t hints = config_probe_req->bmHints;
	Hints_res->setChecked((hints & 0x0001) != 0);
	Hints_prof->setChecked((hints & 0x0002) != 0);
	Hints_ratecontrol->setChecked((hints & 0x0004) != 0);
	Hints_usage->setChecked((hints & 0x0008) != 0);
	Hints_slicemode->setChecked((hints & 0x0010) != 0);
	Hints_sliceunit->setChecked((hints & 0x0020) != 0);
	Hints_view->setChecked((hints & 0x0040) != 0);
	Hints_temporal->setChecked((hints & 0x0080) != 0);
	Hints_snr->setChecked((hints & 0x0100) != 0);
	Hints_spatial->setChecked((hints & 0x0200) != 0);
	Hints_spatiallayer->setChecked((hints & 0x0400) != 0);
	Hints_frameinterval->setChecked((hints & 0x0800) != 0);
	Hints_leakybucket->setChecked((hints & 0x1000) != 0);
	Hints_bitrate->setChecked((hints & 0x2000) != 0);
	Hints_cabac->setChecked((hints & 0x4000) != 0);
	Hints_iframe->setChecked((hints & 0x8000) != 0);
	//wWidth x wHeight

	//wSliceMode
	SliceMode->setCurrentIndex(config_probe_req->wSliceMode);
	//wSliceUnits
	SliceUnits->setValue(config_probe_req->wSliceUnits);
	//wProfile
	uint16_t profile = config_probe_req->wProfile & 0xFF00;
	int prof_index = 0;
	switch(profile)
	{
		case 0x4200:
			prof_index = 0;
			break;
		case 0x4D00:
			prof_index = 1;
			break;
		case 0x6400:
			prof_index = 2;
			break;
		case 0x5300:
			prof_index = 3;
			break;
		case 0x5600:
			prof_index = 4;
			break;
		case 0x7600:
			prof_index = 5;
			break;
		case 0x8000:
			prof_index = 6;
			break;
		default:
			fprintf(stderr, "GUVCVIEW (H264 probe) unknown profile mode 0x%X\n", profile);
			break;

	}
	Profile->setCurrentIndex(prof_index);
	Profile_flags->setValue(config_probe_req->wProfile & 0x00FF);
	//wIFramePeriod
	IFramePeriod->setValue(config_probe_req->wIFramePeriod);
	//wEstimatedVideoDelay
	EstimatedVideoDelay->setValue(config_probe_req->wEstimatedVideoDelay);
	//wEstimatedMaxConfigDelay
	EstimatedMaxConfigDelay->setValue(config_probe_req->wEstimatedMaxConfigDelay);
	//bUsageType
	int usage_type_ind = (config_probe_req->bUsageType & 0x0F) - 1;
	if(usage_type_ind < 0)
		usage_type_ind = 0;
	UsageType->setCurrentIndex(usage_type_ind);
	//bRateControlMode
	RateControlMode->setCurrentIndex((config_probe_req->bRateControlMode & 0x03) - 1);
	RateControlMode_cbr_flag->setValue(config_probe_req->bRateControlMode & 0x0000001C);
	//bTemporalScaleMode
	TemporalScaleMode->setValue(config_probe_req->bTemporalScaleMode);
	//bSpatialScaleMode
	SpatialScaleMode->setValue(config_probe_req->bSpatialScaleMode);
	//bSNRScaleMode
	uint8_t snrscalemode = config_probe_req->bSNRScaleMode & 0x0F;
	int snrscalemode_index = 0;
	switch(snrscalemode)
	{
		case 0:
			snrscalemode_index = 0;
			break;
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
			snrscalemode_index = snrscalemode - 1;
			break;
	}
	SNRScaleMode->setCurrentIndex(snrscalemode_index);
	//bStreamMuxOption
	StreamMuxOption->setChecked((config_probe_req->bStreamMuxOption & 0x01) != 0);
	uint8_t streammux = config_probe_req->bStreamMuxOption & 0x0E;
	int streammux_index = 0;
	switch(streammux)
	{
		case 2:
			streammux_index = 0;
			break;
		case 4:
			streammux_index = 1;
			break;
		case 8:
			streammux_index = 2;
			break;
	}
	StreamMuxOption_aux->setCurrentIndex(streammux_index);
	StreamMuxOption_mjpgcontainer->setChecked((config_probe_req->bStreamMuxOption & 0x40) != 0);
	//bStreamFormat
	StreamFormat->setCurrentIndex(config_probe_req->bStreamMuxOption & 0x01);
	//bEntropyCABAC
	EntropyCABAC->setCurrentIndex(config_probe_req->bEntropyCABAC & 0x01);
	//bTimestamp
	Timestamp->setChecked((config_probe_req->bTimestamp & 0x01) != 0);
	//bNumOfReorderFrames
	NumOfReorderFrames->setValue(config_probe_req->bNumOfReorderFrames);
	//bPreviewFlipped
	PreviewFlipped->setChecked((config_probe_req->bPreviewFlipped & 0x01) != 0);
	//bView
	View->setValue(config_probe_req->bView);
	//bStreamID
	StreamID->setValue(config_probe_req->bStreamID);
	//bSpatialLayerRatio
	int SLRatio = config_probe_req->bSpatialLayerRatio & 0x000000FF;
	double val = (double) ((SLRatio & 0x000000F0)>>4) + (double)((SLRatio & 0x0000000F)/16);
	SpatialLayerRatio->setValue(val);
	//wLeakyBucketSize
	LeakyBucketSize->setValue(config_probe_req->wLeakyBucketSize);
}

/*
 * fill commit probe data from control values
 * args:
 *    none
 *
 * asserts:
 *    none
 *
 * returns: none
 */
void MainWindow::fill_video_config_probe ()
{

	uvcx_video_config_probe_commit_t *config_probe_req = v4l2core_get_h264_config_probe_req(get_v4l2_device_handler());

	//dwFrameInterval
	uint32_t frame_interval = (v4l2core_get_fps_num(get_v4l2_device_handler()) * 1000000000LL / v4l2core_get_fps_denom(get_v4l2_device_handler()))/100;
	config_probe_req->dwFrameInterval = frame_interval;//(uint32_t) gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(h264_controls->FrameInterval));
	//dwBitRate
	config_probe_req->dwBitRate = (uint32_t) BitRate->value();
	//bmHints
	uint16_t hints = 0x0000;
	hints |= Hints_res->isChecked() ?  0x0001 :  0;
	hints |= Hints_prof->isChecked() ? 0x0002 : 0;
	hints |= Hints_ratecontrol->isChecked() ? 0x0004: 0;
	hints |= Hints_usage->isChecked() ? 0x0008: 0;
	hints |= Hints_slicemode->isChecked() ? 0x0010: 0;
	hints |= Hints_sliceunit->isChecked() ? 0x0020: 0;
	hints |= Hints_view->isChecked() ? 0x0040: 0;
	hints |= Hints_temporal->isChecked() ? 0x0080: 0;
	hints |= Hints_snr->isChecked() ? 0x0100: 0;
	hints |= Hints_spatial->isChecked() ? 0x0200: 0;
	hints |= Hints_spatiallayer->isChecked() ? 0x0400: 0;
	hints |= Hints_frameinterval->isChecked() ? 0x0800: 0;
	hints |= Hints_leakybucket->isChecked() ? 0x1000: 0;
	hints |= Hints_bitrate->isChecked() ? 0x2000: 0;
	hints |= Hints_cabac->isChecked() ? 0x4000: 0;
	hints |= Hints_iframe->isChecked() ? 0x8000: 0;
	config_probe_req->bmHints = hints;
	//wWidth x wHeight
	config_probe_req->wWidth = (uint16_t) v4l2core_get_frame_width(get_v4l2_device_handler());
	config_probe_req->wHeight = (uint16_t) v4l2core_get_frame_height(get_v4l2_device_handler());
	//wSliceMode
	config_probe_req->wSliceMode = (uint16_t) SliceMode->currentIndex();
	//wSliceUnits
	config_probe_req->wSliceUnits = (uint16_t) SliceUnits->value();
	//wProfile
	uint16_t profile = (uint16_t) Profile->currentData().toInt();

	profile |= (uint16_t) (Profile_flags->value() & 0x000000FF);
	config_probe_req->wProfile = profile;
	//wIFramePeriod
	config_probe_req->wIFramePeriod = (uint16_t) IFramePeriod->value();
	//wEstimatedVideoDelay
	config_probe_req->wEstimatedVideoDelay = (uint16_t) EstimatedVideoDelay->value();
	//wEstimatedMaxConfigDelay
	config_probe_req->wEstimatedMaxConfigDelay = (uint16_t) EstimatedMaxConfigDelay->value();
	//bUsageType
	config_probe_req->bUsageType = (uint8_t) UsageType->currentData().toInt();
	//bRateControlMode
	config_probe_req->bRateControlMode = (uint8_t) RateControlMode->currentData().toInt();
	config_probe_req->bRateControlMode |= (uint8_t) (RateControlMode_cbr_flag->value() & 0x0000001C);
	//bTemporalScaleMode
	config_probe_req->bTemporalScaleMode = (uint8_t) TemporalScaleMode->value();
	//bSpatialScaleMode
	config_probe_req->bSpatialScaleMode = (uint8_t) SpatialScaleMode->value();
	//bSNRScaleMode
	config_probe_req->bSNRScaleMode = SNRScaleMode->currentData().toInt();
	//bStreamMuxOption
	//uint8_t streammux = StreamMuxOption->isChecked() ? 0x01: 0;
	//streammux |= (uint8_t) (StreamMuxOption_aux->currentData().toInt() & 0x0F);
	//streammux |= StreamMuxOption_mjpgcontainer->isChecked() ? 0x40 : 0x00;
	//config_probe_req->bStreamMuxOption = streammux;
	//bStreamFormat
	//config_probe_req->bStreamFormat = (uint8_t) (StreamFormat->currentData().toInt() & 0x01);
	//bEntropyCABAC
	config_probe_req->bEntropyCABAC = (uint8_t) (EntropyCABAC->currentIndex() & 0x01);
	//bTimestamp
	config_probe_req->bTimestamp = Timestamp->isChecked() ? 0x01 : 0x00;
	//bNumOfReorderFrames
	config_probe_req->bNumOfReorderFrames = (uint8_t) NumOfReorderFrames->value();
	//bPreviewFlipped
	config_probe_req->bPreviewFlipped = PreviewFlipped->isChecked() ? 0x01 : 0x00;
	//bView
	config_probe_req->bView = (uint8_t) View->value();
	//bStreamID
	config_probe_req->bStreamID = (uint8_t) StreamID->value();
	//bSpatialLayerRatio
	double spatialratio = SpatialLayerRatio->value();
	uint8_t high_nibble = floor(spatialratio);
	uint8_t lower_nibble = floor((spatialratio - high_nibble) * 16);
	config_probe_req->bSpatialLayerRatio = (high_nibble << 4) + lower_nibble;
	//wLeakyBucketSize
	config_probe_req->wLeakyBucketSize = (uint16_t) LeakyBucketSize->value();
}
/*
 * h264 video rate control mode callback
 * args:
 *   index - current combobox index
 *
 * asserts:
 *   none
 *
 * returns: none
 */
void MainWindow::h264_rate_control_mode_changed(int index)
{
	uint8_t rate_mode = (uint8_t) RateControlMode->itemData(index).toInt();

	rate_mode |= (uint8_t) (RateControlMode_cbr_flag->value() & 0x0000001C);

	v4l2core_set_h264_video_rate_control_mode(get_v4l2_device_handler(), rate_mode);

	rate_mode = v4l2core_get_h264_video_rate_control_mode(get_v4l2_device_handler(), UVC_GET_CUR);

	int ratecontrolmode_index = rate_mode - 1; // from 0x01 to 0x03
	if(ratecontrolmode_index < 0)
		ratecontrolmode_index = 0;

	RateControlMode->blockSignals(true);
	RateControlMode->setCurrentIndex(ratecontrolmode_index);
	RateControlMode->blockSignals(false);
}

/*
 * h264 temporal scale mode callback
 * args:
 *   value - new value
 *
 * asserts:
 *   none
 *
 * returns: none
 */
void MainWindow::h264_TemporalScaleMode_changed(int value)
{
	uint8_t scale_mode = (uint8_t) TemporalScaleMode->value();

	v4l2core_set_h264_temporal_scale_mode(get_v4l2_device_handler(), scale_mode);

	scale_mode = v4l2core_get_h264_temporal_scale_mode(get_v4l2_device_handler(), UVC_GET_CUR) & 0x07;

	TemporalScaleMode->blockSignals(true);
	TemporalScaleMode->setValue(scale_mode);
	TemporalScaleMode->blockSignals(false);
}

/*
 * h264 spatial scale mode callback
 * args:
 *   value - spin value
 *
 * asserts:
 *   none
 *
 * returns: none
 */
void MainWindow::h264_SpatialScaleMode_changed(int value)
{
	uint8_t scale_mode = (uint8_t) SpatialScaleMode->value();

	v4l2core_set_h264_spatial_scale_mode(get_v4l2_device_handler(), scale_mode);

	scale_mode = v4l2core_get_h264_spatial_scale_mode(get_v4l2_device_handler(), UVC_GET_CUR) & 0x07;

	SpatialScaleMode->blockSignals(true);
	SpatialScaleMode->setValue(scale_mode);
	SpatialScaleMode->blockSignals(false);
}

/*
 * h264 Frame Interval callback
 * args:
 *   value - spin value
 *
 * asserts:
 *   none
 *
 * returns: none
 */
void MainWindow::h264_FrameInterval_changed(int value)
{
	uint32_t framerate = (uint32_t) FrameInterval->value();

	v4l2core_set_h264_frame_rate_config(get_v4l2_device_handler(), framerate);

	framerate = v4l2core_get_h264_frame_rate_config(get_v4l2_device_handler());

	FrameInterval->blockSignals(true);
	FrameInterval->setValue(framerate);
	FrameInterval->blockSignals(false);
}

/*
 * h264 commit button callback
 * args:
 *   none
 *
 * asserts:
 *   none
 *
 * returns: none
 */
void MainWindow::h264_commit_button_clicked()
{
	fill_video_config_probe();

	v4l2core_set_h264_no_probe_default(get_v4l2_device_handler(), 1);

	request_format_update();

	int counter = 0;

	/*wait for device->h264_no_probe = 0*/
	struct timespec req = {
		.tv_sec = 0,
		.tv_nsec = 50000000};/*nanosec*/

	while(v4l2core_get_h264_no_probe_default(get_v4l2_device_handler()) > 0 && counter < 10)
	{
		nanosleep(&req, NULL);
		counter++;
	}

	/*make sure we reset this flag, although the core probably handle it already*/
	v4l2core_set_h264_no_probe_default(get_v4l2_device_handler(), 0);

	update_h264_controls();
}

/*
 * h264 reset button callback
 * args:
 *   none
 *
 * asserts:
 *   none
 *
 * returns: none
 */
void MainWindow::h264_reset_button_clicked()
{
	v4l2core_reset_h264_encoder(get_v4l2_device_handler());
}

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
int MainWindow::gui_attach_qt5_h264ctrls (QWidget *parent)
{
	/*assertions*/
	assert(parent != NULL);

	if(debug_level > 1)
		std::cout << "GUVCVIEW (Qt5): attaching H264 controls" <<std::endl;


	//get current values
	v4l2core_probe_h264_config_probe_req(get_v4l2_device_handler(), UVC_GET_CUR, NULL);

	//get Max values
	uvcx_video_config_probe_commit_t config_probe_max;
	v4l2core_probe_h264_config_probe_req(get_v4l2_device_handler(), UVC_GET_MAX, &config_probe_max);

	//get Min values
	uvcx_video_config_probe_commit_t config_probe_min;
	v4l2core_probe_h264_config_probe_req(get_v4l2_device_handler(), UVC_GET_MIN, &config_probe_min);

	QGridLayout *grid_layout = new QGridLayout();

	h264_controls_grid = new QWidget(parent);
	h264_controls_grid->setLayout(grid_layout);
	h264_controls_grid->show();

	int line = 0;

	//streaming controls

	//bRateControlMode
	QLabel *label_RateControlMode = new QLabel(_("Rate Control Mode:"),
		h264_controls_grid);
	label_RateControlMode->show();
	grid_layout->addWidget(label_RateControlMode, line, 0, Qt::AlignRight);

	uint8_t min_ratecontrolmode = v4l2core_get_h264_video_rate_control_mode(get_v4l2_device_handler(), UVC_GET_MIN) & 0x03;
	uint8_t max_ratecontrolmode = v4l2core_get_h264_video_rate_control_mode(get_v4l2_device_handler(), UVC_GET_MAX) & 0x03;

	RateControlMode = new QComboBox(h264_controls_grid);
	RateControlMode->show();
	grid_layout->addWidget(RateControlMode, line, 1);
	
	if(max_ratecontrolmode >= 1 && min_ratecontrolmode < 2)
		RateControlMode->addItem(_("CBR"), 1);
	if(max_ratecontrolmode >= 2 && min_ratecontrolmode < 3)
		RateControlMode->addItem(_("VBR"), 2);

	if(max_ratecontrolmode >= 3 && min_ratecontrolmode < 4)
		RateControlMode->addItem(_("Constant QP"), 3);

	uint8_t cur_ratecontrolmode = v4l2core_get_h264_video_rate_control_mode(get_v4l2_device_handler(), UVC_GET_CUR) & 0x03;
	int ratecontrolmode_index = cur_ratecontrolmode - 1; // from 0x01 to 0x03
	if(ratecontrolmode_index < 0)
		ratecontrolmode_index = 0;

	RateControlMode->setCurrentIndex(ratecontrolmode_index);

	//connect signal
	connect(RateControlMode, SIGNAL(currentIndexChanged(int)), 
		this, SLOT(h264_rate_control_mode_changed(int)));

	//bRateControlMode flags (Bits 4-8)
	line++;
	QLabel* label_RateControlMode_cbr_flag = new QLabel(_("Rate Control Mode flags:"),
		h264_controls_grid);
	label_RateControlMode_cbr_flag->show();
	grid_layout->addWidget(label_RateControlMode_cbr_flag, line, 0, Qt::AlignRight);
	
	uint8_t cur_vrcflags = v4l2core_get_h264_video_rate_control_mode(get_v4l2_device_handler(), UVC_GET_CUR) & 0x1C;
	uint8_t max_vrcflags = v4l2core_get_h264_video_rate_control_mode(get_v4l2_device_handler(), UVC_GET_MAX) & 0x1C;
	uint8_t min_vrcflags = v4l2core_get_h264_video_rate_control_mode(get_v4l2_device_handler(), UVC_GET_MIN) & 0x1C;

	RateControlMode_cbr_flag = new QSpinBox(h264_controls_grid);
	RateControlMode_cbr_flag->show();
	grid_layout->addWidget(RateControlMode_cbr_flag, line, 1);
	
	RateControlMode_cbr_flag->setRange(min_vrcflags, max_vrcflags);
	RateControlMode_cbr_flag->setSingleStep(1);
	RateControlMode_cbr_flag->setValue(cur_vrcflags);

	//bTemporalScaleMode
	line++;
	QLabel *label_TemporalScaleMode = new QLabel(_("Temporal Scale Mode:"),
		h264_controls_grid);
	label_TemporalScaleMode->show();
	grid_layout->addWidget(label_TemporalScaleMode, line, 0, Qt::AlignRight);
	
	uvcx_video_config_probe_commit_t *h264_config_probe_req = v4l2core_get_h264_config_probe_req(get_v4l2_device_handler());
	
	uint8_t cur_tsmflags = h264_config_probe_req->bTemporalScaleMode & 0x07;
	uint8_t max_tsmflags = config_probe_max.bTemporalScaleMode & 0x07;
	uint8_t min_tsmflags = config_probe_min.bTemporalScaleMode & 0x07;
	
	TemporalScaleMode = new QSpinBox(h264_controls_grid);	
	TemporalScaleMode->show();
	grid_layout->addWidget(TemporalScaleMode, line, 1);
	
	TemporalScaleMode->setRange(min_tsmflags, max_tsmflags);
	TemporalScaleMode->setSingleStep(1);
	TemporalScaleMode->setValue(cur_tsmflags);
	
	//connect signal
	connect(TemporalScaleMode, SIGNAL(valueChanged(int)), 
		this, SLOT(h264_TemporalScaleMode_changed(int)));

	//bSpatialScaleMode
	line++;
	QLabel *label_SpatialScaleMode = new QLabel(_("Spatial Scale Mode:"),
		h264_controls_grid);
	label_SpatialScaleMode->show();
	grid_layout->addWidget(label_SpatialScaleMode, line, 0, Qt::AlignRight);
	
	uint8_t cur_ssmflags = h264_config_probe_req->bSpatialScaleMode & 0x0F;
	uint8_t max_ssmflags = config_probe_max.bSpatialScaleMode & 0x0F;
	uint8_t min_ssmflags = config_probe_min.bSpatialScaleMode & 0x0F;
	
	SpatialScaleMode = new QSpinBox(h264_controls_grid);	
	SpatialScaleMode->show();
	grid_layout->addWidget(SpatialScaleMode, line, 1);
	
	SpatialScaleMode->setRange(min_ssmflags, max_ssmflags);
	SpatialScaleMode->setSingleStep(1);
	SpatialScaleMode->setValue(cur_ssmflags);

    //dwFrameInterval
    if(is_control_panel) //if streaming use fps in video tab to set FrameInterval
    {
		line++;
		QLabel *label_FrameInterval = new QLabel(_("Frame Interval (100ns units):"),
			h264_controls_grid);
		label_FrameInterval->show();
		grid_layout->addWidget(label_FrameInterval, line, 0, Qt::AlignRight);

		//uint32_t cur_framerate = (global->fps_num * 1000000000LL / global->fps)/100;
		uint32_t cur_framerate = v4l2core_get_h264_frame_rate_config(get_v4l2_device_handler());
		uint32_t max_framerate = v4l2core_query_h264_frame_rate_config(get_v4l2_device_handler(), UVC_GET_MAX);
		uint32_t min_framerate = v4l2core_query_h264_frame_rate_config(get_v4l2_device_handler(), UVC_GET_MIN);

		FrameInterval = new QSpinBox(h264_controls_grid);	
		FrameInterval->show();
		grid_layout->addWidget(FrameInterval, line, 1);
	
		FrameInterval->setRange(min_framerate, max_framerate);
		FrameInterval->setSingleStep(1);
		FrameInterval->setValue(cur_framerate);
	
		//connect signal
		connect(TemporalScaleMode, SIGNAL(valueChanged(int)), 
			this, SLOT(h264_FrameInterval_changed(int)));
	}

	/*
	 * probe_commit specific controls
	 */

	if(is_control_panel)
		return 0; //don't create probe_commit specific controls if we are in control panel mode

	//dwBitRate
	line++;
	QLabel *label_BitRate =  new QLabel(_("Bit Rate:"),
		h264_controls_grid);
	label_BitRate->show();
	grid_layout->addWidget(label_BitRate, line, 0, Qt::AlignRight);
		
	BitRate = new QSpinBox(h264_controls_grid);	
	BitRate->show();
	grid_layout->addWidget(BitRate, line, 1);
	
	BitRate->setRange(config_probe_min.dwBitRate, config_probe_max.dwBitRate);
	BitRate->setSingleStep(1);
	BitRate->setValue(h264_config_probe_req->dwBitRate);

	//bmHints
	line++;
	
	QLabel *label_Hints = new QLabel(_("Hints:"),
		h264_controls_grid);
	label_Hints->show();
	grid_layout->addWidget(label_Hints, line, 0, 1, 2, Qt::AlignCenter);

	//Hints grid
	line++;
	
	QWidget* hints_table = new QWidget(h264_controls_grid);
	QGridLayout *hints_layout = new QGridLayout();
	hints_table->setLayout(hints_layout);
	hints_table->show();
	grid_layout->addWidget(hints_table, line, 0, 1, 2);

	//Hints Resolution
	Hints_res = new QCheckBox(_("Resolution"), hints_table);
	hints_layout->addWidget(Hints_res, 0, 0);
	Hints_res->setChecked((h264_config_probe_req->bmHints & 0x0001) > 0);
	//Hints Profile
	Hints_prof = new QCheckBox(_("Profile"), hints_table);
	hints_layout->addWidget(Hints_prof, 0, 1);
	Hints_prof->setChecked((h264_config_probe_req->bmHints & 0x0002) > 0);
	//Hints Ratecontrol
	Hints_ratecontrol = new QCheckBox(_("Rate Control"), hints_table);
	hints_layout->addWidget(Hints_ratecontrol, 0, 2);
	Hints_ratecontrol->setChecked((h264_config_probe_req->bmHints & 0x0004) > 0);
	//Hints Usage
	Hints_usage = new QCheckBox(_("Usage Type"), hints_table);
	hints_layout->addWidget(Hints_usage, 0, 3);
	Hints_usage->setChecked((h264_config_probe_req->bmHints & 0x0008) > 0);
	//Hints SliceMode
	Hints_slicemode = new QCheckBox(_("Slice Mode"), hints_table);
	hints_layout->addWidget(Hints_slicemode, 1, 0);
	Hints_slicemode->setChecked((h264_config_probe_req->bmHints & 0x0010) > 0);
	//Hints Sliceunit
	Hints_sliceunit = new QCheckBox(_("Slice Unit"), hints_table);
	hints_layout->addWidget(Hints_sliceunit, 1, 1);
	Hints_sliceunit->setChecked((h264_config_probe_req->bmHints & 0x0020) > 0);
	//Hints View
	Hints_view = new QCheckBox(_("MVC View"), hints_table);
	hints_layout->addWidget(Hints_view, 1, 2);
	Hints_view->setChecked((h264_config_probe_req->bmHints & 0x0040) > 0);
	//Hints View
	Hints_temporal = new QCheckBox(_("Temporal Scale"), hints_table);
	hints_layout->addWidget(Hints_temporal, 1, 3);
	Hints_temporal->setChecked((h264_config_probe_req->bmHints & 0x0080) > 0);
	//Hints View
	Hints_snr = new QCheckBox(_("SNR Scale"), hints_table);
	hints_layout->addWidget(Hints_snr, 2, 0);
	Hints_snr->setChecked((h264_config_probe_req->bmHints & 0x0100) > 0);
	//Hints Spatial
	Hints_spatial = new QCheckBox(_("Spatial Scale"), hints_table);
	hints_layout->addWidget(Hints_spatial, 2, 1);
	Hints_spatial->setChecked((h264_config_probe_req->bmHints & 0x0200) > 0);
	//Hints Spatial layer
	Hints_spatiallayer = new QCheckBox(_("Spatial Layer Ratio"), hints_table);
	hints_layout->addWidget(Hints_spatiallayer, 2, 2);
	Hints_spatiallayer->setChecked((h264_config_probe_req->bmHints & 0x0400) > 0);
	//Hints FrameInterval
	Hints_frameinterval = new QCheckBox(_("Frame Interval"), hints_table);
	hints_layout->addWidget(Hints_frameinterval, 2, 3);
	Hints_frameinterval->setChecked((h264_config_probe_req->bmHints & 0x0800) > 0);
	//Hints Leaky bucket
	Hints_leakybucket = new QCheckBox(_("Leaky Bucket Size"), hints_table);
	hints_layout->addWidget(Hints_leakybucket, 3, 0);
	Hints_leakybucket->setChecked((h264_config_probe_req->bmHints & 0x1000) > 0);
	//Hints Bit Rate
	Hints_bitrate = new QCheckBox(_("Bit Rate"), hints_table);
	hints_layout->addWidget(Hints_bitrate, 3, 1);
	Hints_bitrate->setChecked((h264_config_probe_req->bmHints & 0x2000) > 0);
	//Hints Cabac
	Hints_cabac = new QCheckBox(_("CABAC"), hints_table);
	hints_layout->addWidget(Hints_cabac, 3, 2);
	Hints_cabac->setChecked((h264_config_probe_req->bmHints & 0x4000) > 0);
	//Hints I Frame
	Hints_iframe = new QCheckBox(_("(I) Frame Period"), hints_table);
	hints_layout->addWidget(Hints_iframe, 3, 3);
	Hints_iframe->setChecked((h264_config_probe_req->bmHints & 0x8000) > 0);
	
	//wWidth x wHeight - use global values

	//wSliceMode
	line++;
	
	QLabel *label_SliceMode = new QLabel(_("Slice Mode:"),
		h264_controls_grid);
	label_SliceMode->show();
	grid_layout->addWidget(label_SliceMode, line, 0, Qt::AlignRight);

	SliceMode = new QComboBox(h264_controls_grid);
	SliceMode->show();
	grid_layout->addWidget(SliceMode, line, 1);

	SliceMode->addItem(_("no multiple slices"), 0);
	SliceMode->addItem(_("bits/slice"), 1);
	SliceMode->addItem(_("Mbits/slice"), 2);
	SliceMode->addItem(_("slices/frame"), 3);
	
	SliceMode->setCurrentIndex(h264_config_probe_req->wSliceMode); //0 indexed

	//wSliceUnits
	line++;

	QLabel *label_SliceUnits = new QLabel(_("Slice Units:"),
		h264_controls_grid);
	label_SliceUnits->show();
	grid_layout->addWidget(label_SliceUnits, line, 0, Qt::AlignRight);

	SliceUnits = new QSpinBox(h264_controls_grid);
	SliceUnits->show();
	grid_layout->addWidget(SliceUnits, line, 1);
	
	SliceUnits->setRange(config_probe_min.wSliceUnits, 
		config_probe_max.wSliceUnits);
	SliceUnits->setSingleStep(1);
	SliceUnits->setValue(h264_config_probe_req->wSliceUnits);

	//wProfile
	line++;

	QLabel *label_Profile = new QLabel(_("Profile:"),
		h264_controls_grid);
	label_Profile->show();
	grid_layout->addWidget(label_Profile, line, 0, Qt::AlignRight);

	Profile = new QComboBox(h264_controls_grid);
	Profile->show();
	grid_layout->addWidget(Profile, line, 1);

	Profile->addItem(_("Baseline Profile"), 0x4200);
	Profile->addItem(_("Main Profile"),  0x4D00);
	Profile->addItem(_("High Profile"), 0x6400);
	Profile->addItem(_("Scalable Baseline Profile"), 0x5300);
	Profile->addItem(_("Scalable High Profile"), 0x5600);
	Profile->addItem(_("Multiview High Profile"), 0x7600);
	Profile->addItem(_("Stereo High Profile"), 0x8000);

	uint16_t profile = h264_config_probe_req->wProfile & 0xFF00;
	int prof_index = 0;
	switch(profile)
	{
		case 0x4200:
			prof_index = 0;
			break;
		case 0x4D00:
			prof_index = 1;
			break;
		case 0x6400:
			prof_index = 2;
			break;
		case 0x5300:
			prof_index = 3;
			break;
		case 0x5600:
			prof_index = 4;
			break;
		case 0x7600:
			prof_index = 5;
			break;
		case 0x8000:
			prof_index = 6;
			break;
		default:
			std::cerr << "GUVCVIEW (Qt5): (H264 probe) unknown profile mode "
				<< std::hex << profile << std::dec << std::endl;
			break;

	}
	Profile->setCurrentIndex(prof_index);

	//wProfile (Bits 0-7)
	line++;

	QLabel *label_Profile_flags = new QLabel(_("Profile flags:"),
		h264_controls_grid);
	label_Profile_flags->show();
	grid_layout->addWidget(label_Profile_flags, line, 0, Qt::AlignRight);

	int cur_flags = h264_config_probe_req->wProfile & 0x000000FF;
	int max_flags = config_probe_max.wProfile & 0x000000FF;
	int min_flags = config_probe_min.wProfile & 0x000000FF;

	Profile_flags = new QSpinBox(h264_controls_grid);
	Profile_flags->show();
	grid_layout->addWidget(Profile_flags, line, 1);

	Profile_flags->setRange(min_flags, max_flags);
	Profile_flags->setSingleStep(1);
	Profile_flags->setValue(cur_flags);

	//wIFramePeriod
	line++;

	QLabel *label_IFramePeriod = new QLabel(_("(I) Frame Period (ms):"),
		h264_controls_grid);
	label_IFramePeriod->show();
	grid_layout->addWidget(label_IFramePeriod, line, 0, Qt::AlignRight);

	IFramePeriod = new QSpinBox(h264_controls_grid);
	IFramePeriod->show();
	grid_layout->addWidget(IFramePeriod, line, 1);

	IFramePeriod->setRange(config_probe_min.wIFramePeriod,
		config_probe_max.wIFramePeriod);
	IFramePeriod->setSingleStep(1);
	IFramePeriod->setValue(h264_config_probe_req->wIFramePeriod);

	//wEstimatedVideoDelay
	line++;

	QLabel *label_EstimatedVideoDelay = new QLabel(_("Estimated Video Delay (ms):"),
		h264_controls_grid);
	label_EstimatedVideoDelay->show();
	grid_layout->addWidget(label_EstimatedVideoDelay, line, 0, Qt::AlignRight);

	EstimatedVideoDelay = new QSpinBox(h264_controls_grid);
	EstimatedVideoDelay->show();
	grid_layout->addWidget(EstimatedVideoDelay, line, 1);

	EstimatedVideoDelay->setRange(config_probe_min.wEstimatedVideoDelay,
		config_probe_max.wEstimatedVideoDelay);
	EstimatedVideoDelay->setSingleStep(1);
	EstimatedVideoDelay->setValue(h264_config_probe_req->wEstimatedVideoDelay);

    //wEstimatedMaxConfigDelay
	line++;

	QLabel *label_EstimatedMaxConfigDelay = new QLabel(_("Estimated Max Config Delay (ms):"),
		h264_controls_grid);
	label_EstimatedMaxConfigDelay->show();
	grid_layout->addWidget(label_EstimatedMaxConfigDelay, line, 0, Qt::AlignRight);

	EstimatedMaxConfigDelay = new QSpinBox(h264_controls_grid);
	EstimatedMaxConfigDelay->show();
	grid_layout->addWidget(EstimatedMaxConfigDelay, line, 1);

	EstimatedMaxConfigDelay->setRange(config_probe_min.wEstimatedMaxConfigDelay,
		config_probe_max.wEstimatedMaxConfigDelay);
	EstimatedMaxConfigDelay->setSingleStep(1);
	EstimatedMaxConfigDelay->setValue(h264_config_probe_req->wEstimatedMaxConfigDelay);

	//bUsageType
	line++;

	QLabel *label_UsageType = new QLabel(_("Usage Type:"),
		h264_controls_grid);
	label_UsageType->show();
	grid_layout->addWidget(label_UsageType, line, 0, Qt::AlignRight);

	UsageType = new QComboBox(h264_controls_grid);
	UsageType->show();
	grid_layout->addWidget(UsageType, line, 1);

	UsageType->addItem(_("Real-time"), 1);
	UsageType->addItem(_("Broadcast"), 2);
	UsageType->addItem(_("Storage"), 3);
	UsageType->addItem(_("(0) Non-scalable single layer AVC"), 4);
	UsageType->addItem(_("(1) SVC temporal scalability with hierarchical P"), 5);
	UsageType->addItem(_("(2q) SVC temporal scalability + Quality/SNR scalability"), 6);
	UsageType->addItem(_("(2s) SVC temporal scalability + spatial scalability"), 7);
	UsageType->addItem(_("(3) Full SVC scalability"), 8);

	uint8_t usage = h264_config_probe_req->bUsageType & 0x0F;
	int usage_index = usage - 1; // from 0x01 to 0x0F
	if(usage_index < 0)
		usage_index = 0;
	else if(usage_index > 7)
		usage_index = 7;

	UsageType->setCurrentIndex(usage_index);

	//bSNRScaleMode
	line++;
	
	QLabel *label_SNRScaleMode = new QLabel(_("SNR Control Mode:"),
		h264_controls_grid);
	label_SNRScaleMode->show();
	grid_layout->addWidget(label_SNRScaleMode, line, 0, Qt::AlignRight);

	SNRScaleMode = new QComboBox(h264_controls_grid);
	SNRScaleMode->show();
	grid_layout->addWidget(SNRScaleMode, line, 1);

	SNRScaleMode->addItem(_("No SNR Enhancement Layer"), 0);
	SNRScaleMode->addItem(_("CGS NonRewrite (2 Layer)"), 2);
	SNRScaleMode->addItem(_("CGS NonRewrite (3 Layer)"), 3);
	SNRScaleMode->addItem(_("CGS Rewrite (2 Layer)"), 4);
	SNRScaleMode->addItem(_("CGS Rewrite (3 Layer)"), 5);
	SNRScaleMode->addItem(_("MGS (2 Layer)"), 6);

	uint8_t snrscalemode = h264_config_probe_req->bSNRScaleMode & 0x0F;
	int snrscalemode_index = 0;
	switch(snrscalemode)
	{
		case 0:
			snrscalemode_index = 0;
			break;
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
			snrscalemode_index = snrscalemode - 1;
			break;
	}
	SNRScaleMode->setCurrentIndex(snrscalemode_index);

	//bStreamMuxOption (done directly in the core)
	line++;
	
	QWidget* mux_table = new QWidget(h264_controls_grid);
	QGridLayout *mux_layout = new QGridLayout();
	mux_table->setLayout(mux_layout);
	mux_table->show();
	grid_layout->addWidget(mux_table, line, 0, 1, 2);

	//---enable
	StreamMuxOption = new QCheckBox(_("Stream Mux Enable"), mux_table);
	StreamMuxOption->show();
	mux_layout->addWidget(StreamMuxOption, 0, 0);
	StreamMuxOption->setChecked((h264_config_probe_req->bStreamMuxOption & 0x01) != 0);
	
	//---aux
	StreamMuxOption_aux = new QComboBox(mux_table);
	StreamMuxOption_aux->show();
	mux_layout->addWidget(StreamMuxOption_aux, 0, 1);
	StreamMuxOption_aux->addItem(_("Embed H.264 aux stream"), 2);
	StreamMuxOption_aux->addItem(_("Embed YUY2 aux stream"), 4);
	StreamMuxOption_aux->addItem(_("Embed NV12 aux stream"), 8);

	uint8_t streammux = h264_config_probe_req->bStreamMuxOption & 0x0E;
	int streammux_index = 0;
	switch(streammux)
	{
		case 2:
			streammux_index = 0;
			break;
		case 4:
			streammux_index = 1;
			break;
		case 8:
			streammux_index = 2;
			break;
	}
	StreamMuxOption_aux->setCurrentIndex(streammux_index);
	//---mjpg payload
	StreamMuxOption_mjpgcontainer = new QCheckBox(_("MJPG payload container"), mux_table);
	StreamMuxOption_mjpgcontainer->show();
	mux_layout->addWidget(StreamMuxOption_mjpgcontainer, 0, 2);
	StreamMuxOption_mjpgcontainer->setChecked((h264_config_probe_req->bStreamMuxOption & 0x40) != 0);
	//No support yet for multiple streams (set in the core)
	StreamMuxOption->setDisabled(true);
	StreamMuxOption_aux->setDisabled(true);
	StreamMuxOption_mjpgcontainer->setDisabled(true);

	//bStreamFormat
	line++;
	
	QLabel *label_StreamFormat = new QLabel(_("Stream Format:"),
		h264_controls_grid);
	label_StreamFormat->show();
	grid_layout->addWidget(label_StreamFormat, line, 0, Qt::AlignRight);

	StreamFormat = new QComboBox(h264_controls_grid);
	StreamFormat->show();
	grid_layout->addWidget(StreamFormat, line, 1);

	StreamFormat->addItem(_("Byte stream format (H.264 Annex-B)"), 0);
	StreamFormat->addItem(_("NAL stream format"), 1);

	int streamformat_index = (int) (h264_config_probe_req->bStreamFormat & 0x01);
	StreamFormat->setCurrentIndex(streamformat_index);

	//bEntropyCABAC
	line++;
	
	QLabel *label_EntropyCABAC = new QLabel(_("Entropy CABAC:"),
		h264_controls_grid);
	label_EntropyCABAC->show();
	grid_layout->addWidget(label_EntropyCABAC, line, 0, Qt::AlignRight);

	EntropyCABAC = new QComboBox(h264_controls_grid);
	EntropyCABAC->show();
	grid_layout->addWidget(EntropyCABAC, line, 1);

	EntropyCABAC->addItem(_("CAVLC"), 0);
	EntropyCABAC->addItem(_("CABAC"), 1);

	int entropycabac_index = (int) (h264_config_probe_req->bEntropyCABAC & 0x01);
	EntropyCABAC->setCurrentIndex(entropycabac_index);

	//bTimestamp
	line++;
	
	Timestamp = new QCheckBox(_("Picture timing SEI"), h264_controls_grid);
	Timestamp->show();
	grid_layout->addWidget(Timestamp, line, 1);
	Timestamp->setChecked((h264_config_probe_req->bTimestamp & 0x01) > 0);

	//bNumOfReorderFrames
	line++;
	
	QLabel *label_NumOfReorderFrames = new QLabel(_("B Frames:"),
		h264_controls_grid);
	label_NumOfReorderFrames->show();
	grid_layout->addWidget(label_NumOfReorderFrames, line, 0, Qt::AlignRight);

	NumOfReorderFrames = new QSpinBox(h264_controls_grid);
	NumOfReorderFrames->show();
	grid_layout->addWidget(NumOfReorderFrames, line, 1);

	cur_flags = h264_config_probe_req->bNumOfReorderFrames & 0x000000FF;
	max_flags = config_probe_max.bNumOfReorderFrames & 0x000000FF;
	min_flags = config_probe_min.bNumOfReorderFrames & 0x000000FF;
	
	NumOfReorderFrames->setRange(min_flags, max_flags);
	NumOfReorderFrames->setSingleStep(1);
	NumOfReorderFrames->setValue(cur_flags);

	//bPreviewFlipped
	line++;
	
	PreviewFlipped = new QCheckBox(_("Preview Flipped"), h264_controls_grid);
	PreviewFlipped->show();
	grid_layout->addWidget(PreviewFlipped, line, 1);
	PreviewFlipped->setChecked((h264_config_probe_req->bPreviewFlipped & 0x01) > 0);

	//bView
	line++;
	
	QLabel *label_View = new QLabel(_("Additional MVC Views:"),
		h264_controls_grid);
	label_View->show();
	grid_layout->addWidget(label_View, line, 0, Qt::AlignRight);

	View = new QSpinBox(h264_controls_grid);
	View->show();
	grid_layout->addWidget(View, line, 1);

	cur_flags = h264_config_probe_req->bView & 0x000000FF;
	max_flags = config_probe_max.bView & 0x000000FF;
	min_flags = config_probe_min.bView & 0x000000FF;
	
	View->setRange(min_flags, max_flags);
	View->setSingleStep(1);
	View->setValue(cur_flags);

    //bStreamID
	line++;
	
	QLabel *label_StreamID = new QLabel(_("Simulcast stream index:"),
		h264_controls_grid);
	label_StreamID->show();
	grid_layout->addWidget(label_StreamID, line, 0, Qt::AlignRight);

	StreamID = new QSpinBox(h264_controls_grid);
	StreamID->show();
	grid_layout->addWidget(StreamID, line, 1);

	cur_flags = h264_config_probe_req->bStreamID & 0x000000FF;
	max_flags = config_probe_max.bStreamID & 0x000000FF;
	min_flags = config_probe_min.bStreamID & 0x000000FF;
	
	StreamID->setRange(min_flags, max_flags);
	StreamID->setSingleStep(1);
	StreamID->setValue(cur_flags);

    //bSpatialLayerRatio
	line++;
	
	QLabel *label_SpatialLayerRatio = new QLabel(_("Spatial Layer Ratio:"),
		h264_controls_grid);
	label_SpatialLayerRatio->show();
	grid_layout->addWidget(label_SpatialLayerRatio, line, 0, Qt::AlignRight);

	SpatialLayerRatio = new QDoubleSpinBox(h264_controls_grid);
	SpatialLayerRatio->show();
	grid_layout->addWidget(SpatialLayerRatio, line, 1);

	cur_flags = h264_config_probe_req->bSpatialLayerRatio & 0x000000FF;
	max_flags = config_probe_max.bSpatialLayerRatio & 0x000000FF;
	min_flags = config_probe_min.bSpatialLayerRatio & 0x000000FF;

	double cur = (double) ((cur_flags & 0x000000F0)>>4) + (double)((cur_flags & 0x0000000F)/16);
	double min = (double) ((min_flags & 0x000000F0)>>4) + (double)((min_flags & 0x0000000F)/16);
	double max = (double) ((max_flags & 0x000000F0)>>4) + (double)((max_flags & 0x0000000F)/16);

	SpatialLayerRatio->setRange(min, max);
	SpatialLayerRatio->setSingleStep(0.1);
	SpatialLayerRatio->setValue(cur);

	//wLeakyBucketSize
	line++;

	QLabel *label_LeakyBucketSize = new QLabel(_("Leaky Bucket Size (ms):"),
		h264_controls_grid);
	label_LeakyBucketSize->show();
	grid_layout->addWidget(label_LeakyBucketSize, line, 0, Qt::AlignRight);

	LeakyBucketSize = new QSpinBox(h264_controls_grid);
	LeakyBucketSize->show();
	grid_layout->addWidget(LeakyBucketSize, line, 1);

	cur_flags = h264_config_probe_req->wLeakyBucketSize;
	max_flags = config_probe_max.wLeakyBucketSize;
	min_flags = config_probe_min.wLeakyBucketSize;

	LeakyBucketSize->setRange(min_flags, max_flags);
	LeakyBucketSize->setSingleStep(1);
	LeakyBucketSize->setValue(cur_flags);

	//PROBE COMMIT buttons
	line++;

	//encoder reset
	QPushButton *reset_button = new QPushButton(_("Encoder Reset"), 
		h264_controls_grid);
	reset_button->show();
	grid_layout->addWidget(reset_button, line, 0, Qt::AlignRight);
	
	/*signals*/
	connect(reset_button, SIGNAL(clicked()), this, SLOT(h264_reset_button_clicked()));

	//commit button
	QPushButton *commit_button = new QPushButton(_("COMMIT"), 
		h264_controls_grid);
	commit_button->show();
	grid_layout->addWidget(commit_button, line, 1, Qt::AlignLeft);

	/*signals*/
	connect(commit_button, SIGNAL(clicked()), this, SLOT(h264_commit_button_clicked()));

	return 0;
}
