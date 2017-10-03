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
#include <math.h>
/* support for internationalization - i18n */
#include <locale.h>
#include <libintl.h>

#include "gui_gtk3.h"
#include "gui_gtk3_callbacks.h"
#include "gui.h"
/*add this last to avoid redefining _() and N_()*/
#include "gview.h"
#include "gviewrender.h"
#include "video_capture.h"


extern int debug_level;
extern int is_control_panel;

/*
 * H264 control widgets
 */
GtkWidget *RateControlMode = NULL;
GtkWidget *RateControlMode_cbr_flag = NULL;
GtkWidget *TemporalScaleMode = NULL;
GtkWidget *SpatialScaleMode = NULL;
GtkWidget *FrameInterval = NULL;
GtkWidget *BitRate = NULL;
GtkWidget *Hints_res = NULL;
GtkWidget *Hints_prof = NULL;
GtkWidget *Hints_ratecontrol = NULL;
GtkWidget *Hints_usage = NULL;
GtkWidget *Hints_slicemode = NULL;
GtkWidget *Hints_sliceunit = NULL;
GtkWidget *Hints_view = NULL;
GtkWidget *Hints_temporal = NULL;
GtkWidget *Hints_snr = NULL;
GtkWidget *Hints_spatial = NULL;
GtkWidget *Hints_spatiallayer = NULL;
GtkWidget *Hints_frameinterval = NULL;
GtkWidget *Hints_leakybucket = NULL;
GtkWidget *Hints_bitrate = NULL;
GtkWidget *Hints_cabac = NULL;
GtkWidget *Hints_iframe = NULL;
GtkWidget *SliceMode = NULL;
GtkWidget *SliceUnits = NULL;
GtkWidget *Profile = NULL;
GtkWidget *Profile_flags = NULL;
GtkWidget *IFramePeriod = NULL;
GtkWidget *EstimatedVideoDelay = NULL;
GtkWidget *EstimatedMaxConfigDelay = NULL;
GtkWidget *UsageType = NULL;
GtkWidget *SNRScaleMode = NULL;
GtkWidget *StreamFormat = NULL;
GtkWidget *EntropyCABAC = NULL;
GtkWidget *Timestamp = NULL;
GtkWidget *NumOfReorderFrames = NULL;
GtkWidget *PreviewFlipped = NULL;
GtkWidget *View = NULL;
GtkWidget *StreamID = NULL;
GtkWidget *SpatialLayerRatio = NULL;
GtkWidget *LeakyBucketSize = NULL;

