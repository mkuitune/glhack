/** 


*/


#include "glbase.h"
#include "asset_manager.h"
#include "glh_generators.h"
#include "glh_image.h"
#include "glh_typedefs.h"
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
"in vec3      VertexColor;       "
"in vec3      TexCoord;"
"out vec3 v_color;            "
"out vec2 v_texcoord;"
"void main()                "
"{                          "
"    v_color = VertexColor;   "
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
"in vec3 Color;                "
"in vec2 v_texcoord;           "
"out vec4 FragColor;           "
"void main() {                 "
"    vec4 texColor = texture( Sampler, v_texcoord );"
"    FragColor = texColor;"
//"    FragColor = vec4(Color, 1.0); "
"}";

const char* sh_geometry = "";

glh::ProgramHandle* sp_vcolor_handle;
glh::ProgramHandle* sp_vcolor_rot_handle;
glh::ProgramHandle* sp_tex_handle;
glh::ProgramHandle* sp_colorcoded;

std::shared_ptr<glh::Texture> texture;

// App state

glh::RenderPassSettings g_renderpass_settings(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT,
                                            glh::vec4(0.76f,0.71f,0.5f,1.0f), 1);

bool g_run = true;

std::shared_ptr<glh::DefaultMesh> mesh(new glh::DefaultMesh()); // ram vertex data

// Screen quad shader
const char* sp_vcolors = "screen";
const char* sp_obj     = "screen_obj";
const char* sp_obj_tex = "screen_obj_tex";
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
    texture = std::make_shared<glh::Texture>();

    const char* image_test_path = "bitmaps/test_512.png";
    const char* image_buble_path = "bitmaps/bubble.png";

    image_test   = manager->load_image_gl(image_test_path);
    image_bubble = manager->load_image_gl(image_buble_path);

    write_image_png(image_test, "out.png");

    //noise_target = make_noise_texture(512);
    //write_image_png(noise_target, "noise.png");

    texture->assign(image_test, 0);

    // TODO: wrap blending on per drawable basis
    // drawable: program, mesh + uniforms - texture properties - render settings, overloads
    // TODO: RenderPassSettings as RenderSettings, stackable and overloadable
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // TODO to texture parameters.
}

void init_uniform_data(){
    env.set_vec4("ObjColor", glh::vec4(0.f, 1.f, 0.f, 1.f));
    env.set_texture2d("Sampler", texture);
}

bool init(glh::App* app)
{
    glh::GraphicsManager* gm = app->graphics_manager();

    sp_vcolor_rot_handle = gm->create_program(sp_obj, sh_geometry, sh_vertex_obj, sh_fragment);
    sp_tex_handle        = gm->create_program(sp_obj_tex, sh_geometry, sh_vertex_obj_tex, sh_fragment_tex);
    sp_colorcoded        = gm->create_program(sp_obj_colored, sh_geometry, sh_vertex_obj_pos, sh_fragment_fix_color);

    mesh_load_screenquad(*mesh);
    load_image();
    init_uniform_data();

    screenquad_color.bind_program(*sp_colorcoded);
    screenquad_color.set_mesh(mesh);

    screenquad_image.bind_program(*sp_tex_handle);
    screenquad_image.set_mesh(mesh);

    return true;
}

bool update(glh::App* app)
{
    float t = (float) app->time();
    angle += (t - tprev) * radial_speed;
    tprev = t;
    Eigen::Affine3f transform;
    transform = Eigen::AngleAxis<float>(angle, glh::vec3(0.f, 0.f, 1.f));

    env.set_mat4(OBJ2WORLD, transform.matrix());

    return g_run;
}

void render_textured(glh::App* app)
{
    apply(g_renderpass_settings);

    screenquad_image.render(env);
}

void render_colorcoded(glh::App* app)
{
    apply(g_renderpass_settings);

    screenquad_color.render(env);
}

//std::function<void(glh::App*)> render = render_textured;
//std::function<void(glh::App*)> render = render_colorcoded;
std::function<void(glh::App*)> render = render_textured;

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
    else if(key == 'T'){ texture->assign(image_test, 0);}
    else if(key == 'B'){ texture->assign(image_bubble, 0);}
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
