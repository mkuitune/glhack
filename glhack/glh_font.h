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

/** 2D quadrangle encoded into two triangles. */
// TODO: Use directly two triangles instead.
typedef std::array<vec2, 6> quad2d_coord_t;

inline vec2 max(const quad2d_coord_t& coord){
    vec2 res = coord[0];
    for(auto& c : coord){ 
        res[0] = std::max(res[0], c[0]);
        res[1] = std::max(res[1], c[1]);
    }
    return res;
}

inline vec2 min(const quad2d_coord_t& coord){
    vec2 res = coord[0];
    for(auto& c : coord){
        res[0] = std::min(res[0], c[0]);
        res[1] = std::min(res[1], c[1]);
    }
    return res;
}

struct textured_quad2d_t{

    struct iterator{
        static const int end_index = 6;
        textured_quad2d_t& quad_;
        int index_;

        struct pos_ref_t{ vec2& pos; vec2& tex; };

        iterator(textured_quad2d_t& q):iterator(q, 0){}
        iterator(textured_quad2d_t& q, int index):quad_(q), index_(index){}

        void operator++(){ index_++;}
        bool operator!=(const iterator& i){ return index_ != i.index_; }
        pos_ref_t operator*(){ return{quad_.pos_[index_], quad_.tex_[index_]}; }
    };

    quad2d_coord_t pos_; 
    quad2d_coord_t tex_;

    void set_at(vec2 pos, vec2 tex, size_t index){
        pos_[index] = pos;
        tex_[index] = tex;
    }

    iterator begin(){ return iterator(*this); }
    iterator end(){ return iterator(*this, iterator::end_index); }

};

struct GlyphCoords{

    typedef std::vector<textured_quad2d_t> row_coords_t;
    typedef std::vector<row_coords_t>      row_coords_container_t;

    row_coords_container_t coords_;

    size_t size(){ return coords_.size(); }

    void clear(){ coords_.clear(); }

    bool empty(){ return coords_.empty() || (coords_.size() == 1 && coords_[0].empty()); }

    row_coords_t& last(){ return coords_.back(); }

    row_coords_t& at(int i){ return coords_.at(i); }

    int row_len(int row_index){
        if(in_range_inclusive(row_index, 0, ((int) size()) - 1)){
            return coords_[row_index].size();
        }
        else return 0;
    }

    // TODO remove temp method to port existing code to new struct
    textured_quad2d_t& last_quad(){
        return coords_.back()[coords_.back().size()]; }

    quad2d_coord_t& pos(int j, int i){
        return coords_[i][j].pos_;
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