/*disabled*/
GtkWidget *StreamMuxOption = NULL;
GtkWidget *StreamMuxOption_aux = NULL;
GtkWidget *StreamMuxOption_mjpgcontainer = NULL;

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
static void update_h264_controls()
{
	uvcx_video_config_probe_commit_t *config_probe_req = v4l2core_get_h264_config_probe_req(get_v4l2_device_handler());
	//dwFrameInterval
	//gtk_spin_button_set_value(GTK_SPIN_BUTTON(h264_controls->FrameInterval), config_probe_req->dwFrameInterval);
	//dwBitRate
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(BitRate), config_probe_req->dwBitRate);
	//bmHints
	uint16_t hints = config_probe_req->bmHints;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(Hints_res), ((hints & 0x0001) != 0));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(Hints_prof), ((hints & 0x0002) != 0));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(Hints_ratecontrol), ((hints & 0x0004) != 0));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(Hints_usage), ((hints & 0x0008) != 0));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(Hints_slicemode), ((hints & 0x0010) != 0));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(Hints_sliceunit), ((hints & 0x0020) != 0));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(Hints_view), ((hints & 0x0040) != 0));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(Hints_temporal), ((hints & 0x0080) != 0));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(Hints_snr), ((hints & 0x0100) != 0));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(Hints_spatial), ((hints & 0x0200) != 0));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(Hints_spatiallayer), ((hints & 0x0400) != 0));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(Hints_frameinterval), ((hints & 0x0800) != 0));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(Hints_leakybucket), ((hints & 0x1000) != 0));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(Hints_bitrate), ((hints & 0x2000) != 0));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(Hints_cabac), ((hints & 0x4000) != 0));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(Hints_iframe), ((hints & 0x8000) != 0));
	//wWidth x wHeight

	//wSliceMode
	gtk_combo_box_set_active(GTK_COMBO_BOX(SliceMode), config_probe_req->wSliceMode);
	//wSliceUnits
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(SliceUnits), config_probe_req->wSliceUnits);
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
	gtk_combo_box_set_active(GTK_COMBO_BOX(Profile), prof_index);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(Profile_flags), config_probe_req->wProfile & 0x00FF);
	//wIFramePeriod
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(IFramePeriod), config_probe_req->wIFramePeriod);
	//wEstimatedVideoDelay
	 gtk_spin_button_set_value(GTK_SPIN_BUTTON(EstimatedVideoDelay), config_probe_req->wEstimatedVideoDelay);
	//wEstimatedMaxConfigDelay
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(EstimatedMaxConfigDelay), config_probe_req->wEstimatedMaxConfigDelay);
	//bUsageType
	int usage_type_ind = (config_probe_req->bUsageType & 0x0F) - 1;
	if(usage_type_ind < 0)
		usage_type_ind = 0;
	gtk_combo_box_set_active(GTK_COMBO_BOX(UsageType), usage_type_ind);
	//bRateControlMode
	gtk_combo_box_set_active(GTK_COMBO_BOX(RateControlMode), (config_probe_req->bRateControlMode & 0x03) - 1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(RateControlMode_cbr_flag), config_probe_req->bRateControlMode & 0x0000001C);
	//bTemporalScaleMode
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(TemporalScaleMode), config_probe_req->bTemporalScaleMode);
	//bSpatialScaleMode
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(SpatialScaleMode), config_probe_req->bSpatialScaleMode);
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
	gtk_combo_box_set_active(GTK_COMBO_BOX(SNRScaleMode), snrscalemode_index);
	//bStreamMuxOption
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(StreamMuxOption),((config_probe_req->bStreamMuxOption & 0x01) != 0));
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
	gtk_combo_box_set_active (GTK_COMBO_BOX(StreamMuxOption_aux), streammux_index);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(StreamMuxOption_mjpgcontainer),((config_probe_req->bStreamMuxOption & 0x40) != 0));
	//bStreamFormat
	gtk_combo_box_set_active (GTK_COMBO_BOX(StreamFormat), config_probe_req->bStreamMuxOption & 0x01);
	//bEntropyCABAC
	gtk_combo_box_set_active (GTK_COMBO_BOX(EntropyCABAC), config_probe_req->bEntropyCABAC & 0x01);
	//bTimestamp
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(Timestamp), (config_probe_req->bTimestamp & 0x01) != 0);
	//bNumOfReorderFrames
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(NumOfReorderFrames), config_probe_req->bNumOfReorderFrames);
	//bPreviewFlipped
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(PreviewFlipped), (config_probe_req->bPreviewFlipped & 0x01) != 0);
	//bView
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(View), config_probe_req->bView);
	//bStreamID
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(StreamID), config_probe_req->bStreamID);
	//bSpatialLayerRatio
	int SLRatio = config_probe_req->bSpatialLayerRatio & 0x000000FF;
	gdouble val = (gdouble) ((SLRatio & 0x000000F0)>>4) + (gdouble)((SLRatio & 0x0000000F)/16);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(SpatialLayerRatio), val);
	//wLeakyBucketSize
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(LeakyBucketSize), config_probe_req->wLeakyBucketSize);
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
static void fill_video_config_probe ()
{

	uvcx_video_config_probe_commit_t *config_probe_req = v4l2core_get_h264_config_probe_req(get_v4l2_device_handler());

	//dwFrameInterval
	uint32_t frame_interval = (v4l2core_get_fps_num(get_v4l2_device_handler()) * 1000000000LL / v4l2core_get_fps_denom(get_v4l2_device_handler()))/100;
	config_probe_req->dwFrameInterval = frame_interval;//(uint32_t) gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(h264_controls->FrameInterval));
	//dwBitRate
	config_probe_req->dwBitRate = (uint32_t) gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(BitRate));
	//bmHints
	uint16_t hints = 0x0000;
	hints |= gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(Hints_res)) ?  0x0001 :  0;
	hints |= gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(Hints_prof)) ? 0x0002 : 0;
	hints |= gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(Hints_ratecontrol)) ? 0x0004: 0;
	hints |= gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(Hints_usage)) ? 0x0008: 0;
	hints |= gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(Hints_slicemode)) ? 0x0010: 0;
	hints |= gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(Hints_sliceunit)) ? 0x0020: 0;
	hints |= gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(Hints_view)) ? 0x0040: 0;
	hints |= gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(Hints_temporal)) ? 0x0080: 0;
	hints |= gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(Hints_snr)) ? 0x0100: 0;
	hints |= gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(Hints_spatial)) ? 0x0200: 0;
	hints |= gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(Hints_spatiallayer)) ? 0x0400: 0;
	hints |= gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(Hints_frameinterval)) ? 0x0800: 0;
	hints |= gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(Hints_leakybucket)) ? 0x1000: 0;
	hints |= gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(Hints_bitrate)) ? 0x2000: 0;
	hints |= gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(Hints_cabac)) ? 0x4000: 0;
	hints |= gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(Hints_iframe)) ? 0x8000: 0;
	config_probe_req->bmHints = hints;
	//wWidth x wHeight
	config_probe_req->wWidth = (uint16_t) v4l2core_get_frame_width(get_v4l2_device_handler());
	config_probe_req->wHeight = (uint16_t) v4l2core_get_frame_height(get_v4l2_device_handler());
	//wSliceMode
	config_probe_req->wSliceMode = (uint16_t) gtk_combo_box_get_active(GTK_COMBO_BOX(SliceMode));
	//wSliceUnits
	config_probe_req->wSliceUnits = (uint16_t) gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(SliceUnits));
	//wProfile
	int profile_index = gtk_combo_box_get_active(GTK_COMBO_BOX(Profile));
	uint16_t profile = 0x4200;
	switch(profile_index)
	{
		case 0:
			profile = 0x4200;
			break;
		case 1:
			profile = 0x4D00;
			break;
		case 2:
			profile = 0x6400;
			break;
		case 3:
			profile = 0x5300;
			break;
		case 4:
			profile = 0x5600;
			break;
		case 5:
			profile = 0x7600;
			break;
		case 6:
			profile = 0x8000;
			break;
		default:
			fprintf(stderr, "GUVCVIEW: (H264 probe) unknown profile\n");
			break;
	}

	profile |= (uint16_t) (gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(Profile_flags)) & 0x000000FF);
	config_probe_req->wProfile = profile;
	//wIFramePeriod
	config_probe_req->wIFramePeriod = (uint16_t) gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(IFramePeriod));
	//wEstimatedVideoDelay
	config_probe_req->wEstimatedVideoDelay = (uint16_t) gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(EstimatedVideoDelay));
	//wEstimatedMaxConfigDelay
	config_probe_req->wEstimatedMaxConfigDelay = (uint16_t) gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(EstimatedMaxConfigDelay));
	//bUsageType
	config_probe_req->bUsageType = (uint8_t) (gtk_combo_box_get_active(GTK_COMBO_BOX(UsageType)) + 1);
	//bRateControlMode
	config_probe_req->bRateControlMode = (uint8_t) (gtk_combo_box_get_active(GTK_COMBO_BOX(RateControlMode)) + 1);
	config_probe_req->bRateControlMode |= (uint8_t) (gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(RateControlMode_cbr_flag)) & 0x0000001C);
	//bTemporalScaleMode
	config_probe_req->bTemporalScaleMode = (uint8_t) gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(TemporalScaleMode));
	//bSpatialScaleMode
	config_probe_req->bSpatialScaleMode = (uint8_t) gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(SpatialScaleMode));
	//bSNRScaleMode
	int snrscalemode_index = gtk_combo_box_get_active(GTK_COMBO_BOX(SNRScaleMode));
	config_probe_req->bSNRScaleMode = 0;
	switch(snrscalemode_index)
	{
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			config_probe_req->bSNRScaleMode = snrscalemode_index + 1;
			break;
	}
	//bStreamMuxOption
	/*
	uint8_t streammux = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(StreamMuxOption)) ? 0x01: 0;

	int streammux_index = gtk_combo_box_get_active(GTK_COMBO_BOX(StreamMuxOption_aux));
	switch(streammux_index)
	{
		case 0:
			streammux |= 0x02;
			break;
		case 1:
			streammux |= 0x04;
			break;
		case 2:
			streammux |= 0x08;
			break;
	}
	streammux |= gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(StreamMuxOption_mjpgcontainer)) ? 0x40 : 0x00;

	config_probe_req->bStreamMuxOption = streammux;
	//bStreamFormat
	config_probe_req->bStreamMuxOption = (uint8_t) gtk_combo_box_get_active(GTK_COMBO_BOX(StreamFormat)) & 0x01;
	*/

	//bEntropyCABAC
	config_probe_req->bEntropyCABAC = (uint8_t) gtk_combo_box_get_active(GTK_COMBO_BOX(EntropyCABAC)) & 0x01;
	//bTimestamp
	config_probe_req->bTimestamp = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(Timestamp)) ? 0x01 : 0x00;
	//bNumOfReorderFrames
	config_probe_req->bNumOfReorderFrames = (uint8_t) gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(NumOfReorderFrames));
	//bPreviewFlipped
	config_probe_req->bPreviewFlipped = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(PreviewFlipped)) ? 0x01 : 0x00;
	//bView
	config_probe_req->bView = (uint8_t) gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(View));
	//bStreamID
	config_probe_req->bStreamID = (uint8_t) gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(StreamID));
	//bSpatialLayerRatio
	gdouble spatialratio = gtk_spin_button_get_value(GTK_SPIN_BUTTON(SpatialLayerRatio));
	uint8_t high_nibble = floor(spatialratio);
	uint8_t lower_nibble = floor((spatialratio - high_nibble) * 16);
	config_probe_req->bSpatialLayerRatio = (high_nibble << 4) + lower_nibble;
	//wLeakyBucketSize
	config_probe_req->wLeakyBucketSize = (uint16_t) gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(LeakyBucketSize));
}
/*
 * h264 video rate control mode callback
 * args:
 *   combo - widget that caused the event
 *   data  - user data
 *
 * asserts:
 *   none
 *
 * returns: none
 */
void h264_rate_control_mode_changed(GtkComboBox *combo, void *data)
{
	uint8_t rate_mode = (uint8_t) (gtk_combo_box_get_active (combo) + 1);

	rate_mode |= (uint8_t) (gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(RateControlMode_cbr_flag)) & 0x0000001C);

	v4l2core_set_h264_video_rate_control_mode(get_v4l2_device_handler(), rate_mode);

	rate_mode = v4l2core_get_h264_video_rate_control_mode(get_v4l2_device_handler(), UVC_GET_CUR);

	int ratecontrolmode_index = rate_mode - 1; // from 0x01 to 0x03
	if(ratecontrolmode_index < 0)
		ratecontrolmode_index = 0;

	g_signal_handlers_block_by_func(combo, G_CALLBACK (h264_rate_control_mode_changed), data);
	gtk_combo_box_set_active(GTK_COMBO_BOX(RateControlMode), ratecontrolmode_index);
	g_signal_handlers_unblock_by_func(combo, G_CALLBACK (h264_rate_control_mode_changed), data);
}

/*
 * h264 temporal scale mode callback
 * args:
 *   spin - widget that caused the event
 *   data  - user data
 *
 * asserts:
 *   none
 *
 * returns: none
 */
