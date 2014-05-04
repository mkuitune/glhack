/**\file glh_default_assets.h
    \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/

#include "glh_default_assets.h"
#include "glh_names.h"

#include <memory>

namespace glh{
namespace {
    const char* sh_vertex_obj =
        "#version 150               \n"
        "uniform mat4 LocalToWorld;" // TODO add world to screen
        "uniform mat4 WorldToScreen;"
        "in vec3      VertexPosition;    "
        "in vec3      VertexColor;       "
        "out vec3     v_color;       "
        "void main()                "
        "{                          "
        "    v_color = VertexColor;   "
        "    gl_Position = WorldToScreen * LocalToWorld * vec4( VertexPosition, 1.0 );"
        "}";

    const char* sh_fragment_fix_color =
        "#version 150                  \n"
        "uniform vec4 ColorAlbedo;"
        "out vec4 FragColor;           "
        "void main() {                 "
        "    FragColor = ColorAlbedo;   "
        "}";

    const char* sh_fragment_selection_color =
        "#version 150                  \n"
        "uniform vec4 " UICTX_SELECT_NAME ";"
        "out vec4 FragColor;           "
        "void main() {                 "
        "    FragColor = " UICTX_SELECT_NAME "; "
        "}";

    const char* sh_vertex_obj_tex =
        "#version 150               \n"
        "uniform mat4 LocalToWorld;"
        "uniform mat4 WorldToScreen;"
        "in vec3      VertexPosition;    "
        "in vec3      TexCoord;"
        "out vec2 v_texcoord;"
        "void main()                "
        "{                          "
        "    v_texcoord = TexCoord.xy;"
        "    gl_Position = WorldToScreen * LocalToWorld * vec4( VertexPosition, 1.0 );"
        "    /*gl_Position = WorldToScreen * override * vec4( VertexPosition, 1.0 );*/"
        "}";

    const char* sh_fragment_tex_alpha_solid_color =
        "#version 150                  \n"
        "uniform sampler2D Sampler;    "
        "uniform vec4      ColorAlbedo;    "
        "in vec2 v_texcoord;           "
        "out vec4 FragColor;           "
        "void main() {                 "
        "    vec4 texColor = vec4(ColorAlbedo.rgb, ColorAlbedo.a * texture( Sampler, v_texcoord ).r);"
        "    FragColor = texColor;"
        "}";

    const char* sh_geometry = "";

}


void load_default_programs_glsl150(GraphicsManager* gm){
    gm->create_program(GLH_COLOR_PICKER_PROGRAM, sh_geometry, sh_vertex_obj, sh_fragment_selection_color);
    gm->create_program(GLH_CONSTANT_ALBEDO_PROGRAM, sh_geometry, sh_vertex_obj, sh_fragment_fix_color);
    gm->create_program(GLH_TEXTURED_FONT_PROGRAM_SOLID_FILL, sh_geometry, sh_vertex_obj_tex, sh_fragment_tex_alpha_solid_color);
}

}
