/** 

*/

#include "glh_app_services.h"

#include <cctype>
#include <vector>

//////////////// Program stuff ////////////////

//const char* sh_vertex   = 
//"#version 150               \n"
//"in vec3 VertexPosition;    "
//"in vec3 VertexColor;       "
//"out vec3 Color;            "
//"void main()                "
//"{                          "
//"    Color = VertexColor;   "
//"    gl_Position = vec4( VertexPosition, 1.0 );"
//"}";
//
//const char* sh_vertex_obj   = 
//"#version 150               \n"
//"uniform mat4 ObjectToScreen;"
//"in vec3      VertexPosition;    "
//"in vec3      VertexColor;       "
//"out vec3 Color;            "
//"void main()                "
//"{                          "
//"    Color = VertexColor;   "
//"    gl_Position = ObjectToScreen * vec4( VertexPosition, 1.0 );"
//"}";


const char* sh_vertex_obj =
"#version 150               \n"
"uniform mat4 LocalToWorld;"
"uniform mat4 WorldToScreen;"
"in vec3      VertexPosition;"
"in vec3      VertexColor;   "
"out vec3     v_color;       "
"void main()                "
"{                          "
"    v_color = VertexColor;"
"    gl_Position = WorldToScreen * LocalToWorld * vec4( VertexPosition, 1.0 );"
"}";

const char* sh_fragment_vertex_color =
"#version 150                  \n"
"uniform vec4 ColorAlbedo;"
"in vec3 v_color;               "
"out vec4 FragColor;           "
"void main() {                 "
"    FragColor = ColorAlbedo;   "
"}";

// Font material

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
"    /* mat4 override = LocalToWorld;*/"
"    /* override[3][0] = 500.f;*/"
"    /* override[3][1] = 500.f;*/"
"    gl_Position = WorldToScreen * LocalToWorld * vec4( VertexPosition, 1.0 );"
"    /*gl_Position = WorldToScreen * override * vec4( VertexPosition, 1.0 );*/"
"}";


const char* sh_fragment_tex_alpha = 
"#version 150                  \n"
"uniform sampler2D Sampler;    "
"uniform vec4      ColorAlbedo;    "
"in vec2 v_texcoord;           "
"out vec4 FragColor;           "
"void main() {                 "
"    vec4 texColor = vec4(ColorAlbedo.rgb, ColorAlbedo.a * texture( Sampler, v_texcoord ).r);"
"    FragColor = texColor;"
//"    FragColor = vec4(Color, 1.0); "
"}";

const char* sh_geometry = "";

glh::ProgramHandle* sp_vcolor_handle;
glh::ProgramHandle* sp_tex_handle;
glh::ProgramHandle* sp_colorcoded;

// App state

bool g_run = true;

// Screen quad shader
const char* font_program_name = "screen_obj_font";
const char* object_colored_program_name = "screen_obj_colored";

float tprev = 0;
float angle = 0.f;
float radial_speed = 0.0f * PIf;

glh::AppServices  services;
glh::RenderQueue* render_queue;
glh::RenderPass*  render_pass;

glh::Camera*      camera;

glh::GlyphPane* glyph_pane;

void load_font_resources(glh::GraphicsManager* gm)
{
    using namespace glh;

    std::string old_goudy("OFLGoudyStMTT.ttf");
    std::string junction("Junction-webfont.ttf");

    float fontsize = 15.0f;

    glyph_pane = services.assets().create_glyph_pane(font_program_name, object_colored_program_name);  // TODO: font program should be automatically handled by scene assets I think
    vec3 glyph_pane_loc(500.f, 500.f, 0.f); // TODO to glyph pane loc node
    glyph_pane->root()->transform_.position_ = glyph_pane_loc;

    glyph_pane->set_font(junction, fontsize);

    glyph_pane->text_field_.push_line("Hello, world.");
    glyph_pane->text_field_.push_line("This is another line, then.");
    glyph_pane->update_representation();
    glyph_pane->dirty_ = true;

    glyph_pane->cursor_node_->material_.set_vec4(GLH_COLOR_ALBEDO, glh::vec4(0.f, 0.f, 0.f, 1.f));
    glyph_pane->background_mesh_node_->material_.set_vec4(GLH_COLOR_ALBEDO, glh::vec4(0.5f, 0.51f, 0.51f, 1.f));

    // TODO: View layouter that aligns glyph_pane
}