void h264_TemporalScaleMode_changed(GtkSpinButton *spin, void *data)
{
	uint8_t scale_mode = (uint8_t) gtk_spin_button_get_value_as_int(spin);

	v4l2core_set_h264_temporal_scale_mode(get_v4l2_device_handler(), scale_mode);

	scale_mode = v4l2core_get_h264_temporal_scale_mode(get_v4l2_device_handler(), UVC_GET_CUR) & 0x07;

	g_signal_handlers_block_by_func (spin, G_CALLBACK (h264_TemporalScaleMode_changed), data);
	gtk_spin_button_set_value (spin, scale_mode);
	g_signal_handlers_unblock_by_func (spin, G_CALLBACK (h264_TemporalScaleMode_changed), data);
}

/*
 * h264 spatial scale mode callback
 * args:
 *   spin - widget that caused the event
 *   data  - user data
 *
 * asserts:
 *   none
 *
 * returns: none
 */
void h264_SpatialScaleMode_changed(GtkSpinButton *spin, void *data)
{
	uint8_t scale_mode = (uint8_t) gtk_spin_button_get_value_as_int(spin);

	v4l2core_set_h264_spatial_scale_mode(get_v4l2_device_handler(), scale_mode);

	scale_mode = v4l2core_get_h264_spatial_scale_mode(get_v4l2_device_handler(), UVC_GET_CUR) & 0x07;

	g_signal_handlers_block_by_func (spin, G_CALLBACK (h264_SpatialScaleMode_changed), data);
	gtk_spin_button_set_value (spin, scale_mode);
	g_signal_handlers_unblock_by_func (spin, G_CALLBACK (h264_SpatialScaleMode_changed), data);
}

/*
 * h264 Frame Interval callback
 * args:
 *   spin - widget that caused the event
 *   data  - user data
 *
 * asserts:
 *   none
 *
 * returns: none
 */
void h264_FrameInterval_changed(GtkSpinButton *spin, void *data)
{
	uint32_t framerate = (uint32_t) gtk_spin_button_get_value_as_int(spin);

	v4l2core_set_h264_frame_rate_config(get_v4l2_device_handler(), framerate);

	framerate = v4l2core_get_h264_frame_rate_config(get_v4l2_device_handler());

	g_signal_handlers_block_by_func (spin, G_CALLBACK (h264_FrameInterval_changed), data);
	gtk_spin_button_set_value (spin, framerate);
	g_signal_handlers_unblock_by_func (spin, G_CALLBACK (h264_FrameInterval_changed), data);
}

/*
 * h264 commit button callback
 * args:
 *   button - widget that caused the event
 *   data  - user data
 *
 * asserts:
 *   none
 *
 * returns: none
 */
void h264_commit_button_clicked(GtkButton *button, void *data)
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
 *   button - widget that caused the event
 *   data  - user data
 *
 * asserts:
 *   none
 *
 * returns: none
 */
void h264_reset_button_clicked(GtkButton *button, void *data)
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
int gui_attach_gtk3_h264ctrls (GtkWidget *parent)
{
	/*assertions*/
	assert(parent != NULL);

	if(debug_level > 1)
		printf("GUVCVIEW (Gtk3): attaching H264 controls\n");


	//get current values
	v4l2core_probe_h264_config_probe_req(get_v4l2_device_handler(), UVC_GET_CUR, NULL);

	//get Max values
	uvcx_video_config_probe_commit_t config_probe_max;
	v4l2core_probe_h264_config_probe_req(get_v4l2_device_handler(), UVC_GET_MAX, &config_probe_max);

	//get Min values
	uvcx_video_config_probe_commit_t config_probe_min;
	v4l2core_probe_h264_config_probe_req(get_v4l2_device_handler(), UVC_GET_MIN, &config_probe_min);


	GtkWidget *h264_controls_grid = gtk_grid_new();
	gtk_widget_show (h264_controls_grid);

	gtk_grid_set_column_homogeneous (GTK_GRID(h264_controls_grid), FALSE);
	gtk_widget_set_hexpand (h264_controls_grid, TRUE);
	gtk_widget_set_halign (h264_controls_grid, GTK_ALIGN_FILL);

	gtk_grid_set_row_spacing (GTK_GRID(h264_controls_grid), 4);
	gtk_grid_set_column_spacing (GTK_GRID (h264_controls_grid), 4);
	gtk_container_set_border_width (GTK_CONTAINER (h264_controls_grid), 2);


	int line = 0;

	//streaming controls

	//bRateControlMode
	line++;
	GtkWidget* label_RateControlMode = gtk_label_new(_("Rate Control Mode:"));
#if GTK_VER_AT_LEAST(3,15)
	gtk_label_set_xalign(GTK_LABEL(label_RateControlMode), 1);
	gtk_label_set_yalign(GTK_LABEL(label_RateControlMode), 0.5);
#else
	gtk_misc_set_alignment (GTK_MISC (label_RateControlMode), 1, 0.5);
#endif
	gtk_grid_attach (GTK_GRID(h264_controls_grid), label_RateControlMode, 0, line, 1, 1);
	gtk_widget_show (label_RateControlMode);

	uint8_t min_ratecontrolmode = v4l2core_get_h264_video_rate_control_mode(get_v4l2_device_handler(), UVC_GET_MIN) & 0x03;
	uint8_t max_ratecontrolmode = v4l2core_get_h264_video_rate_control_mode(get_v4l2_device_handler(), UVC_GET_MAX) & 0x03;

	RateControlMode = gtk_combo_box_text_new();
	if(max_ratecontrolmode >= 1 && min_ratecontrolmode < 2)
		gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(RateControlMode),
										_("CBR"));
	if(max_ratecontrolmode >= 2 && min_ratecontrolmode < 3)
		gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(RateControlMode),
										_("VBR"));

	if(max_ratecontrolmode >= 3 && min_ratecontrolmode < 4)
		gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(RateControlMode),
										_("Constant QP"));

	uint8_t cur_ratecontrolmode = v4l2core_get_h264_video_rate_control_mode(get_v4l2_device_handler(), UVC_GET_CUR) & 0x03;
	int ratecontrolmode_index = cur_ratecontrolmode - 1; // from 0x01 to 0x03
	if(ratecontrolmode_index < 0)
		ratecontrolmode_index = 0;

	gtk_combo_box_set_active(GTK_COMBO_BOX(RateControlMode), ratecontrolmode_index);

	//connect signal
	g_signal_connect (GTK_COMBO_BOX_TEXT(RateControlMode), "changed",
			G_CALLBACK (h264_rate_control_mode_changed), NULL);

	gtk_grid_attach (GTK_GRID(h264_controls_grid), RateControlMode, 1, line, 1 ,1);
	gtk_widget_show (RateControlMode);

	//bRateControlMode flags (Bits 4-8)
	line++;
	GtkWidget* label_RateControlMode_cbr_flag = gtk_label_new(_("Rate Control Mode flags:"));
#if GTK_VER_AT_LEAST(3,15)
	gtk_label_set_xalign(GTK_LABEL(label_RateControlMode_cbr_flag), 1);
	gtk_label_set_yalign(GTK_LABEL(label_RateControlMode_cbr_flag), 0.5);
#else
	gtk_misc_set_alignment (GTK_MISC (label_RateControlMode_cbr_flag), 1, 0.5);
#endif
	gtk_grid_attach (GTK_GRID(h264_controls_grid), label_RateControlMode_cbr_flag, 0, line, 1, 1);
	gtk_widget_show (label_RateControlMode_cbr_flag);

	uint8_t cur_vrcflags = v4l2core_get_h264_video_rate_control_mode(get_v4l2_device_handler(), UVC_GET_CUR) & 0x1C;
	uint8_t max_vrcflags = v4l2core_get_h264_video_rate_control_mode(get_v4l2_device_handler(), UVC_GET_MAX) & 0x1C;
	uint8_t min_vrcflags = v4l2core_get_h264_video_rate_control_mode(get_v4l2_device_handler(), UVC_GET_MIN) & 0x1C;

	GtkAdjustment *adjustment7 = gtk_adjustment_new (
                                	cur_vrcflags,
                                	min_vrcflags,
                                    max_vrcflags,
                                    1,
                                    10,
                                    0);

    RateControlMode_cbr_flag = gtk_spin_button_new(adjustment7, 1, 0);
    gtk_editable_set_editable(GTK_EDITABLE(RateControlMode_cbr_flag), TRUE);

    gtk_grid_attach (GTK_GRID(h264_controls_grid), RateControlMode_cbr_flag, 1, line, 1 ,1);
    gtk_widget_show (RateControlMode_cbr_flag);

	//bTemporalScaleMode
	line++;
	GtkWidget* label_TemporalScaleMode = gtk_label_new(_("Temporal Scale Mode:"));
