/**\file glh_scene_extensions.h
\author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/
#pragma once

#include "glh_scene_extensions.h"

namespace glh{

void transfer_position_data_to_mesh(GlyphCoords& coords,
    glh::DefaultMesh* fontmesh)
{
    using namespace glh;
    // TODO: All of these can be reserved from a arena that is filled per frame
    // (never deallocated, just held for reserve at each frame)
    // Use an arena-allocator getter at app or gm (probably app or app::resources or
    // something like that).

    GlyphCoords::row_coords_container_t& text_coords(coords.coords_);

    std::vector<float> posdata;
    std::vector<float> texdata;

    // transfer tex_coords to mesh
    for(auto& row : text_coords){
        for(textured_quad2d_t& quad : row){
            for(auto ref : quad){
                posdata.push_back(ref.pos[0]);
                posdata.push_back(ref.pos[1]);
                posdata.push_back(0.f);

                texdata.push_back(ref.tex[0]);
                texdata.push_back(ref.tex[1]);
                texdata.push_back(0.f);
            }
        }
    }

    if(posdata.size() > 0){
        fontmesh->get(ChannelType::Position).set(&posdata[0], posdata.size());
        fontmesh->get(ChannelType::Texture).set(&texdata[0], texdata.size());
    }
    else {
        float* nul = 0;
        fontmesh->get(ChannelType::Position).set(nul, 0);
        fontmesh->get(ChannelType::Texture).set(nul, 0);
    }
}

void render_glyph_coordinates_to_mesh(
    FontContext& context,
    TextField& t,
    BakedFontHandle handle,
    vec2 origin,
    double line_height /* Multiples of font height */,
    DefaultMesh& mesh){
        t.glyph_coords_.clear();
        float line_number = 0.f;
        float spacing = (float)(line_height * handle.second);

        for(auto&line : t.text_fields_){

            t.glyph_coords_.push_row();

            context.write_pixel_coords_for_string(line->string, 
                handle, origin[0], origin[1] + line_number * spacing, t.glyph_coords_.last());
            line_number += 1.0f;
        }

        transfer_position_data_to_mesh(t.glyph_coords_, &mesh);
}

void render_glyph_coordinates_to_mesh(FontContext& context, const std::string& row,
    BakedFontHandle handle, vec2 origin, DefaultMesh& mesh)
{
    GlyphCoords glyph_coords;
    glyph_coords.push_row();
    context.write_pixel_coords_for_string(row, handle, origin[0], origin[1], glyph_coords.last());
    transfer_position_data_to_mesh(glyph_coords, &mesh);
}

// TODO to text util or such.
char us_ascii_tolower(char c){

#define IFN(param, target) case param: return target; break
        switch(c){
            IFN('~', '`');
            IFN('!', '1');
            IFN('@', '2');
            IFN('#', '3');
            IFN('$', '4');
            IFN('%', '5');
            IFN('^', '6');
            IFN('&', '7');
            IFN('*', '8');
            IFN('(', '9');
            IFN(')', '0');
            IFN('_', '-');
            IFN('+', '=');

            IFN('{', '[');
            IFN('}', ']');
            IFN(':', ';');
            IFN('"', '\'');
            IFN('|', '\\');
            IFN('<', ',');
            IFN('>', '.');
            IFN('?', '/');
        default:{
            if(c > 64 && c < 91){
                return c + 32;
            } else return c;
                }
        }
#undef IFN
    }

char us_ascii_toupper(char c){

#define IFN(param, target) case target: return param; break
    switch(c){
        IFN('~', '`');
        IFN('!', '1');
        IFN('@', '2');
        IFN('#', '3');
        IFN('$', '4');
        IFN('%', '5');
        IFN('^', '6');
        IFN('&', '7');
        IFN('*', '8');
        IFN('(', '9');
        IFN(')', '0');
        IFN('_', '-');
        IFN('+', '=');

        IFN('{', '[');
        IFN('}', ']');
        IFN(':', ';');
        IFN('"', '\'');
        IFN('|', '\\');
        IFN('<', ',');
        IFN('>', '.');
        IFN('?', '/');
    default:{    
        if(c > 96 && c < 123){
            return c - 32;
        } else return c;
            }
    }
#undef IFN
}

