/** 


*/


#include "glbase.h"
#include "asset_manager.h"
#include "glh_generators.h"
#include "glh_image.h"
#include "glh_typedefs.h"
#include "glh_font.h"
#include "shims_and_types.h"


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
glh::ProgramHandle* sp_vcolor_rot_handle;
glh::ProgramHandle* sp_tex_handle;
glh::ProgramHandle* sp_fonts_handle;
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
const char* sp_obj_font = "screen_obj_font";
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

glh::FullRenderable screenquad_image;
glh::FullRenderable screenquad_color;

glh::FullRenderable font_renderable;

std::shared_ptr<glh::FontContext> fontcontext;

int tex_unit_0 = 0;
int tex_unit_1 = 1;

// TODO: create texture set from shader program. Assign 
// default checker pattern prior to having loaded texture data.

glh::Image8 make_noise_texture(int dimension)
{
    using namespace glh;

    int w = dimension;
    int h = dimension;

    typedef InterpolatingMap<float, vec3, Smoothstep<float, vec3>> ColorMap;
    ColorMap colormap;


    vec3 black(0.f, 0.f, 0.f);
    vec3 white(1.f, 1.f, 1.f);

    //colormap.insert(0.0f, white);
    //colormap.insert(0.1f, black);
    //colormap.insert(0.2f, white);
    //colormap.insert(1.0f, white);

    colormap.insert(0.0f, white);
    colormap.insert(0.1f, black);
    colormap.insert(1.0f, white);

    Image8 image(h,w,3);
    uint8_t usample[3];

    int sample_count = 1024;
    Sampler1D<vec3> sampler = sample_interpolating_map(colormap, sample_count);
    sampler.technique(InterpolationType::Nearest);

    Autotimer timer;
    for_pixels(image, [&](uint8_t* pix, int xx, int yy){
        //double scale = 0.001;
        //std::cout << xx << " " << yy << std::endl;
        double scale = 0.01;
        double nx = scale * xx;
        double ny = scale * yy;
        
        float s = (float) simplex_noise(nx, ny);

        vec3 color = sampler.get(s);

        usample[0] = float_to_ubyte(color.x());
        usample[1] = float_to_ubyte(color.y());
        usample[2] = float_to_ubyte(color.z());
        Image8::set(pix, image.channels_, usample);
    });

    timer.stop("Generated in ");

    return image;
}

void load_image()
{
    const char* image_test_path = "bitmaps/test_512.png";
    const char* image_buble_path = "bitmaps/bubble.png";

    image_test   = manager->load_image_gl(image_test_path);
    image_bubble = manager->load_image_gl(image_buble_path);

    write_image_png(image_test, "out.png");

    //noise_target = make_noise_texture(512);
    //write_image_png(noise_target, "noise.png");

    texture->attach_image(image_test);
}

void load_font_image(glh::GraphicsManager* gm)
{
    using namespace glh;

    std::string old_goudy("OFLGoudyStMTT.ttf");
    std::string junction("Junction-webfont.ttf");

    std::string fontpath = manager->fontpath();

    fontcontext.reset(new glh::FontContext(fontpath));

    int texsize     = 1024;
    float fontsize = 15.0f;

    BakedFontHandle handle;
    glh::Image8* fontimage;

    try{
        handle = fontcontext->render_bitmap(junction, fontsize, texsize);
        fontimage = fontcontext->get_font_map(handle);
        // TODO: write font image to file.
        write_image_png(*fontimage, "font.png");
    }
    catch(glh::GraphicsException& e){
        GLH_LOG_EXPR(e.get_message());
    }

    std::string msg("Hello, world.");
    std::string msg2("A");
    std::vector<std::tuple<vec2, vec2>> text_coords;
    //float x = 0.f;
    //float y = 0.f;
    float x = 100.f;
    float y = 100.f;
    float line_height = fontsize;

    fontcontext->write_pixel_coords_for_string(msg, handle, x, y, text_coords);
    fontcontext->write_pixel_coords_for_string("This should be another line, then", handle, x, y + line_height, text_coords);

    // Create font material and renderable.
    DefaultMesh* fontmesh = gm->create_mesh();

    fonttexture->attach_image(*fontimage);

    font_renderable.bind_program(*sp_fonts_handle);
    font_renderable.set_mesh(fontmesh);
    //font_renderable.material_.set_texture2d("Sampler", fonttexture);

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



    // font renderable, with texture unit bound to env, 
    // and font image to texture unit
    // and mesh also.
    //
    // font render text takes in the font system data and the renderable, and writes per character squareds to 
    // the mesh.
    //
    // gen font-texture, to material env
    // bind font-image to font-texture
    // new material, program
    // bind font-texture to program
    // 
}