#if GTK_VER_AT_LEAST(3,15)
	gtk_label_set_xalign(GTK_LABEL(label_TemporalScaleMode), 1);
	gtk_label_set_yalign(GTK_LABEL(label_TemporalScaleMode), 0.5);
#else
	gtk_misc_set_alignment (GTK_MISC (label_TemporalScaleMode), 1, 0.5);
#endif
	gtk_grid_attach (GTK_GRID(h264_controls_grid), label_TemporalScaleMode, 0, line, 1, 1);
	gtk_widget_show (label_TemporalScaleMode);

	uvcx_video_config_probe_commit_t *h264_config_probe_req = v4l2core_get_h264_config_probe_req(get_v4l2_device_handler());
	
	uint8_t cur_tsmflags = h264_config_probe_req->bTemporalScaleMode & 0x07;
	uint8_t max_tsmflags = config_probe_max.bTemporalScaleMode & 0x07;
	uint8_t min_tsmflags = config_probe_min.bTemporalScaleMode & 0x07;

	GtkAdjustment *adjustment8 = gtk_adjustment_new (
                                	cur_tsmflags,
                                	min_tsmflags,
                                    max_tsmflags,
                                    1,
                                    10,
                                    0);

    TemporalScaleMode = gtk_spin_button_new(adjustment8, 1, 0);
    gtk_editable_set_editable(GTK_EDITABLE(TemporalScaleMode), TRUE);

	g_signal_connect(GTK_SPIN_BUTTON(TemporalScaleMode),"value-changed",
                                    G_CALLBACK (h264_TemporalScaleMode_changed), NULL);

    gtk_grid_attach (GTK_GRID(h264_controls_grid), TemporalScaleMode, 1, line, 1 ,1);
    gtk_widget_show (TemporalScaleMode);

	//bSpatialScaleMode
	line++;
	GtkWidget* label_SpatialScaleMode = gtk_label_new(_("Spatial Scale Mode:"));
#if GTK_VER_AT_LEAST(3,15)
	gtk_label_set_xalign(GTK_LABEL(label_SpatialScaleMode), 1);
	gtk_label_set_yalign(GTK_LABEL(label_SpatialScaleMode), 0.5);
#else
	gtk_misc_set_alignment (GTK_MISC (label_SpatialScaleMode), 1, 0.5);
#endif

	gtk_grid_attach (GTK_GRID(h264_controls_grid), label_SpatialScaleMode, 0, line, 1, 1);
	gtk_widget_show (label_SpatialScaleMode);

	uint8_t cur_ssmflags = h264_config_probe_req->bSpatialScaleMode & 0x0F;
	uint8_t max_ssmflags = config_probe_max.bSpatialScaleMode & 0x0F;
	uint8_t min_ssmflags = config_probe_min.bSpatialScaleMode & 0x0F;

	GtkAdjustment *adjustment9 = gtk_adjustment_new (
                                	cur_ssmflags,
                                	min_ssmflags,
                                    max_ssmflags,
                                    1,
                                    10,
                                    0);

    SpatialScaleMode = gtk_spin_button_new(adjustment9, 1, 0);
    gtk_editable_set_editable(GTK_EDITABLE(SpatialScaleMode), TRUE);

	g_signal_connect(GTK_SPIN_BUTTON(SpatialScaleMode),"value-changed",
                                    G_CALLBACK (h264_SpatialScaleMode_changed), NULL);

    gtk_grid_attach (GTK_GRID(h264_controls_grid), SpatialScaleMode, 1, line, 1 ,1);
    gtk_widget_show (SpatialScaleMode);

    //dwFrameInterval
    if(is_control_panel) //if streaming use fps in video tab to set FrameInterval
    {
		line++;
		GtkWidget* label_FrameInterval = gtk_label_new(_("Frame Interval (100ns units):"));
#if GTK_VER_AT_LEAST(3,15)
	gtk_label_set_xalign(GTK_LABEL(label_FrameInterval), 1);
	gtk_label_set_yalign(GTK_LABEL(label_FrameInterval), 0.5);
#else
	gtk_misc_set_alignment (GTK_MISC (label_FrameInterval), 1, 0.5);
#endif
		gtk_grid_attach (GTK_GRID(h264_controls_grid), label_FrameInterval, 0, line, 1, 1);
		gtk_widget_show (label_FrameInterval);

		//uint32_t cur_framerate = (global->fps_num * 1000000000LL / global->fps)/100;
		uint32_t cur_framerate = v4l2core_get_h264_frame_rate_config(get_v4l2_device_handler());
		uint32_t max_framerate = v4l2core_query_h264_frame_rate_config(get_v4l2_device_handler(), UVC_GET_MAX);
		uint32_t min_framerate = v4l2core_query_h264_frame_rate_config(get_v4l2_device_handler(), UVC_GET_MIN);

		GtkAdjustment *adjustment0 = gtk_adjustment_new (
										cur_framerate,
										min_framerate,
										max_framerate,
										1,
										10,
										0);

		FrameInterval = gtk_spin_button_new(adjustment0, 1, 0);
		gtk_editable_set_editable(GTK_EDITABLE(FrameInterval), TRUE);

		g_signal_connect(GTK_SPIN_BUTTON(FrameInterval),"value-changed",
										G_CALLBACK (h264_FrameInterval_changed), NULL);

		gtk_grid_attach (GTK_GRID(h264_controls_grid), FrameInterval, 1, line, 1 ,1);
		gtk_widget_show (FrameInterval);
	}

	/*
	 * probe_commit specific controls
	 */

	if(is_control_panel)
		return 0; //don't create probe_commit specific controls if we are in control panel mode

	//dwBitRate
	line++;
	GtkWidget* label_BitRate = gtk_label_new(_("Bit Rate:"));
#if GTK_VER_AT_LEAST(3,15)
	gtk_label_set_xalign(GTK_LABEL(label_BitRate), 1);
	gtk_label_set_yalign(GTK_LABEL(label_BitRate), 0.5);
#else
	gtk_misc_set_alignment (GTK_MISC (label_BitRate), 1, 0.5);
#endif
	gtk_grid_attach (GTK_GRID(h264_controls_grid), label_BitRate, 0, line, 1, 1);
	gtk_widget_show (label_BitRate);

	GtkAdjustment *adjustment1 = gtk_adjustment_new (
                                	h264_config_probe_req->dwBitRate,
                                	config_probe_min.dwBitRate,
                                    config_probe_max.dwBitRate,
                                    1,
                                    10,
                                    0);

    BitRate = gtk_spin_button_new(adjustment1, 1, 0);
    gtk_editable_set_editable(GTK_EDITABLE(BitRate), TRUE);

    gtk_grid_attach (GTK_GRID(h264_controls_grid), BitRate, 1, line, 1 ,1);
    gtk_widget_show (BitRate);

	//bmHints
	line++;
	GtkWidget* hints_table = gtk_grid_new();

	GtkWidget* label_Hints = gtk_label_new(_("Hints:"));
#if GTK_VER_AT_LEAST(3,15)
	gtk_label_set_xalign(GTK_LABEL(label_Hints), 1);
	gtk_label_set_yalign(GTK_LABEL(label_Hints), 0.5);
#else
	gtk_misc_set_alignment (GTK_MISC (label_Hints), 1, 0.5);
