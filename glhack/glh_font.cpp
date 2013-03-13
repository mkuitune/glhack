/**\file glh_font.cpp
    \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/
#pragma once

#include "glh_font.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#undef STB_TRUETYPE_IMPLEMENTATION

namespace glh{

void print_string_to_image(Image8& image, vec2i position, Font& font, std::string& str)
{

}

}
