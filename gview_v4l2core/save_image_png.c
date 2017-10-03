/*******************************************************************************#
#           guvcview              http://guvcview.sourceforge.net               #
#                                                                               #
#           Paulo Assis <pj.assis@gmail.com>                                    #
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
#include <inttypes.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <png.h>

#include "gviewv4l2core.h"
#include "save_image.h"
#include "colorspaces.h"

/*
 * save rgb data into png format file
 * args:
 *    filename - string with filename
 *    width - image width (in pixels)
 *    height - image height (in pixels)
 *    data -pointer to rgb data to save
 *
 * asserts:
 *   data is not null
 *
 * returns: error code
 */
static int save_png(const char *filename, int width, int height, uint8_t *data)
{
	/*assertions*/
	assert(data != NULL);

	int l=0;
	FILE *fp;
	png_structp png_ptr;
	png_infop info_ptr;
	png_text text_ptr[3];

	png_bytep row_pointers[height];
	/* open the file */
	fp = fopen(filename, "wb");
	if (fp == NULL)
		return (E_FILE_IO_ERR);

	/*
	 * Create and initialize the png_struct with the desired error handler
	 * functions.  If you want to use the default stderr and longjump method,
	 * you can supply NULL for the last three parameters.  We also check that
	 * the library version is compatible with the one used at compile time,
	 * in case we are using dynamically linked libraries.
	 */
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
		NULL, NULL, NULL);

	if (png_ptr == NULL)
	{
		fclose(fp);
		return (E_ALLOC_ERR);
	}

	/* Allocate/initialize the image information data. */
	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL)
	{
		fclose(fp);
		png_destroy_write_struct(&png_ptr, NULL);
		return (E_ALLOC_ERR);
	}

	/* Set error handling.  REQUIRED if you aren't supplying your own
	* error handling functions in the png_create_write_struct() call.
	*/
	if (setjmp(png_jmpbuf(png_ptr)))
	{
		fclose(fp);
		png_destroy_write_struct(&png_ptr, &info_ptr);
		return (E_ALLOC_ERR);
	}

	/* set up the output control using standard C streams */
	png_init_io(png_ptr, fp);

	/*
	 * turn on or off filtering, and/or choose
	 * specific filters.  You can use either a single
	 * PNG_FILTER_VALUE_NAME or the bitwise OR of one
	 * or more PNG_FILTER_NAME masks.
	 */
	//png_set_filter(png_ptr, 0,
	//	PNG_FILTER_NONE  | PNG_FILTER_VALUE_NONE |
	//	PNG_FILTER_SUB   | PNG_FILTER_VALUE_SUB  |
	//	PNG_FILTER_UP    | PNG_FILTER_VALUE_UP   |
	//	PNG_FILTER_AVE   | PNG_FILTER_VALUE_AVE  |
	//	PNG_FILTER_PAETH | PNG_FILTER_VALUE_PAETH|
	//	PNG_ALL_FILTERS);

	/* set the zlib compression level */
	//png_set_compression_level(png_ptr,
	//	Z_BEST_COMPRESSION);

	/* set other zlib parameters */
	//png_set_compression_mem_level(png_ptr, 8);
	//png_set_compression_strategy(png_ptr,
	//	Z_DEFAULT_STRATEGY);
	//png_set_compression_window_bits(png_ptr, 15);
	//png_set_compression_method(png_ptr, 8);
	//png_set_compression_buffer_size(png_ptr, 8192);

	png_set_IHDR(png_ptr, info_ptr, width, height,
		8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

	/*
	 * Optional gamma chunk is strongly suggested if you have any guess
	 * as to the correct gamma of the image.
	 */
	//png_set_gAMA(png_ptr, info_ptr, gamma);

	/* Optionally write comments into the image */
	text_ptr[0].key = "Title";
	text_ptr[0].text = (char *) filename;
	text_ptr[0].compression = PNG_TEXT_COMPRESSION_NONE;
	text_ptr[1].key = "Software";
	text_ptr[1].text = "guvcview";
	text_ptr[1].compression = PNG_TEXT_COMPRESSION_NONE;
	text_ptr[2].key = "Description";
	text_ptr[2].text = "File generated by guvcview <http://guvcview.sourceforge.net>";
	text_ptr[2].compression = PNG_TEXT_COMPRESSION_NONE;
#ifdef PNG_iTXt_SUPPORTED
	text_ptr[0].lang = NULL;
	text_ptr[1].lang = NULL;
	text_ptr[2].lang = NULL;
#endif
	png_set_text(png_ptr, info_ptr, text_ptr, 3);

	/* Write the file header information.  REQUIRED */
	png_write_info(png_ptr, info_ptr);

	/* flip BGR pixels to RGB */
	//png_set_bgr(png_ptr); /*?no longuer required?*/

	/* Write the image data.*/
	for (l = 0; l < height; l++)
		row_pointers[l] = data + l * width * 3;

	png_write_image(png_ptr, row_pointers);

	/*
	 * You can write optional chunks like tEXt, zTXt, and tIME at the end
	 * as well.  Shouldn't be necessary in 1.1.0 and up as all the public
	 * chunks are supported and you can use png_set_unknown_chunks() to
	 * register unknown chunks into the info structure to be written out.
	 */

	/* It is REQUIRED to call this to finish writing the rest of the file */
	png_write_end(png_ptr, info_ptr);

	/*
	 * If you png_malloced a palette, free it here
	 * (don't free info_ptr->palette, as recommended in versions 1.0.5m
	 * and earlier of this example;
	 * if libpng mallocs info_ptr->palette, libpng will free it).
	 * If you allocated it with malloc() instead of png_malloc(),
	 * use free() instead of png_free().
	 */
	//png_free(png_ptr, palette);
	//palette=NULL;

	/*
	 * Similarly, if you png_malloced any data that you passed in with
	 * png_set_something(), such as a hist or trans array, free it here,
	 * when you can be sure that libpng is through with it.
	 */
	//png_free(png_ptr, trans);
	//trans=NULL;

	/* clean up after the write, and free any memory allocated */
	png_destroy_write_struct(&png_ptr, &info_ptr);

	/* close the file */
	fflush(fp); //flush data stream to file system
	if(fsync(fileno(fp)) || fclose(fp))
	{
		fprintf(stderr, "V4L2_CORE: (save png) couldn't write to file: %s\n", strerror(errno));
		return(E_FILE_IO_ERR);
	}

	/* that's it */
	return (E_OK);
}

/*
 * save frame data into a png file
 * args:
 *    frame - pointer to frame buffer
 *    filename - string with png filename name
 *
 * asserts:
 *   none
 *
 * returns: error code
 */
int save_image_png(v4l2_frame_buff_t *frame, const char *filename)
{
	int width = frame->width;
	int height = frame->height;

	uint8_t *rgb = calloc( width * height * 3, sizeof(uint8_t));
	if(rgb == NULL)
	{
		fprintf(stderr, "V4L2_CORE: FATAL memory allocation failure (save_image_png): %s\n", strerror(errno));
		exit(-1);
	}

	yu12_to_rgb24(rgb, frame->yuv_frame, width, height);

	int ret = save_png(filename, width, height, rgb);

	free(rgb);

	return ret;
}