#endif
	gtk_grid_attach (GTK_GRID(hints_table), label_Hints, 0, 1, 2, 1);
	gtk_widget_show (label_Hints);

	Hints_res = gtk_check_button_new_with_label (_("Resolution"));
	gtk_grid_attach (GTK_GRID(hints_table), Hints_res, 0, 2, 1, 1);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(Hints_res),((h264_config_probe_req->bmHints & 0x0001) > 0));
	gtk_widget_show (Hints_res);
	Hints_prof = gtk_check_button_new_with_label (_("Profile"));
	gtk_grid_attach (GTK_GRID(hints_table), Hints_prof, 1, 2, 1, 1);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(Hints_prof),((h264_config_probe_req->bmHints & 0x0002) > 0));
	gtk_widget_show (Hints_prof);
	Hints_ratecontrol = gtk_check_button_new_with_label (_("Rate Control"));
	gtk_grid_attach (GTK_GRID(hints_table), Hints_ratecontrol, 2, 2, 1, 1);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(Hints_ratecontrol),((h264_config_probe_req->bmHints & 0x0004) > 0));
	gtk_widget_show (Hints_ratecontrol);
	Hints_usage = gtk_check_button_new_with_label (_("Usage Type"));
	gtk_grid_attach (GTK_GRID(hints_table), Hints_usage, 3, 2, 1, 1);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(Hints_usage),((h264_config_probe_req->bmHints & 0x0008) > 0));
	gtk_widget_show (Hints_usage);
	Hints_slicemode = gtk_check_button_new_with_label (_("Slice Mode"));
	gtk_grid_attach (GTK_GRID(hints_table), Hints_slicemode, 0, 3, 1, 1);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(Hints_slicemode),((h264_config_probe_req->bmHints & 0x0010) > 0));
	gtk_widget_show (Hints_slicemode);
	Hints_sliceunit = gtk_check_button_new_with_label (_("Slice Unit"));
	gtk_grid_attach (GTK_GRID(hints_table), Hints_sliceunit, 1, 3, 1, 1);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(Hints_sliceunit),((h264_config_probe_req->bmHints & 0x0020) > 0));
	gtk_widget_show (Hints_sliceunit);
	Hints_view = gtk_check_button_new_with_label (_("MVC View"));
	gtk_grid_attach (GTK_GRID(hints_table), Hints_view, 2, 3, 1, 1);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(Hints_view),((h264_config_probe_req->bmHints & 0x0040) > 0));
	gtk_widget_show (Hints_view);
	Hints_temporal = gtk_check_button_new_with_label (_("Temporal Scale"));
	gtk_grid_attach (GTK_GRID(hints_table), Hints_temporal, 3, 3, 1, 1);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(Hints_temporal),((h264_config_probe_req->bmHints & 0x0080) > 0));
	gtk_widget_show (Hints_temporal);
	Hints_snr = gtk_check_button_new_with_label (_("SNR Scale"));
	gtk_grid_attach (GTK_GRID(hints_table), Hints_snr, 0, 4, 1, 1);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(Hints_snr),((h264_config_probe_req->bmHints & 0x0100) > 0));
	gtk_widget_show (Hints_snr);
	Hints_spatial = gtk_check_button_new_with_label (_("Spatial Scale"));
	gtk_grid_attach (GTK_GRID(hints_table), Hints_spatial, 1, 4, 1, 1);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(Hints_spatial),((h264_config_probe_req->bmHints & 0x0200) > 0));
	gtk_widget_show (Hints_spatial);
	Hints_spatiallayer = gtk_check_button_new_with_label (_("Spatial Layer Ratio"));
	gtk_grid_attach (GTK_GRID(hints_table), Hints_spatiallayer, 2, 4, 1, 1);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(Hints_spatiallayer),((h264_config_probe_req->bmHints & 0x0400) > 0));
	gtk_widget_show (Hints_spatiallayer);
	Hints_frameinterval = gtk_check_button_new_with_label (_("Frame Interval"));
	gtk_grid_attach (GTK_GRID(hints_table), Hints_frameinterval, 3, 4, 1, 1);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(Hints_frameinterval),((h264_config_probe_req->bmHints & 0x0800) > 0));
	gtk_widget_show (Hints_frameinterval);
	Hints_leakybucket = gtk_check_button_new_with_label (_("Leaky Bucket Size"));
	gtk_grid_attach (GTK_GRID(hints_table), Hints_leakybucket, 0, 5, 1, 1);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(Hints_leakybucket),((h264_config_probe_req->bmHints & 0x1000) > 0));
	gtk_widget_show (Hints_leakybucket);
	Hints_bitrate = gtk_check_button_new_with_label (_("Bit Rate"));
	gtk_grid_attach (GTK_GRID(hints_table), Hints_bitrate, 1, 5, 1, 1);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(Hints_bitrate),((h264_config_probe_req->bmHints & 0x2000) > 0));
	gtk_widget_show (Hints_bitrate);
	Hints_cabac = gtk_check_button_new_with_label (_("CABAC"));
	gtk_grid_attach (GTK_GRID(hints_table), Hints_cabac, 2, 5, 1, 1);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(Hints_cabac),((h264_config_probe_req->bmHints & 0x4000) > 0));
	gtk_widget_show (Hints_cabac);
	Hints_iframe = gtk_check_button_new_with_label (_("(I) Frame Period"));
	gtk_grid_attach (GTK_GRID(hints_table), Hints_iframe, 3, 5, 1, 1);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON( Hints_iframe),((h264_config_probe_req->bmHints & 0x8000) > 0));
	gtk_widget_show (Hints_iframe);

	gtk_grid_attach (GTK_GRID(h264_controls_grid), hints_table, 0, line, 2, 1);
	gtk_widget_show(hints_table);

	//wWidth x wHeight - use global values

	//wSliceMode
	line++;
	GtkWidget* label_SliceMode = gtk_label_new(_("Slice Mode:"));
#if GTK_VER_AT_LEAST(3,15)
	gtk_label_set_xalign(GTK_LABEL(label_SliceMode), 1);
	gtk_label_set_yalign(GTK_LABEL(label_SliceMode), 0.5);
#else
	gtk_misc_set_alignment (GTK_MISC (label_SliceMode), 1, 0.5);
#endif
	gtk_grid_attach (GTK_GRID(h264_controls_grid), label_SliceMode, 0, line, 1, 1);
	gtk_widget_show (label_SliceMode);

	SliceMode = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(SliceMode),
								_("no multiple slices"));
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(SliceMode),
								_("bits/slice"));
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(SliceMode),
								_("Mbits/slice"));
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(SliceMode),
								_("slices/frame"));

	gtk_combo_box_set_active(GTK_COMBO_BOX(SliceMode), h264_config_probe_req->wSliceMode); //0 indexed

	gtk_grid_attach (GTK_GRID(h264_controls_grid), SliceMode, 1, line, 1 ,1);
	gtk_widget_show (SliceMode);

	//wSliceUnits
	line++;
	GtkWidget* label_SliceUnits = gtk_label_new(_("Slice Units:"));
#if GTK_VER_AT_LEAST(3,15)
	gtk_label_set_xalign(GTK_LABEL(label_SliceUnits), 1);
	gtk_label_set_yalign(GTK_LABEL(label_SliceUnits), 0.5);
#else
	gtk_misc_set_alignment (GTK_MISC (label_SliceUnits), 1, 0.5);
#endif
	gtk_grid_attach (GTK_GRID(h264_controls_grid), label_SliceUnits, 0, line, 1, 1);
	gtk_widget_show (label_SliceUnits);

	GtkAdjustment *adjustment2 = gtk_adjustment_new (
                                	h264_config_probe_req->wSliceUnits,
                                	config_probe_min.wSliceUnits,
                                    config_probe_max.wSliceUnits,
                                    1,
                                    10,
                                    0);

    SliceUnits = gtk_spin_button_new(adjustment2, 1, 0);
    gtk_editable_set_editable(GTK_EDITABLE(SliceUnits), TRUE);

    gtk_grid_attach (GTK_GRID(h264_controls_grid), SliceUnits, 1, line, 1 ,1);
    gtk_widget_show (SliceUnits);

	//wProfile
	line++;
	GtkWidget* label_Profile = gtk_label_new(_("Profile:"));
#if GTK_VER_AT_LEAST(3,15)
	gtk_label_set_xalign(GTK_LABEL(label_Profile), 1);
	gtk_label_set_yalign(GTK_LABEL(label_Profile), 0.5);
