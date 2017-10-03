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

#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>
#include <SFML/Config.hpp>

#define LIBSFML_VER_AT_LEAST(major,minor)  (SFML_VERSION_MAJOR > major || \
                                              (SFML_VERSION_MAJOR == major && \
                                               SFML_VERSION_MINOR >= minor))

extern "C" {
#include <stdint.h>
}

class SFMLRender {
	
	public:
		SFMLRender(int width, int height, int flags);
		~SFMLRender();
		int render_frame(uint8_t *frame, int width, int height);
		void set_caption(const char* caption);
		void dispatch_events();
		bool has_window() {return window.isOpen();};

	private:
		sf::RenderWindow window;
		sf::Texture texture;
		sf::Texture texY;
		sf::Texture texU;
		sf::Texture texV;

		sf::Sprite sprite;
		sf::Shader conv_yuv2rgb_shd;
		bool use_shader;

		uint8_t *pix;

};

#endif
