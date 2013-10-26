/** 


*/


#include "glbase.h"
#include "asset_manager.h"
#include "glh_generators.h"
#include "glh_image.h"
#include "glh_typedefs.h"
#include "shims_and_types.h"
#include "glh_scene_extensions.h"
#include <vector>

//////////////// Program stuff ////////////////

const char* sh_vertex   = 
"#version 150               \n"
"in vec3 VertexPosition;    "
"in vec3 VertexColor;       "
"out vec3 Color;            "
"void main()                "
"{                          "
"    Color = VertexColor;   "
"    gl_Position = vec4( VertexPosition, 1.0 );"
"}";

#define OBJ2WORLD "ObjectToWorld"

const char* sh_vertex_obj   = 
"#version 150               \n"
"uniform mat4 ObjectToWorld;"
"in vec3      VertexPosition;    "
"in vec3      VertexColor;       "
"out vec3 Color;            "
"void main()                "
"{                          "
"    Color = VertexColor;   "
"    gl_Position = ObjectToWorld * vec4( VertexPosition, 1.0 );"
"}";

const char* sh_vertex_obj_pos   = 
"#version 150               \n"
"uniform mat4 ObjectToWorld;"
"in vec3      VertexPosition;    "
"void main()                "
"{                          "
"    gl_Position = ObjectToWorld * vec4( VertexPosition, 1.0 );"
"}";

const char* sh_vertex_obj_tex   = 
"#version 150               \n"
"uniform mat4 ObjectToWorld;"
"in vec3      VertexPosition;    "
"in vec3      TexCoord;"
"out vec3 v_color;            "
"out vec2 v_texcoord;"
"void main()                "
"{                          "
"    v_texcoord = TexCoord.xy;"
"    gl_Position = ObjectToWorld * vec4( VertexPosition, 1.0 );"
"}";


const char* sh_fragment = 
"#version 150                  \n"
"in vec3 Color;                "
"out vec4 FragColor;           "
"void main() {                 "
"    FragColor = vec4(Color, 1.0); "
"}";

const char* sh_fragment_fix_color = 
"#version 150                  \n"
"uniform vec4 ObjColor;        "
"out vec4 FragColor;           "
"void main() {                 "
"    FragColor = ObjColor; "
"}";

const char* sh_fragment_tex = 
"#version 150                  \n"
"uniform sampler2D Sampler;    "
"in vec2 v_texcoord;           "
"out vec4 FragColor;           "
"void main() {                 "
"    vec4 texColor = texture( Sampler, v_texcoord );"
"    FragColor = texColor;"
//"    FragColor = vec4(Color, 1.0); "
"}";

const char* sh_fragment_tex_alpha = 
"#version 150                  \n"
"uniform sampler2D Sampler;    "
"uniform vec4      Albedo;    "
"in vec2 v_texcoord;           "
"out vec4 FragColor;           "
"void main() {                 "
"    vec4 texColor = vec4(Albedo.rgb, Albedo.a * texture( Sampler, v_texcoord ).r);"
"    FragColor = texColor;"
//"    FragColor = vec4(Color, 1.0); "
"}";

const char* sh_geometry = "";

glh::ProgramHandle* sp_vcolor_handle;
glh::ProgramHandle* sp_tex_handle;
glh::ProgramHandle* sp_colorcoded;

glh::Texture* texture;
glh::Texture* fonttexture;

// App state

glh::RenderPassSettings g_renderpass_settings(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT,
                                            glh::vec4(0.76f,0.71f,0.5f,1.0f), 1);