#else
	gtk_misc_set_alignment (GTK_MISC (label_Profile), 1, 0.5);
#endif
	gtk_grid_attach (GTK_GRID(h264_controls_grid), label_Profile, 0, line, 1, 1);
	gtk_widget_show (label_Profile);

	Profile = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(Profile),
								_("Baseline Profile"));
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(Profile),
								_("Main Profile"));
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(Profile),
								_("High Profile"));
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(Profile),
								_("Scalable Baseline Profile"));
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(Profile),
								_("Scalable High Profile"));
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(Profile),
								_("Multiview High Profile"));
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(Profile),
								_("Stereo High Profile"));

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
			fprintf(stderr, "GUVCVIEW: (H264 probe) unknown profile mode 0x%X\n", profile);
			break;

	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(Profile), prof_index);

	gtk_grid_attach (GTK_GRID(h264_controls_grid), Profile, 1, line, 1 ,1);
	gtk_widget_show (Profile);

	//wProfile (Bits 0-7)
	line++;
	GtkWidget* label_Profile_flags = gtk_label_new(_("Profile flags:"));
#if GTK_VER_AT_LEAST(3,15)
	gtk_label_set_xalign(GTK_LABEL(label_Profile_flags), 1);
	gtk_label_set_yalign(GTK_LABEL(label_Profile_flags), 0.5);
#else
	gtk_misc_set_alignment (GTK_MISC (label_Profile_flags), 1, 0.5);
#endif
	gtk_grid_attach (GTK_GRID(h264_controls_grid), label_Profile_flags, 0, line, 1, 1);
	gtk_widget_show (label_Profile_flags);

	int cur_flags = h264_config_probe_req->wProfile & 0x000000FF;
	int max_flags = config_probe_max.wProfile & 0x000000FF;
	int min_flags = config_probe_min.wProfile & 0x000000FF;

	GtkAdjustment *adjustment3 = gtk_adjustment_new (
                                	cur_flags,
                                	min_flags,
                                    max_flags,
                                    1,
                                    10,
                                    0);

    Profile_flags = gtk_spin_button_new(adjustment3, 1, 0);
    gtk_editable_set_editable(GTK_EDITABLE(Profile_flags), TRUE);

    gtk_grid_attach (GTK_GRID(h264_controls_grid), Profile_flags, 1, line, 1 ,1);
    gtk_widget_show (Profile_flags);

	//wIFramePeriod
	line++;
	GtkWidget* label_IFramePeriod = gtk_label_new(_("(I) Frame Period (ms):"));
#if GTK_VER_AT_LEAST(3,15)
	gtk_label_set_xalign(GTK_LABEL(label_IFramePeriod), 1);
	gtk_label_set_yalign(GTK_LABEL(label_IFramePeriod), 0.5);
#else
	gtk_misc_set_alignment (GTK_MISC (label_IFramePeriod), 1, 0.5);
#endif
	gtk_grid_attach (GTK_GRID(h264_controls_grid), label_IFramePeriod, 0, line, 1, 1);
	gtk_widget_show (label_IFramePeriod);

	GtkAdjustment *adjustment4 = gtk_adjustment_new (
                                	h264_config_probe_req->wIFramePeriod,
                                	config_probe_min.wIFramePeriod,
                                    config_probe_max.wIFramePeriod,
                                    1,
                                    10,
                                    0);

    IFramePeriod = gtk_spin_button_new(adjustment4, 1, 0);
    gtk_editable_set_editable(GTK_EDITABLE(IFramePeriod), TRUE);

    gtk_grid_attach (GTK_GRID(h264_controls_grid), IFramePeriod, 1, line, 1 ,1);
    gtk_widget_show (IFramePeriod);

	//wEstimatedVideoDelay
	line++;
	GtkWidget* label_EstimatedVideoDelay = gtk_label_new(_("Estimated Video Delay (ms):"));
#if GTK_VER_AT_LEAST(3,15)
	gtk_label_set_xalign(GTK_LABEL(label_EstimatedVideoDelay), 1);
	gtk_label_set_yalign(GTK_LABEL(label_EstimatedVideoDelay), 0.5);
#else
	gtk_misc_set_alignment (GTK_MISC (label_EstimatedVideoDelay), 1, 0.5);
#endif
	gtk_grid_attach (GTK_GRID((h264_controls_grid)), label_EstimatedVideoDelay, 0, line, 1, 1);
	gtk_widget_show (label_EstimatedVideoDelay);

	GtkAdjustment *adjustment5 = gtk_adjustment_new (
                                	h264_config_probe_req->wEstimatedVideoDelay,
                                	config_probe_min.wEstimatedVideoDelay,
                                    config_probe_max.wEstimatedVideoDelay,
                                    1,
                                    10,
                                    0);

    EstimatedVideoDelay = gtk_spin_button_new(adjustment5, 1, 0);
    gtk_editable_set_editable(GTK_EDITABLE(EstimatedVideoDelay), TRUE);

    gtk_grid_attach (GTK_GRID((h264_controls_grid)), EstimatedVideoDelay, 1, line, 1 ,1);
    gtk_widget_show (EstimatedVideoDelay);

    //wEstimatedMaxConfigDelay
	line++;
	GtkWidget* label_EstimatedMaxConfigDelay = gtk_label_new(_("Estimated Max Config Delay (ms):"));
#if GTK_VER_AT_LEAST(3,15)
	gtk_label_set_xalign(GTK_LABEL(label_EstimatedMaxConfigDelay), 1);
	gtk_label_set_yalign(GTK_LABEL(label_EstimatedMaxConfigDelay), 0.5);
#else
	gtk_misc_set_alignment (GTK_MISC (label_EstimatedMaxConfigDelay), 1, 0.5);
#endif
	gtk_grid_attach (GTK_GRID(h264_controls_grid), label_EstimatedMaxConfigDelay, 0, line, 1, 1);
	gtk_widget_show (label_EstimatedMaxConfigDelay);

	GtkAdjustment *adjustment6 = gtk_adjustment_new (
                                	h264_config_probe_req->wEstimatedMaxConfigDelay,
                                	config_probe_min.wEstimatedMaxConfigDelay,
                                    config_probe_max.wEstimatedMaxConfigDelay,
                                    1,
                                    10,
                                    0);

    EstimatedMaxConfigDelay = gtk_spin_button_new(adjustment6, 1, 0);
    gtk_editable_set_editable(GTK_EDITABLE(EstimatedMaxConfigDelay), TRUE);

    gtk_grid_attach (GTK_GRID(h264_controls_grid), EstimatedMaxConfigDelay, 1, line, 1 ,1);
    gtk_widget_show (EstimatedMaxConfigDelay);

	//bUsageType
	line++;
	GtkWidget* label_UsageType = gtk_label_new(_("Usage Type:"));
#if GTK_VER_AT_LEAST(3,15)
	gtk_label_set_xalign(GTK_LABEL(label_UsageType), 1);
	gtk_label_set_yalign(GTK_LABEL(label_UsageType), 0.5);
#else
	gtk_misc_set_alignment (GTK_MISC (label_UsageType), 1, 0.5);
#endif
	gtk_grid_attach (GTK_GRID(h264_controls_grid), label_UsageType, 0, line, 1, 1);
	gtk_widget_show (label_UsageType);

	UsageType = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(UsageType),
								_("Real-time"));
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(UsageType),
								_("Broadcast"));
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(UsageType),
								_("Storage"));
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(UsageType),
								_("(0) Non-scalable single layer AVC"));
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(UsageType),
								_("(1) SVC temporal scalability with hierarchical P"));
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(UsageType),
								_("(2q) SVC temporal scalability + Quality/SNR scalability"));
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(UsageType),
								_("(2s) SVC temporal scalability + spatial scalability"));
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(UsageType),
								_("(3) Full SVC scalability"));

	uint8_t usage = h264_config_probe_req->bUsageType & 0x0F;
	int usage_index = usage - 1; // from 0x01 to 0x0F
	if(usage_index < 0)
		usage_index = 0;

	gtk_combo_box_set_active(GTK_COMBO_BOX(UsageType), usage_index);

	gtk_grid_attach (GTK_GRID(h264_controls_grid), UsageType, 1, line, 1 ,1);
	gtk_widget_show (UsageType);

	//bSNRScaleMode
	line++;
	GtkWidget* label_SNRScaleMode = gtk_label_new(_("SNR Control Mode:"));
