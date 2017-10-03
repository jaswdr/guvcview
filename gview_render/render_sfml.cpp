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

#include "render_sfml.hpp"
#include <iostream>
#include <cstring>

extern "C" {
#include "gview.h"
#include "gviewrender.h"
#include "render.h"
#include "render_sfml.h"
#include "../config.h"
}

extern int verbosity;

/*
 * yu12 to rgba (rgb32)
 * args:
 *    out - pointer to output rgba data buffer
 *    in - pointer to input yu12 data buffer
 *    width - buffer width (in pixels)
 *    height - buffer height (in pixels)
 *
 * asserts:
 *    none
 *
 * returns: none
 */
static void yu12_to_rgba (uint8_t *out, uint8_t *in, int width, int height)
{
	uint8_t *py1 = in; //line 1
	uint8_t *py2 = py1 + width; //line 2
	uint8_t *pu = in + (width * height);
	uint8_t *pv = pu + ((width * height) / 4);
	
	uint8_t *pout1 = out; //first line
	uint8_t *pout2 = out + (width * 4); //second line
	
	int h=0, w=0;
	
	for(h=0; h < height; h+=2) //every two lines
	{
		py1 = in + (h * width);
		py2 = py1 + width;
		
		pout1 = out + (h * width * 4);
		pout2 = pout1 + (width * 4);
		
		for(w=0; w<width; w+=2) //every 2 pixels
		{
			/* standart: r = y0 + 1.402 (v-128) */
			/* logitech: r = y0 + 1.370705 (v-128) */
			*pout1++=CLIP(*py1 + 1.402 * (*pv-128));
			*pout2++=CLIP(*py2 + 1.402 * (*pv-128));	
			/* standart: g = y0 - 0.34414 (u-128) - 0.71414 (v-128)*/
			/* logitech: g = y0 - 0.337633 (u-128)- 0.698001 (v-128)*/
			*pout1++=CLIP(*py1 - 0.34414 * (*pu-128) -0.71414*(*pv-128));
			*pout2++=CLIP(*py2 - 0.34414 * (*pu-128) -0.71414*(*pv-128));
			/* standart: b = y0 + 1.772 (u-128) */
			/* logitech: b = y0 + 1.732446 (u-128) */
			*pout1++=CLIP(*py1 + 1.772 *( *pu-128));
			*pout2++=CLIP(*py2 + 1.772 *( *pu-128));
			/* alpha set to 255*/
			*pout1++=255;
			*pout2++=255;

			py1++;
			py2++;
			
			/* standart: r1 =y1 + 1.402 (v-128) */
			/* logitech: r1 = y1 + 1.370705 (v-128) */
			*pout1++=CLIP(*py1 + 1.402 * (*pv-128));
			*pout2++=CLIP(*py2 + 1.402 * (*pv-128));
			/* standart: g1 = y1 - 0.34414 (u-128) - 0.71414 (v-128)*/
			/* logitech: g1 = y1 - 0.337633 (u-128)- 0.698001 (v-128)*/
			*pout1++=CLIP(*py1 - 0.34414 * (*pu-128) -0.71414 * (*pv-128));
			*pout2++=CLIP(*py2 - 0.34414 * (*pu-128) -0.71414 * (*pv-128));
			/* standart: b1 = y1 + 1.772 (u-128) */
			/* logitech: b1 = y1 + 1.732446 (u-128) */
			*pout1++=CLIP(*py1 + 1.772 * (*pu-128));
			*pout2++=CLIP(*py2 + 1.772 * (*pu-128));
			/* alpha set to 255*/
			*pout1++=255;
			*pout2++=255;

			py1++;
			py2++;
			pu++;
			pv++;
		}
	}
}

