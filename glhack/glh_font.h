/**\file glh_font.h
    \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/
#pragma once

#include "glh_image.h"
#include "math_tools.h"
#include<map>
#include<vector>

namespace glh{


typedef std::pair<std::string, double> BakedFontHandle;

inline BakedFontHandle invalid_font_handle(){return std::make_pair(std::string(""), 0.0);}
inline bool handle_valid(BakedFontHandle& h){return h.first != "" && h.second > 0.;}

struct FontCharData;

/** Configures one face size of one font kind. */
struct FontConfig{
    double      glyph_size_; //> Size in pixels.
    std::string name_;
    int         texture_width_;
    int         texture_height_;
};

/** Contains font rendering related state and data. */
class FontContext {
public:

    FontContext(const std::string& font_directory);

    std::list<std::string> list_fonts() const;

    BakedFontHandle render_bitmap(std::string& fontname, double font_size, int texture_size);
    BakedFontHandle render_bitmap(const FontConfig& config);

    Image8*         get_font_map(const BakedFontHandle& handle);
    FontCharData*   get_font_char_data(const BakedFontHandle& handle);

    /** Write pixel coords for given string. Scale to display units outside of this.
    *   Assume orthographic projection with units = screen pixels, origin at top left.
    *   \param  string String to encode
    *   \param  handel Font handle to used.
    *   \param  coords Output coordinates as location, texture-coordinate pairs.
    *   \return The final x,y coordinates of the quads laid out.
    */
    std::tuple<float, float> write_pixel_coords_for_string(const std::string& string,
                                                           const BakedFontHandle& handle,
                                                           const float x, const float y,
                                                           std::vector<std::tuple<vec2, vec2>>& coords);

private:

    const std::string font_directory_;

    std::map<BakedFontHandle, Image8>        bitmaps_;
    std::map<BakedFontHandle, FontCharData*> chardata_;

};

}