glh::RenderPassSettings g_blend_settings(glh::RenderPassSettings::BlendSettings(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

bool g_run = true;

std::shared_ptr<glh::DefaultMesh> mesh(new glh::DefaultMesh()); // ram vertex data

// Screen quad shader
const char* sp_vcolors = "screen";
const char* sp_obj     = "screen_obj";
const char* sp_obj_tex = "screen_obj_tex";
const char* font_program_name = "screen_obj_font";
const char* sp_obj_colored = "screen_obj_colored";

float tprev = 0;
float angle = 0.f;
float radial_speed = 0.0f * PIf;

glh::Image8 image_test;
glh::Image8 image_bubble;
glh::Image8 font_target;
glh::Image8 noise_target;

glh::AssetManagerPtr manager;

glh::RenderEnvironment env;


glh::FullRenderable font_renderable;

std::shared_ptr<glh::FontContext> fontcontext;

glh::TextLine text_line;


void init_font_context(glh::GraphicsManager* gm)
{
    // TODO: gm->fontmanager

}

std::shared_ptr<glh::GlyphPane> glyph_pane;


void load_font_image(glh::GraphicsManager* gm)
{
    using namespace glh;

    std::string old_goudy("OFLGoudyStMTT.ttf");
    std::string junction("Junction-webfont.ttf");

    std::string fontpath = manager->fontpath();

    fontcontext.reset(new glh::FontContext(fontpath));

    float fontsize = 15.0f;

    BakedFontHandle handle;
    glh::Image8* fontimage;
    FontConfig config = {(double) fontsize, junction, 1024, 256};

    try{
        handle = fontcontext->render_bitmap(config);
        fontimage = fontcontext->get_font_map(handle);
        // TODO: write font image to file.
        write_image_png(*fontimage, "font.png");
    }
    catch(glh::GraphicsException& e){
        GLH_LOG_EXPR(e.get_message());
    }

    fonttexture->attach_image(*fontimage);

    glyph_pane = std::make_shared<GlyphPane>(gm, font_program_name, fontcontext.get());
    glyph_pane->font_handle_ = handle;
    glyph_pane->origin_ = vec2(100.f, 100.f);
    glyph_pane->text_field_.push_line("Hello, world.");
    glyph_pane->text_field_.push_line("This is another line, then.");
    glyph_pane->update_representation();
}
    // TODO
    //    Use scene tree to render. Attach glyph pane to parent node
    //    Use parent node material to render glyph_pane mesh
    //    Attach a dark background mesh to glyph pane
    //    Make text in glyph pane selectable
    //    Move font context as part of app or graphics manager so no need to init piece by piece
    //    Store and generate these 'default material' like the font material in sentral place
    //    Use even a global singleton to get them or something. Only rendering thread will access
    //    them and the default assets should be created on init.
    //    ie. GraphicsManager->default_assets or something.
    //    Figure out a nice way to handle the mapping. Now the matrix screen_to_view
    //      is just used to transform the mesh to pixel coordinates. 
    //    As a first step in centralized assets management impelement a monolithic
    //      asset owner and initializer class that just load and holds all the expected defaults.
    //      Later do something more sophisticated.

void init_uniform_data(){
    env.set_vec4("ObjColor", glh::vec4(0.f, 1.f, 0.f, 1.f));
    env.set_vec4("Albedo", glh::vec4(0.28f, 0.024f, 0.024f, 1.0));
    //env.set_texture2d("Sampler", texture);
    env.set_texture2d("Sampler", fonttexture); // TODO: Do we need material instance
                                               // that would allocate the sampler resources
                                               // to it's map?
}

bool init(glh::App* app)
{
    glh::GraphicsManager* gm = app->graphics_manager();

    fonttexture = gm->create_texture();

    gm->create_program(font_program_name, sh_geometry, sh_vertex_obj_tex, sh_fragment_tex_alpha);

   // mesh_load_screenquad(*mesh);
    int width = app->config().width;
    int height = app->config().height;
    mesh_load_screenquad((float) width, (float) height, *mesh);

    //mesh_load_screenquad(0.5f, 0.5f, *mesh);

    load_font_image(gm);

    init_uniform_data();


    return true;
}


bool update(glh::App* app)
{
    using namespace glh;

    float t = (float) app->time();
    angle += (t - tprev) * radial_speed;
    tprev = t;
    Eigen::Affine3f transform;
    transform = Eigen::AngleAxis<float>(angle, glh::vec3(0.f, 0.f, 1.f));

    mat4 screen_to_view = app_orthographic_pixel_projection(app);

    env.set_mat4(OBJ2WORLD, screen_to_view);

    float albedo_alpha = 1.0f - (t - floor(t));
    env.set_vec4("Albedo", glh::vec4(0.28f, 0.024f, 0.024f,albedo_alpha));



    return g_run;
}

void render_font(glh::App* app)
{
    glh::GraphicsManager* gm = app->graphics_manager();
    apply(g_renderpass_settings);
    apply(g_blend_settings);
    //font_renderable.reset_buffers(); 'light torture'
    gm->render(*glyph_pane->renderable_, env, env);
}

void resize(glh::App* app, int width, int height)
{
    std::cout << "Resize:" << width << " " << height << std::endl;
}

void print_mouse_position()
{
}

int g_mouse_x;
int g_mouse_y;

void mouse_move_callback(int x, int y)
{
    g_mouse_x = x;
    g_mouse_y = y;

    std::cout << "Mouse:"  << x << " " << y << std::endl;
}

char key_to_char(int key){
    if(key < CHAR_MAX && key > 0) return static_cast<char>(key);
    else return 0;
}

void key_callback(int key, const glh::Input::ButtonState& s)
{
    using namespace glh;

    if(key == Input::Esc) { g_run = false;}
    else if(key == Input::Left){ radial_speed -= 0.1f * PIf;}
    else if(key == Input::Right){ radial_speed += 0.1f * PIf;}
    else if(key == 'T'){ texture->attach_image(image_test);}
    else if(key == 'B'){ texture->attach_image(image_bubble);}
    else if(key == 'R') {
        font_renderable.reset_buffers();
    } 
}

int main(int arch, char* argv)
{
    glh::AppConfig config = glh::app_config_default(init, update, render_font, resize);

    config.width = 1024;
    config.height = 640;

    const char* config_file = "config.mp";

    manager = glh::make_asset_manager(config_file);

    glh::App app(config);

    GLH_LOG_EXPR("Logger started");
    add_key_callback(app, key_callback);
    add_mouse_move_callback(app, mouse_move_callback);
    glh::default_main(app);

    // TODO resource manager
    glyph_pane.reset();

    return 0;
}
