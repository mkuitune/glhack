/**\file glh_font.h
    \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/
#pragma once

#include "glh_image.h"
#include "math_tools.h"
#include<map>
#include<vector>
#include<array>

namespace glh{

struct GlyphCoords{
    typedef std::array<std::tuple<vec2, vec2>, 4> glyph_quad_t;
    typedef std::vector<std::tuple<vec2, vec2>>   row_coords_t;
    typedef std::vector<row_coords_t>             row_coords_container_t;

    row_coords_container_t coords_;

    size_t size(){ return coords_.size(); }

    void clear(){ coords_.clear(); }

    bool empty(){ return coords_.empty() || (coords_.size() == 1 && coords_[0].empty()); }

    row_coords_t& last(){ return coords_.back(); }

    int row_len(int row_index){
        if(in_range_inclusive(row_index, 0, ((int) size()) - 1)){
            return coords_[row_index].size();
        }
        else return 0;
    }

    // TODO remove temp method to port existing code to new struct
    std::tuple<vec2, vec2> last_pos(){
        return coords_.back()[coords_.back().size() - 2]; }

    std::tuple<vec2, vec2> pos(int j, int i){


        return coords_[i][j * 4 - 2];
    }

    row_coords_t* push_row(){
        coords_.emplace_back();
        return &coords_.back();
    }

    //int height(){ return (int) coords_.size();}

};

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

/** Approximate required texture size based on the requested glyph_size_. Tweak as necessary */
inline FontConfig get_font_config(std::string name, double glyph_size){
    FontConfig f;
    f.name_ = name;
    f.glyph_size_ = glyph_size;
    f.texture_width_ = 1024;
    f.texture_height_ = 256;
    return f;
}

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
                                                           GlyphCoords::row_coords_t& coords);

private:

    const std::string font_directory_;

    std::map<BakedFontHandle, Image8>        bitmaps_;
    std::map<BakedFontHandle, FontCharData*> chardata_;

};

}