bool init(glh::App* app)
{
    const char* config_file = "config.mp";

    services.init(app, config_file);

    glh::GraphicsManager* gm = app->graphics_manager();

    gm->create_program(font_program_name, sh_geometry, sh_vertex_obj_tex, sh_fragment_tex_alpha);
    gm->create_program(object_colored_program_name, sh_geometry, sh_vertex_obj, sh_fragment_vertex_color);

    load_font_resources(gm);

    render_pass = services.assets_->create_render_pass();
    render_queue = &render_pass->queue_;

    // TODO: Default mat
    render_pass->env_.set_vec4(GLH_COLOR_ALBEDO, glh::vec4(0.28f, 0.024f, 0.024f, 1.0));
    render_pass->env_.set_texture2d("Sampler", glyph_pane->fonttexture_);// todo: then env.set "Sampler" texture into glyph_pane TODO move this into GlyphPane

    render_pass->add_settings(glh::RenderPassSettings(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT,
        glh::Color(0.5f, 0.5f, 0.5f, 1.0f), 1));
    render_pass->add_settings(glh::RenderPassSettings(glh::RenderPassSettings::BlendSettings(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)));

    camera = services.assets_->create_camera(glh::Camera::Projection::PixelSpaceOrthographic);

    render_pass->set_camera(camera);

    return true;
}

Eigen::Affine3f rotating_transform(glh::App* app){
    float t = (float) app->time();
    angle += (t - tprev) * radial_speed;
    tprev = t;
    Eigen::Affine3f transform;
    transform = Eigen::AngleAxis<float>(angle, glh::vec3(0.f, 0.f, 1.f));
    return transform;
}

bool update(glh::App* app)
{
    using namespace glh;

    services.update();

    rotating_transform(app);

    mat4 screen_to_view = app_orthographic_pixel_projection(app); 

    float albedo_alpha = 1.0f;
    render_pass->env_.set_vec4(GLH_COLOR_ALBEDO, glh::vec4(0.28f, 0.024f, 0.024f, albedo_alpha));

    glyph_pane->glyph_node_->material_.set_vec4(GLH_COLOR_ALBEDO, glh::vec4(1.f, 1.f, 1.f, 1.f));

    float cursor_albedo = abs(sin(5.f * app->time()));
    glyph_pane->cursor_node_->material_.set_vec4(GLH_COLOR_ALBEDO, glh::vec4(0.f, 0.f, 0.f, cursor_albedo));

    //Math<float>::quaternion_t rot(1.f, 0.f, 0.f, 0.f);
    //rot.z() = 0.01f;
    //glyph_pane->pane_root_->transform_.rotation_ = rot * glyph_pane->pane_root_->transform_.rotation_;
    //glyph_pane->pane_root_->transform_.rotation_.normalize();

    render_pass->set_queue_filter(glh::pass_all);
    render_pass->update_queue_filtered(services.assets_->tree_);

    return g_run;
}

void render_font(glh::App* app)
{
    // TODO FIX renderpass settings, non-obvious
    glh::GraphicsManager* gm = app->graphics_manager();

    render_pass->render(gm);
}

void resize(glh::App* app, int width, int height)
{
    std::cout << "Resize:" << width << " " << height << std::endl;
}

int g_mouse_x;
int g_mouse_y;

void mouse_move_callback(int x, int y)
{
    g_mouse_x = x;
    g_mouse_y = y;

    std::cout << "Mouse:"  << x << " " << y << std::endl;
}

glh::Modifiers modifiers;

void key_callback(int key, const glh::Input::ButtonState& s)
{
    using namespace glh;

    if(key == Input::Esc) { g_run = false;}
    if(key == Input::Lshift || key == Input::Rshift){
        if(s == glh::Input::Held)
            modifiers.shift_ = true;
        else modifiers.shift_ = false;
    }
    else if(key == Input::Left){ radial_speed -= 0.1f * PIf;}
    else if(key == Input::Right){ radial_speed += 0.1f * PIf;}
    else if(s == Input::Held){
        glyph_pane->recieve_characters(key, modifiers);
    }
}

int main(int arch, char* argv)
{
    glh::AppConfig config = glh::app_config_default(init, update, render_font, resize);

    config.width = 1024;
    config.height = 640;

    glh::App app(config);

    GLH_LOG_EXPR("Logger started");
    add_key_callback(app, key_callback);
    add_mouse_move_callback(app, mouse_move_callback);
    glh::default_main(app);
    services.finalize();
    return 0;
}