#if GTK_VER_AT_LEAST(3,15)
	gtk_label_set_xalign(GTK_LABEL(label_SNRScaleMode), 1);
	gtk_label_set_yalign(GTK_LABEL(label_SNRScaleMode), 0.5);
#else
	gtk_misc_set_alignment (GTK_MISC (label_SNRScaleMode), 1, 0.5);
#endif
	gtk_grid_attach (GTK_GRID(h264_controls_grid), label_SNRScaleMode, 0, line, 1, 1);
	gtk_widget_show (label_SNRScaleMode);

	SNRScaleMode = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(SNRScaleMode),
								_("No SNR Enhancement Layer"));
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(SNRScaleMode),
								_("CGS NonRewrite (2 Layer)"));
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(SNRScaleMode),
								_("CGS NonRewrite (3 Layer)"));
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(SNRScaleMode),
								_("CGS Rewrite (2 Layer)"));
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(SNRScaleMode),
								_("CGS Rewrite (3 Layer)"));
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(SNRScaleMode),
								_("MGS (2 Layer)"));

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
	gtk_combo_box_set_active(GTK_COMBO_BOX(SNRScaleMode), snrscalemode_index);

	gtk_grid_attach (GTK_GRID(h264_controls_grid), SNRScaleMode, 1, line, 1 ,1);
	gtk_widget_show (SNRScaleMode);

	//bStreamMuxOption (done directly in the core)
	line++;
	GtkWidget *StreamMuxOption_table = gtk_grid_new();
	StreamMuxOption = gtk_check_button_new_with_label (_("Stream Mux Enable"));
	gtk_grid_attach (GTK_GRID(StreamMuxOption_table), StreamMuxOption, 0, 1, 1, 1);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(StreamMuxOption),((h264_config_probe_req->bStreamMuxOption & 0x01) != 0));
	gtk_widget_show (StreamMuxOption);

	StreamMuxOption_aux = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(StreamMuxOption_aux),
								_("Embed H.264 aux stream"));
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(StreamMuxOption_aux),
								_("Embed YUY2 aux stream"));
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(StreamMuxOption_aux),
								_("Embed NV12 aux stream"));

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
	gtk_combo_box_set_active(GTK_COMBO_BOX(StreamMuxOption_aux), streammux_index);
	gtk_widget_show(StreamMuxOption_aux);
	gtk_grid_attach (GTK_GRID(StreamMuxOption_table), StreamMuxOption_aux, 1, 1, 1, 1);

	StreamMuxOption_mjpgcontainer = gtk_check_button_new_with_label (_("MJPG payload container"));
	gtk_grid_attach (GTK_GRID(StreamMuxOption_table), StreamMuxOption_mjpgcontainer, 2, 1, 1, 1);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(StreamMuxOption_mjpgcontainer),((h264_config_probe_req->bStreamMuxOption & 0x40) != 0));
	gtk_widget_show (StreamMuxOption_mjpgcontainer);

	gtk_grid_attach (GTK_GRID((h264_controls_grid)), StreamMuxOption_table, 0, line, 2, 1);
	gtk_widget_show(StreamMuxOption_table);

	gtk_widget_set_sensitive(StreamMuxOption, FALSE); //No support yet (set in the core)
	gtk_widget_set_sensitive(StreamMuxOption_aux, FALSE); //No support yet (set in the core)
	gtk_widget_set_sensitive(StreamMuxOption_mjpgcontainer, FALSE); //No support yet (set in the core)

	//bStreamFormat
	line++;
	GtkWidget* label_StreamFormat = gtk_label_new(_("Stream Format:"));
#if GTK_VER_AT_LEAST(3,15)
	gtk_label_set_xalign(GTK_LABEL(label_StreamFormat), 1);
	gtk_label_set_yalign(GTK_LABEL(label_StreamFormat), 0.5);
#else
	gtk_misc_set_alignment (GTK_MISC (label_StreamFormat), 1, 0.5);
#endif
	gtk_grid_attach (GTK_GRID((h264_controls_grid)), label_StreamFormat, 0, line, 1, 1);
	gtk_widget_show (label_StreamFormat);

	StreamFormat = gtk_combo_box_text_new();
	//TODO: check for min and max values
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(StreamFormat),
								_("Byte stream format (H.264 Annex-B)"));
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(StreamFormat),
								_("NAL stream format"));

	uint8_t streamformat = h264_config_probe_req->bStreamFormat & 0x01;
	int streamformat_index = streamformat;
	gtk_combo_box_set_active(GTK_COMBO_BOX(StreamFormat), streamformat_index);

	gtk_grid_attach (GTK_GRID(h264_controls_grid), StreamFormat, 1, line, 1 ,1);
	gtk_widget_show (StreamFormat);

	//bEntropyCABAC
	line++;
	GtkWidget* label_EntropyCABAC = gtk_label_new(_("Entropy CABAC:"));
#if GTK_VER_AT_LEAST(3,15)
	gtk_label_set_xalign(GTK_LABEL(label_EntropyCABAC), 1);
	gtk_label_set_yalign(GTK_LABEL(label_EntropyCABAC), 0.5);
#else
	gtk_misc_set_alignment (GTK_MISC (label_EntropyCABAC), 1, 0.5);