SFMLRender::SFMLRender(int width, int height, int flags)
{
	int w = width;
	int h = height;

	use_shader = true;

	pix = NULL;

	//get the current resolution
	sf::VideoMode display_mode = sf::VideoMode::getDesktopMode();

	if(verbosity > 0)
		std::cout << "RENDER: (SFML) desktop resolution (" <<
			display_mode.width << "x" << 
			display_mode.height << ")" << std::endl;

	if((display_mode.width > 0) && (display_mode.height > 0))
	{
		if(w > display_mode.width)
			w = display_mode.width;
		if(h > display_mode.height)
			h = display_mode.height;
	}

	// Create the main window
	if(flags == 1) //fullscreen
	{
		window.create(sf::VideoMode(w, h), "SFML window", sf::Style::Fullscreen);
		if(!window.isOpen())
		{
			std::cerr << "RENDER: (SFML) couldn't open window in fullscreen mode" << std::endl;
			window.create(sf::VideoMode(w, h), "SFML window");
		}
	}
	else
		window.create(sf::VideoMode(w, h), "SFML window");

	if(!window.isOpen())
	{
		std::cerr << "RENDER: (SFML) couldn't open window" << std::endl;
		return;
	}

	if(!texture.create(width, height))
	{
		std::cerr << "RENDER: (SFML) couldn't create texture" << std::endl;
		return;
	}

	if (sf::Shader::isAvailable() )
	{
		const std::string fragmentShader = \
			"uniform sampler2D texY;" \
			"uniform sampler2D texU;" \
			"uniform sampler2D texV;" \
			"void main(void)" \
			"{" \
				"float nx, ny;" \
				"vec3 yuv;" \
				"vec3 rgb;" \
				"vec2 norm_pos = vec2( gl_TexCoord[0].x,gl_TexCoord[0].y);" \
				"yuv.x = texture2D(texY, norm_pos).r;" \
				"yuv.y = texture2D(texU, norm_pos).r - 0.5;" \
				"yuv.z = texture2D(texV, norm_pos).r - 0.5;" \
				"rgb = mat3(      1,       1,       1," \
				"                 0, -.21482, 2.12798," \
				"           1.28033, -.38059,       0) * yuv;" \
				"gl_FragColor = vec4(rgb, 1.0);" \
			"}";

		if(!conv_yuv2rgb_shd.loadFromMemory(fragmentShader, sf::Shader::Fragment))
		{
			std::cerr << "RENDER: (SFML) couldn't load fragment shader for yuv conversion" << std::endl;
			use_shader = false;
		}
		else
		{
			if(!texY.create(width, height))
			{
				std::cerr << "RENDER: (SFML) couldn't create texture" << std::endl;
				return;
			}

			if(!texU.create(width/2, height/2))
			{
				std::cerr << "RENDER: (SFML) couldn't create texture" << std::endl;
				return;
			}

			if(!texV.create(width/2, height/2))
			{
				std::cerr << "RENDER: (SFML) couldn't create texture" << std::endl;
				return;
			}

#if LIBSFML_VER_AT_LEAST(2,4)
			conv_yuv2rgb_shd.setUniform("texY", texY);
			conv_yuv2rgb_shd.setUniform("texU", texU);
			conv_yuv2rgb_shd.setUniform("texV", texV);
#else
                        conv_yuv2rgb_shd.setParameter("texY", texY);
			conv_yuv2rgb_shd.setParameter("texU", texU);
			conv_yuv2rgb_shd.setParameter("texV", texV);
#endif
		}
	}
	else
	{
		std::cerr << "RENDER: (SFML) shader not available for yuv conversion" << std::endl;
		use_shader = false;
	}

	//use pix buffer for rgba conversion
	if(!use_shader)
		pix = (uint8_t *) calloc(width*height*4, sizeof(uint8_t));

	sprite.setTexture(texture);

	texture.setSmooth(true);

	window.clear(sf::Color::Black);
	window.display();

	if(verbosity > 1)
		std::cout << "RENDER: (SFML) window created" << std::endl;
}

SFMLRender::~SFMLRender()
{

	if(pix)
		free(pix);

	window.close();
}

