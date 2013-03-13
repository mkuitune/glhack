/**\file glh_font.h
    \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/
#pragma once

#include "glh_image.h"
#include "math_tools.h"

namespace glh{

struct Font{
    int size;
};

void print_string_to_image(Image8& image, vec2i position, Font& font, std::string& str);

}