#endif
	gtk_grid_attach (GTK_GRID(h264_controls_grid), label_EntropyCABAC, 0, line, 1, 1);
	gtk_widget_show (label_EntropyCABAC);

	EntropyCABAC = gtk_combo_box_text_new();
	//TODO: check for min and max values
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(EntropyCABAC),
								_("CAVLC"));
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(EntropyCABAC),
								_("CABAC"));

	uint8_t entropycabac = h264_config_probe_req->bEntropyCABAC & 0x01;
	int entropycabac_index = entropycabac;
	gtk_combo_box_set_active(GTK_COMBO_BOX(EntropyCABAC), entropycabac_index);

	gtk_grid_attach (GTK_GRID(h264_controls_grid), EntropyCABAC, 1, line, 1 ,1);
	gtk_widget_show (EntropyCABAC);

	//bTimestamp
	line++;
	Timestamp = gtk_check_button_new_with_label (_("Picture timing SEI"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(Timestamp),((h264_config_probe_req->bTimestamp & 0x01) > 0));
	gtk_grid_attach (GTK_GRID(h264_controls_grid), Timestamp, 1, line, 1 ,1);
	gtk_widget_show (Timestamp);

	//bNumOfReorderFrames
	line++;
	GtkWidget* label_NumOfReorderFrames = gtk_label_new(_("B Frames:"));
#if GTK_VER_AT_LEAST(3,15)
	gtk_label_set_xalign(GTK_LABEL(label_NumOfReorderFrames), 1);
	gtk_label_set_yalign(GTK_LABEL(label_NumOfReorderFrames), 0.5);
#else
	gtk_misc_set_alignment (GTK_MISC (label_NumOfReorderFrames), 1, 0.5);
#endif
	gtk_grid_attach (GTK_GRID((h264_controls_grid)), label_NumOfReorderFrames, 0, line, 1, 1);
	gtk_widget_show (label_NumOfReorderFrames);

	cur_flags = h264_config_probe_req->bNumOfReorderFrames & 0x000000FF;
	max_flags = config_probe_max.bNumOfReorderFrames & 0x000000FF;
	min_flags = config_probe_min.bNumOfReorderFrames & 0x000000FF;

	GtkAdjustment *adjustment10 = gtk_adjustment_new (
                                	cur_flags,
                                	min_flags,
                                    max_flags,
                                    1,
                                    10,
                                    0);

    NumOfReorderFrames = gtk_spin_button_new(adjustment10, 1, 0);
    gtk_editable_set_editable(GTK_EDITABLE(NumOfReorderFrames), TRUE);

    gtk_grid_attach (GTK_GRID(h264_controls_grid), NumOfReorderFrames, 1, line, 1 ,1);
    gtk_widget_show (NumOfReorderFrames);

	//bPreviewFlipped
	line++;
	PreviewFlipped = gtk_check_button_new_with_label (_("Preview Flipped"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(PreviewFlipped),((h264_config_probe_req->bPreviewFlipped & 0x01) > 0));
	gtk_grid_attach (GTK_GRID(h264_controls_grid), PreviewFlipped, 1, line, 1 ,1);
	gtk_widget_show (PreviewFlipped);

	//bView
	line++;
	GtkWidget* label_View = gtk_label_new(_("Additional MVC Views:"));
#if GTK_VER_AT_LEAST(3,15)
	gtk_label_set_xalign(GTK_LABEL(label_View), 1);
	gtk_label_set_yalign(GTK_LABEL(label_View), 0.5);
#else
	gtk_misc_set_alignment (GTK_MISC (label_View), 1, 0.5);
#endif
	gtk_grid_attach (GTK_GRID(h264_controls_grid), label_View, 0, line, 1, 1);
	gtk_widget_show (label_View);

	cur_flags = h264_config_probe_req->bView & 0x000000FF;
	max_flags = config_probe_max.bView & 0x000000FF;
	min_flags = config_probe_min.bView & 0x000000FF;

	GtkAdjustment *adjustment11 = gtk_adjustment_new (
                                	cur_flags,
                                	min_flags,
                                    max_flags,
                                    1,
                                    10,
                                    0);

    View = gtk_spin_button_new(adjustment11, 1, 0);
    gtk_editable_set_editable(GTK_EDITABLE(View), TRUE);

    gtk_grid_attach (GTK_GRID(h264_controls_grid), View, 1, line, 1 ,1);
    gtk_widget_show (View);

    //bStreamID
	line++;
	GtkWidget* label_StreamID = gtk_label_new(_("Simulcast stream index:"));
#if GTK_VER_AT_LEAST(3,15)
	gtk_label_set_xalign(GTK_LABEL(label_StreamID), 1);
	gtk_label_set_yalign(GTK_LABEL(label_StreamID), 0.5);
#else
	gtk_misc_set_alignment (GTK_MISC (label_StreamID), 1, 0.5);
#endif
	gtk_grid_attach (GTK_GRID(h264_controls_grid), label_StreamID, 0, line, 1, 1);
	gtk_widget_show (label_StreamID);

	cur_flags = h264_config_probe_req->bStreamID & 0x000000FF;
	max_flags = config_probe_max.bStreamID & 0x000000FF;
	min_flags = config_probe_min.bStreamID & 0x000000FF;

	GtkAdjustment *adjustment12 = gtk_adjustment_new (
                                	cur_flags,
                                	min_flags,
                                    max_flags,
                                    1,
                                    10,
                                    0);

    StreamID = gtk_spin_button_new(adjustment12, 1, 0);
    gtk_editable_set_editable(GTK_EDITABLE(StreamID), TRUE);

    gtk_grid_attach (GTK_GRID((h264_controls_grid)), StreamID, 1, line, 1 ,1);
    gtk_widget_show (StreamID);

    //bSpatialLayerRatio
	line++;
	GtkWidget* label_SpatialLayerRatio = gtk_label_new(_("Spatial Layer Ratio:"));
#if GTK_VER_AT_LEAST(3,15)
	gtk_label_set_xalign(GTK_LABEL(label_SpatialLayerRatio), 1);
	gtk_label_set_yalign(GTK_LABEL(label_SpatialLayerRatio), 0.5);
#else
	gtk_misc_set_alignment (GTK_MISC (label_SpatialLayerRatio), 1, 0.5);
#endif
	gtk_grid_attach (GTK_GRID(h264_controls_grid), label_SpatialLayerRatio, 0, line, 1, 1);
	gtk_widget_show (label_SpatialLayerRatio);

	cur_flags = h264_config_probe_req->bSpatialLayerRatio & 0x000000FF;
	max_flags = config_probe_max.bSpatialLayerRatio & 0x000000FF;
	min_flags = config_probe_min.bSpatialLayerRatio & 0x000000FF;

	gdouble cur = (gdouble) ((cur_flags & 0x000000F0)>>4) + (gdouble)((cur_flags & 0x0000000F)/16);
	gdouble min = (gdouble) ((min_flags & 0x000000F0)>>4) + (gdouble)((min_flags & 0x0000000F)/16);
	gdouble max = (gdouble) ((max_flags & 0x000000F0)>>4) + (gdouble)((max_flags & 0x0000000F)/16);

	GtkAdjustment *adjustment13 = gtk_adjustment_new (
                                	cur,
                                	min,
                                    max,
                                    0.1,
                                    1,
                                    1);

    SpatialLayerRatio = gtk_spin_button_new(adjustment13, 1, 0);
    gtk_editable_set_editable(GTK_EDITABLE(SpatialLayerRatio), TRUE);

    gtk_grid_attach (GTK_GRID(h264_controls_grid), SpatialLayerRatio, 1, line, 1 ,1);
    gtk_widget_show (SpatialLayerRatio);

	//wLeakyBucketSize
	line++;
	GtkWidget* label_LeakyBucketSize = gtk_label_new(_("Leaky Bucket Size (ms):"));
#if GTK_VER_AT_LEAST(3,15)
	gtk_label_set_xalign(GTK_LABEL(label_LeakyBucketSize), 1);
	gtk_label_set_yalign(GTK_LABEL(label_LeakyBucketSize), 0.5);
#else
	gtk_misc_set_alignment (GTK_MISC (label_LeakyBucketSize), 1, 0.5);
#endif
	gtk_grid_attach (GTK_GRID(h264_controls_grid), label_LeakyBucketSize, 0, line, 1, 1);
	gtk_widget_show (label_LeakyBucketSize);

	cur_flags = h264_config_probe_req->wLeakyBucketSize;
	max_flags = config_probe_max.wLeakyBucketSize;
	min_flags = config_probe_min.wLeakyBucketSize;

	GtkAdjustment *adjustment14 = gtk_adjustment_new (
                                	cur_flags,
                                	min_flags,
                                    max_flags,
                                    1,
                                    10,
                                    0);

    LeakyBucketSize = gtk_spin_button_new(adjustment14, 1, 0);
    gtk_editable_set_editable(GTK_EDITABLE(LeakyBucketSize), TRUE);

    gtk_grid_attach (GTK_GRID((h264_controls_grid)), LeakyBucketSize, 1, line, 1 ,1);
    gtk_widget_show (LeakyBucketSize);

	//PROBE COMMIT buttons
	line++;

	//encoder reset
	GtkWidget *reset_button = gtk_button_new_with_label(_("Encoder Reset"));
	g_signal_connect (GTK_BUTTON(reset_button), "clicked",
                                G_CALLBACK (h264_reset_button_clicked), NULL);

    gtk_grid_attach (GTK_GRID((h264_controls_grid)), reset_button, 0, line, 1 ,1);
	gtk_widget_show(reset_button);

/*
	GtkWidget *probe_button = gtk_button_new_with_label(_("PROBE"));
	g_signal_connect (GTK_BUTTON(probe_button), "clicked",
                                G_CALLBACK (h264_probe_button_clicked), all_data);

    gtk_grid_attach (GTK_GRID(h264_controls_grid), probe_button, 0, line, 1 ,1);
	gtk_widget_show(probe_button);
*/
	GtkWidget *commit_button = gtk_button_new_with_label(_("COMMIT"));
	g_signal_connect (GTK_BUTTON(commit_button), "clicked",
                                G_CALLBACK (h264_commit_button_clicked), NULL);

    gtk_grid_attach (GTK_GRID((h264_controls_grid)), commit_button, 1, line, 1 ,1);
	gtk_widget_show(commit_button);

	gtk_widget_show(h264_controls_grid);

	/*add control grid to parent container*/
	gtk_container_add(GTK_CONTAINER(parent), h264_controls_grid);

	return 0;
}
