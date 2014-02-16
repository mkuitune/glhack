/**\file glh_scene_extensions.h
\author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/
#pragma once

#include "glh_scene_extensions.h"

namespace glh{

    void transfer_position_data_to_mesh(std::vector<std::tuple<glh::vec2, glh::vec2>>& text_coords,
        glh::DefaultMesh* fontmesh)
    {
        using namespace glh;
        // TODO: All of these can be reserved from a arena that is filled per frame
        // (never deallocated, just held for reserve at each frame)
        // Use an arena-allocator getter at app or gm (probably app or app::resources or
        // something like that).
        std::vector<float> posdata;
        std::vector<float> texdata;
        // transfer tex_coords to mesh
        for(auto& pt : text_coords){
            vec2 pos;
            vec2 tex;
            std::tie(pos,tex) = pt;
            posdata.push_back(pos[0]);
            posdata.push_back(pos[1]);
            posdata.push_back(0.f);

            texdata.push_back(tex[0]);
            texdata.push_back(tex[1]);
            texdata.push_back(0.f);
        }

        if(posdata.size() > 0){
            fontmesh->get(glh::ChannelType::Position).set(&posdata[0], posdata.size());
            fontmesh->get(glh::ChannelType::Texture).set(&texdata[0], texdata.size());
        }
        else {
            float* nul = 0;
            fontmesh->get(glh::ChannelType::Position).set(nul, 0);
            fontmesh->get(glh::ChannelType::Texture).set(nul, 0);
        }

    }

    void render_glyph_coordinates_to_mesh(
        FontContext& context,
        TextField& t,
        BakedFontHandle handle,
        vec2 origin,
        double line_height /* Multiples of font height */,
        DefaultMesh& mesh){

            float line_number = 0.f;
            float spacing = (float)(line_height * handle.second);
            std::vector<std::tuple<vec2, vec2>> glyph_coords;
            for(auto&line : t.text_fields_){
                context.write_pixel_coords_for_string(line->string, 
                    handle, origin[0], origin[1] + line_number * spacing, glyph_coords);
                line_number += 1.0f;
            }

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

    void GlyphPane::recieve_characters(int key, Modifiers modifiers)
    {
        char c;

        if(key == Input::Key::Enter){
            text_field_.push_line("");
        }
        else if(key == Input::Space){
            text_field_.last().push_back(' ');
        }
        else if(any_of(key, Input::Del, Input::Backspace)){
            if(text_field_.size() > 0){
                if(text_field_.last().size() > 0){ text_field_.last().erase_from_back(); }
                else if(text_field_.size() > 1){ text_field_.erase(text_field_.last()); }
            }
        }
        else if(c = key_to_char(key, modifiers)){
            text_field_.last().push_back(c);
        } 

        dirty_ = true;
    }

}