char key_to_char(int key, Modifiers modifiers){
    char c = 0;
    if(key < CHAR_MAX && key > 0) c = static_cast<char>(key);
    if(c) c = modifiers.shift_ ? us_ascii_toupper(c) : us_ascii_tolower(c);
    return c;
}


Movement key_to_movement(int key){
    if(key == Input::Left) return Movement::Left;
    else if(key == Input::Right) return Movement::Right;
    else if(key == Input::Up) return Movement::Up;
    else if(key == Input::Down) return Movement::Down;
    else return Movement::None;
}

void GlyphPane::move_cursor(Movement m){
    if(m == Movement::Up){
        cursor_pos_index_[1]--;
    }
    else if(m == Movement::Down){
        cursor_pos_index_[1]++;
    }
    else if(m == Movement::Left){
        if(cursor_pos_index_[0] > 0){
            cursor_pos_index_[0]--;
        } else if(cursor_pos_index_[1] > 0){
            cursor_pos_index_[1]--;
            cursor_pos_index_[0] = text_field_.glyph_coords_.at(cursor_pos_index_[1]).size();
        }
    }
    else if(m == Movement::Right){
        cursor_pos_index_[0]++;
    }
    else if(m == Movement::RightBound){
        cursor_pos_index_[0] = text_field_.glyph_coords_.at(cursor_pos_index_[1]).size();;
    }
    else if(m == Movement::LeftBound){
        cursor_pos_index_[0] = 0;
    }

    update_cursor_pos();
}

void GlyphPane::recieve_characters(int key, Modifiers modifiers)
{
    // TODO: For indices that are passed onto text field, add the
    // current row - take into account a technique to scroll the text.
    
    Movement movement = key_to_movement(key);
    char c = key_to_char(key, modifiers);

    if(key == Input::Space){
        c = ' ';
    }

    if(key == Input::Tab){
        // TODO
    }

    if(key == Input::Key::Enter){
        text_field_.break_line(cursor_pos_index_[0], cursor_pos_index_[1]);
        cursor_pos_index_[1]++;
        cursor_pos_index_[0] = 0;
        dirty_ = true;
    }
    else if(movement != Movement::None){
        if(modifiers.ctrl_)
        {
            if(movement == Movement::Left) movement = Movement::LeftBound;
            if(movement == Movement::Right) movement = Movement::RightBound;
        }
        move_cursor(movement);
    }
    else if(any_of(key, Input::Del, Input::Backspace)){
        if(text_field_.size() > 0){
            if(cursor_pos_index_[0] == 0){
                if(cursor_pos_index_[1] > 0){
                    // Join lines.
                    const int prev_row = cursor_pos_index_[1] - 1;
                    const int rm_row = cursor_pos_index_[1];
                    const int new_cursor_x = text_field_.at(prev_row).size();

                    text_field_.at(prev_row).string += text_field_.at(rm_row).string;

                    text_field_.erase_line(rm_row);

                    cursor_pos_index_[1]--;
                    cursor_pos_index_[0] = new_cursor_x;
                    dirty_ = true;
                }
            }
            else if(cursor_pos_index_[0] > 0){
                text_field_.at(cursor_pos_index_[1]).string.erase(cursor_pos_index_[0] - 1, 1);
                cursor_pos_index_[0]--;
                dirty_ = true;
            }
        }
    }
    else if(c){
        text_field_.insert_char_after(cursor_pos_index_[0], cursor_pos_index_[1], c);
        cursor_pos_index_[0]++;
        dirty_ = true;
    }
}

}