void init_uniform_data(){
    env.set_vec4("ObjColor", glh::vec4(0.f, 1.f, 0.f, 1.f));
    env.set_vec4("Albedo", glh::vec4(0.28f, 0.024f, 0.024f, 1.0));
    //env.set_texture2d("Sampler", texture);
    env.set_texture2d("Sampler", fonttexture);
}

bool init(glh::App* app)
{
    glh::GraphicsManager* gm = app->graphics_manager();

    texture = gm->create_texture();
    fonttexture = gm->create_texture();

    sp_vcolor_rot_handle = gm->create_program(sp_obj, sh_geometry, sh_vertex_obj, sh_fragment);
    sp_tex_handle        = gm->create_program(sp_obj_tex, sh_geometry, sh_vertex_obj_tex, sh_fragment_tex);
    sp_colorcoded        = gm->create_program(sp_obj_colored, sh_geometry, sh_vertex_obj_pos, sh_fragment_fix_color);

    sp_fonts_handle      = gm->create_program(sp_obj_font, sh_geometry, sh_vertex_obj_tex, sh_fragment_tex_alpha);

   // mesh_load_screenquad(*mesh);
    int width = app->config().width;
    int height = app->config().height;
    mesh_load_screenquad((float) width, (float) height, *mesh);

    //mesh_load_screenquad(0.5f, 0.5f, *mesh);

    load_image();
    load_font_image(gm);
    init_uniform_data();

    screenquad_color.bind_program(*sp_colorcoded);
    screenquad_color.set_mesh(mesh.get());

    screenquad_image.bind_program(*sp_tex_handle);
    screenquad_image.set_mesh(mesh.get());

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

void render_textured(glh::App* app)
{
    glh::GraphicsManager* gm = app->graphics_manager();
    apply(g_renderpass_settings);
    apply(g_blend_settings);
    gm->render(screenquad_image, env);
}

void render_font(glh::App* app)
{
    glh::GraphicsManager* gm = app->graphics_manager();
    apply(g_renderpass_settings);
    apply(g_blend_settings);
    gm->render(font_renderable, env);
}


void render_colorcoded(glh::App* app)
{
    glh::GraphicsManager* gm = app->graphics_manager();
    apply(g_renderpass_settings);
    gm->render(screenquad_color, env);
}

//std::function<void(glh::App*)> render = render_textured;
//std::function<void(glh::App*)> render = render_colorcoded;
//std::function<void(glh::App*)> render = render_textured;
std::function<void(glh::App*)> render = render_font;

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

void key_callback(int key, const glh::Input::ButtonState& s)
{
    using namespace glh;

    if(key == Input::Esc) { g_run = false;}
    else if(key == Input::Left){ radial_speed -= 0.1f * PIf;}
    else if(key == Input::Right){ radial_speed += 0.1f * PIf;}
    else if(key == 'T'){ texture->attach_image(image_test);}
    else if(key == 'B'){ texture->attach_image(image_bubble);}
}

int main(int arch, char* argv)
{
    glh::AppConfig config = glh::app_config_default(init, update, render, resize);

    config.width = 1024;
    config.height = 640;

    const char* config_file = "config.mp";

    manager = glh::make_asset_manager(config_file);

    glh::App app(config);

    GLH_LOG_EXPR("Logger started");
    add_key_callback(app, key_callback);
    add_mouse_move_callback(app, mouse_move_callback);
    glh::default_main(app);

    return 0;
}
