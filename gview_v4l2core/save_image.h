/*******************************************************************************#
#           guvcview              http://guvcview.sourceforge.net               #
#                                                                               #
#           Paulo Assis <pj.assis@gmail.com>                                    #
#           Nobuhiro Iwamatsu <iwamatsu@nigauri.org>                            #
#                             Add UYVY color support(Macbook iSight)            #
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

#ifndef SAVE_IMAGE_H
#define SAVE_IMAGE_H

#include "gviewv4l2core.h"
#include "v4l2_core.h"

/*
 * save the current frame to file
 * args:
 *    frame - pointer to frame buffer
 *    filename - output file name
 *    format - image type
 *           (IMG_FMT_RAW, IMG_FMT_JPG, IMG_FMT_PNG, IMG_FMT_BMP)
 *
 * asserts:
 *    vd is not null
 *
 * returns: error code
 */
int save_frame_image(v4l2_frame_buff_t *frame, const char *filename, int format);

/*
 * save frame data to a jpeg file
 * args:
 *    frame - pointer to frame buffer
 *    filename - filename string
 *
 * asserts:
 *    vd is not null
 *
 * returns: error code
 */
int save_image_jpeg(v4l2_frame_buff_t *frame, const char *filename);

/*
 * save frame data to a bmp file
 * args:
 *    frame - pointer to frame buffer
 *    filename - filename string
 *
 * asserts:
 *    vd is not null
 *
 * returns: error code
 */
int save_image_bmp(v4l2_frame_buff_t *frame, const char *filename);

/*
 * save frame data into a png file
 * args:
 *    frame - pointer to frame buffer
 *    filename - string with png filename name
 *
 * asserts:
 *   vd is not null
 *
 * returns: error code
 */
int save_image_png(v4l2_frame_buff_t *frame, const char *filename);

#endif