int SFMLRender::render_frame(uint8_t *frame, int width, int height)
{
	//convert yuv to rgba
	
	if (use_shader)
	{
		uint8_t *pu = frame + (width*height);
		uint8_t *pv = pu + (width*height)/4;
		
		GLint textureBinding;
		// Save the current texture binding, to avoid messing up SFML's OpenGL states
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &textureBinding);

		// :Y
		sf::Texture::bind(&texY);
		
		glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, frame);

		// :U
		sf::Texture::bind(&texU);
		
		glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width/2, height/2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, pu);

		// :V
		sf::Texture::bind(&texV);
		
		glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width/2, height/2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, pv);

		// Restore the previous texture binding
		glBindTexture(GL_TEXTURE_2D, textureBinding);

		//draw frame
		window.draw(sprite, &conv_yuv2rgb_shd);
	}
	else
	{
		yu12_to_rgba ((uint8_t *) pix, frame, width, height);
		//update texture
		texture.update(pix);
		//draw frame
		window.draw(sprite);
	}
	
	window.display();

	return 0;
}

void SFMLRender::set_caption(const char* caption)
{
	window.setTitle(std::string(caption, 0, 30));
}

void SFMLRender::dispatch_events()
{
	// Process events
	sf::Event event;
	while (window.pollEvent(event))
	{
		if (event.type == sf::Event::KeyPressed)
		{
			switch( event.key.code )
			{
				case sf::Keyboard::Escape:
					render_call_event_callback(EV_QUIT);
					break;
				case sf::Keyboard::Up:
					render_call_event_callback(EV_KEY_UP);
					break;

				case sf::Keyboard::Down:
					render_call_event_callback(EV_KEY_DOWN);
					break;

				case sf::Keyboard::Right:
					render_call_event_callback(EV_KEY_RIGHT);
					break;

				case sf::Keyboard::Left:
					render_call_event_callback(EV_KEY_LEFT);
					break;

				case sf::Keyboard::Space:
					render_call_event_callback(EV_KEY_SPACE);
					break;

				case sf::Keyboard::I:
					render_call_event_callback(EV_KEY_I);
					break;

				case sf::Keyboard::V:
					render_call_event_callback(EV_KEY_V);
					break;

				default:
					break;
			}
		}

		// Close window: exit
		if (event.type == sf::Event::Closed)
		{
			if(verbosity > 0)
				std::cout<< "RENDER:(SFML) (event) close\n"<< std::endl;
			render_call_event_callback(EV_QUIT);
		}
	}
}


/******************************* C wrapper functions ********************************/
SFMLRender* render = NULL;

/*
 * init sfml render
 * args:
 *    width - overlay width
 *    height - overlay height
 *    flags - window flags:
 *              0- none
 *              1- fullscreen
 *              2- maximized
 *
 * asserts:
 *
 * returns: error code (0 ok)
 */
 int init_render_sfml(int width, int height, int flags)
 {
	//clean old render
	if(render)
		delete(render);
		
	render = new SFMLRender(width, height, flags);

	if(!render)
	{
		std::cerr << "RENDER: (SFML) couldn't create render" << std::endl;
		return -1;
	}
	
	if(!render->has_window())
	{
		std::cerr << "RENDER: (SFML) couldn't create render window" << std::endl;
		return -2;
	}
	
	return 0;
 }

/*
 * render a frame
 * args:
 *   frame - pointer to frame data (yuyv format)
 *   width - frame width
 *   height - frame height
 *
 * asserts:
 *   poverlay is not nul
 *   frame is not null
 *
 * returns: error code
 */
int render_sfml_frame(uint8_t *frame, int width, int height)
{
	return render->render_frame(frame, width, height);
}

/*
 * set sfml render caption
 * args:
 *   caption - string with render window caption
 *
 * asserts:
 *   none
 *
 * returns: none
 */
void set_render_sfml_caption(const char* caption)
{
	render->set_caption(caption);
}

/*
 * dispatch sfml render events
 * args:
 *   none
 *
 * asserts:
 *   none
 *
 * returns: none
 */
void render_sfml_dispatch_events()
{
	render->dispatch_events();
}

/*
 * clean sfml render data
 * args:
 *   none
 *
 * asserts:
 *   none
 *
 * returns: none
 */
void render_sfml_clean()
{
	delete(render);
	render = NULL;
}

