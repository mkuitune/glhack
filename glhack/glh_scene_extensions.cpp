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

    fontmesh->get(glh::ChannelType::Position).set(&posdata[0], posdata.size());
    fontmesh->get(glh::ChannelType::Texture).set(&texdata[0], texdata.size());
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


}
