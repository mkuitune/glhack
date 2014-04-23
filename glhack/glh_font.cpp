/**\file glh_font.cpp
    \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/
#pragma once

#include "glsystem.h"
#include "glh_font.h"
#include "shims_and_types.h"
#include "iotools.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#undef STB_TRUETYPE_IMPLEMENTATION

#define ASCII_CHARNUM 96
#define ASCII_CHARMIN 32
#define ASCII_CHARMAX 128

namespace glh{

///////////////////// FontContext /////////////////////

struct FontCharData{
    stbtt_bakedchar cdata[96]; // ASCII 32..126 is 95 glyphs
};

FontContext::FontContext(const std::string& font_directory)
    :font_directory_(font_directory){
}

std::list<std::string> FontContext::list_fonts() const{
    // List directory contents, find all .ttf files
    //static_assert(false, "TODO implement");
    assert(!"TODO implement");
    return std::list<std::string>();
}

BakedFontHandle FontContext::render_bitmap(
        std::string& fontname,
        double       font_size, 
        int          texture_size)
{
    std::string              path(font_directory_);
    auto fontfile          = path_join(path, fontname);  
    BakedFontHandle handle = std::make_pair(fontname, font_size);
    
    std::vector<uint8_t> fontdata;
    bool              data_found;
    
    std::tie(fontdata, data_found) = file_to_bytes(fontfile.c_str());

    if(data_found)
    {
        bitmaps_.emplace(handle, Image8(texture_size, texture_size, 1));
        chardata_.emplace(handle, new FontCharData());

        const uint8_t *data = &fontdata[0];
        int offset         = 0;
        float pixel_height = to_float(font_size);
        uint8_t *pixels    = bitmaps_[handle].data();
        int pw             = texture_size;
        int ph             = texture_size;
        int first_char     = 32;
        int num_chars      = 96;

        stbtt_bakedchar* chardata = chardata_[handle]->cdata;

        stbtt_BakeFontBitmap(data, offset, pixel_height, pixels,pw,ph, first_char, num_chars, chardata);

    }
    else throw GraphicsException(std::string("render_bitmap_from_file: Fontfile ") + fontfile + std::string(" not found."));

    return handle;
}

BakedFontHandle FontContext::render_bitmap(const FontConfig& config)
{
    std::string              path(font_directory_);
    auto fontfile          = path_join(path, config.name_);  
    BakedFontHandle handle = std::make_pair(config.name_, config.glyph_size_);
    
    if(chardata_.find(handle) != chardata_.end()){
        return handle;
    }

    // If data for handle not found, render it.

    std::vector<uint8_t> fontdata;
    bool              data_found;
    
    std::tie(fontdata, data_found) = file_to_bytes(fontfile.c_str());

    // TODO FIXME: Check if handle already exists. If it does, 
    if(data_found)
    {

        const int pw = config.texture_width_;
        const int ph = config.texture_height_;

        bitmaps_.emplace(handle, Image8(pw, ph, 1));
        chardata_.emplace(handle, new FontCharData());

        const uint8_t *data = &fontdata[0];
        int offset         = 0;
        float pixel_height = to_float(config.glyph_size_);
        uint8_t *pixels    = bitmaps_[handle].data();

        int first_char     = 32;
        int num_chars      = 96;

        stbtt_bakedchar* chardata = chardata_[handle]->cdata;

        stbtt_BakeFontBitmap(data, offset, pixel_height, pixels, pw,ph, first_char, num_chars, chardata);

    }
    else throw GraphicsException(std::string("render_bitmap_from_file: Fontfile ") + fontfile + std::string(" not found."));

    return handle;
}

Image8* FontContext::get_font_map(const BakedFontHandle& handle) {
    return &bitmaps_[std::make_pair(handle.first, handle.second)];
}

FontCharData* FontContext::get_font_char_data(const BakedFontHandle& handle) {
    return chardata_[handle];
}

std::tuple<float, float> FontContext::write_pixel_coords_for_string(const std::string& string,
                                          const BakedFontHandle& handle, 
                                          const float x,
                                          const float y,
                                          const float max_x,
                                          GlyphCoords::row_coords_t& coords)
{
    Image8*       charimg  = get_font_map(handle);
    FontCharData* chardata = get_font_char_data(handle);

    int w = charimg->width_;
    int h = charimg->height_;

    const char* text = string.c_str();

    // Glyph placement
    float glyph_placement_xx = x;
    float glyph_placement_yy = y;
    int i = 0;
    while (*text) {
      if(*text >= ASCII_CHARMIN && *text < ASCII_CHARMAX){
         stbtt_aligned_quad q;
         float xx_in = glyph_placement_xx;
         float yy_in = glyph_placement_yy;
         stbtt_GetBakedQuad(chardata->cdata, w, h, *text - ASCII_CHARMIN, &xx_in, &yy_in, &q,1);//1=opengl,0=old d3d

         vec2 v0(q.x0,q.y0); vec2 t0(q.s0,q.t0);
         vec2 v1(q.x1,q.y0); vec2 t1(q.s1,q.t0);
         vec2 v2(q.x1,q.y1); vec2 t2(q.s1,q.t1);
         vec2 v3(q.x0, q.y1); vec2 t3(q.s0, q.t1);

         if(q.x0 < max_x && q.x1 < max_x){
            coords.emplace_back(textured_quad2d_t{{v1, v2, v3, v0, v1, v3}, 
                                               {t1, t2, t3, t0, t1, t3}}, i);
            glyph_placement_xx = xx_in;
            glyph_placement_yy = yy_in;
         }
         else
         {
             break;
         }
      }
      ++text;
      ++i;
    }
    return std::make_tuple(glyph_placement_xx, glyph_placement_yy);
}

void FontContext::write_pixel_coords_for_string(const std::string& string,
    const BakedFontHandle& handle,
    float x,
    float y,
    const Box2& bounds,
    GlyphCoords::row_coords_t& row_coords)
{
    Image8*       charimg = get_font_map(handle);
    FontCharData* chardata = get_font_char_data(handle);

    int w = charimg->width_;
    int h = charimg->height_;

    int size = (int) string.size();
    const char* c = string.c_str();
    char str = ' ';
    if(size == 0)
    {
        c = &str;
        size = 1;
    }

    for(int i = 0; i < size; ++i) {
        if(*c >= ASCII_CHARMIN && *c < ASCII_CHARMAX){
            stbtt_aligned_quad q;
            stbtt_GetBakedQuad(chardata->cdata, w, h, *c - ASCII_CHARMIN, &x, &y, &q, 1);//1=opengl,0=old d3d

            vec2 v0(q.x0, q.y0); vec2 t0(q.s0, q.t0);
            vec2 v1(q.x1, q.y0); vec2 t1(q.s1, q.t0);
            vec2 v2(q.x1, q.y1); vec2 t2(q.s1, q.t1);
            vec2 v3(q.x0, q.y1); vec2 t3(q.s0, q.t1);

            Box2 coverage = cover_points<float, 2>({v0, v1, v2, v3});

            if(inside(bounds, coverage)){
                row_coords.emplace_back(textured_quad2d_t{{v1, v2, v3, v0, v1, v3},
                                                      {t1, t2, t3, t0, t1, t3}}, i);
            }
            c++;
        }
    }
}

}// namespace glh
