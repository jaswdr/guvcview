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

#ifndef GUI_QT5_HPP
#define GUI_QT5_HPP

#include <QtWidgets>

extern "C" {
#include "gviewv4l2core.h"
}

class ControlWidgets
{
public:
	ControlWidgets();
	
	int id;                       /*control id*/
	QWidget *label;               /*control label widget*/
	QWidget *widget;              /*control widget 1*/
	QWidget *widget2;             /*control widget 2*/
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();
    ~MainWindow();
	std::vector<ControlWidgets *> control_widgets_list;

public slots:
	void capture_video_clicked();
	void capture_image_clicked();
	void set_statusbar_message(QString message);

protected:
   void closeEvent(QCloseEvent *event);
   void keyPressEvent(QKeyEvent* e);

private slots:
	void quit_button_clicked();
	/*image*/
	void slider_value_changed(int value);
	void spin_value_changed (int value);
	void button_PanTilt1_clicked();
	void button_PanTilt2_clicked();
	void button_clicked();
	void pan_tilt_step_changed(int value);
	void combo_changed (int index);
	void bayer_pix_ord_changed(int index);
	void check_changed (int state);
	void autofocus_changed(int state);
	void setfocus_clicked();
	void int64_button_clicked();
	void string_button_clicked();
	void bitmask_button_clicked();
	/*video*/
    void devices_changed (int index);
    void frame_rate_changed (int index);
    void resolution_changed (int index);
    void format_changed(int index);
    void render_fx_filter_changed(int state);
    void render_osd_changed(int state);
	/*h264*/
	void h264_rate_control_mode_changed(int index);
	void h264_TemporalScaleMode_changed(int value);
	void h264_SpatialScaleMode_changed(int value);
	void h264_FrameInterval_changed(int value);
	void h264_commit_button_clicked();
	void h264_reset_button_clicked();

    /*audio*/
    void audio_api_changed(int index);
    void audio_devices_changed(int index);
    void audio_samplerate_changed(int index);
    void audio_channels_changed(int index);
    void audio_latency_changed(double value);
    void audio_fx_filter_changed(int state);
    /*menu*/
    void control_defaults_clicked ();
    void load_save_profile_clicked();
    void menu_camera_button_clicked();
    void photo_file_clicked();
    void photo_sufix_clicked();
    void video_file_clicked();
    void video_sufix_clicked();
    void video_codec_clicked();
    void video_codec_properties();
    void audio_codec_clicked();
    void audio_codec_properties();
    /*timer*/
    void check_device_events();
	void check_control_events();


private:
   ControlWidgets *gui_qt5_get_widgets_by_id(int id);
   void gui_qt5_update_controls_state();
   int gui_attach_qt5_v4l2ctrls(QWidget *parent);
   int gui_attach_qt5_videoctrls(QWidget *parent);
   int gui_attach_qt5_audioctrls(QWidget *parent);
   int gui_attach_qt5_menu(QWidget *parent);
   int gui_attach_qt5_h264ctrls (QWidget *parent);

   //h264
   void update_h264_controls();
   void fill_video_config_probe ();

   QTimer *timer_check_device;
   QTimer *timer_check_control_events;

   QWidget *img_controls_grid;
   QWidget *h264_controls_grid;
   QWidget *video_controls_grid;
   QWidget *audio_controls_grid;

   QToolButton *cap_img_button;
   QToolButton *cap_video_button;

   QComboBox *combobox_video_devices;
   QComboBox *combobox_FrameRate;
   QComboBox *combobox_resolution;
   QComboBox *combobox_InpType;

   QComboBox *combobox_audio_api;
   QComboBox *combobox_audio_devices;
   QComboBox *combobox_audio_channels; 
   QComboBox *combobox_audio_samprate;
   QDoubleSpinBox *spinbox_audio_latency;

   QMenuBar *menubar;
   QStatusBar *statusbar;

   QAction *webm_vcodec_action;
   QAction *webm_acodec_action;

	//h264 controls
	QComboBox *RateControlMode;
	QSpinBox *RateControlMode_cbr_flag;
	QSpinBox *TemporalScaleMode;
	QSpinBox *SpatialScaleMode;
	QSpinBox *FrameInterval;
	QSpinBox *BitRate;
	QCheckBox *Hints_res;
	QCheckBox *Hints_prof;
	QCheckBox *Hints_ratecontrol;
	QCheckBox *Hints_usage;
	QCheckBox *Hints_slicemode;
	QCheckBox *Hints_sliceunit;
	QCheckBox *Hints_view;
	QCheckBox *Hints_temporal;
	QCheckBox *Hints_snr;
	QCheckBox *Hints_spatial;
	QCheckBox *Hints_spatiallayer;
	QCheckBox *Hints_frameinterval;
	QCheckBox *Hints_leakybucket;
	QCheckBox *Hints_bitrate;
	QCheckBox *Hints_cabac;
	QCheckBox *Hints_iframe;
	QComboBox *SliceMode;
	QSpinBox *SliceUnits;
	QComboBox *Profile;
	QSpinBox *Profile_flags;
	QSpinBox *IFramePeriod;
	QSpinBox *EstimatedVideoDelay;
	QSpinBox *EstimatedMaxConfigDelay;
	QComboBox *UsageType;
	QComboBox *SNRScaleMode;
	QComboBox *StreamFormat;
	QComboBox *EntropyCABAC;
	QCheckBox *Timestamp;
	QSpinBox *NumOfReorderFrames;
	QCheckBox *PreviewFlipped;
	QSpinBox *View;
	QSpinBox *StreamID;
	QDoubleSpinBox *SpatialLayerRatio;
	QSpinBox *LeakyBucketSize;

	/*disabled*/
	QCheckBox *StreamMuxOption;
	QComboBox *StreamMuxOption_aux;
	QCheckBox *StreamMuxOption_mjpgcontainer;
};

#endif
