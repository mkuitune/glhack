/** 
Todo:
- rendertarget onscreen rendering
- mesh rendering
- hiddenlines rendering
- mesh edgelines rendering



class Texture
{
public:
    std::vector<uint8_t> data_;
};


class SamplerHandle
{
public:
    union{
    }
};


*/

#include "glbase.h"
#include "asset_manager.h"

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

std::shared_ptr<glh::Texture> texture;

// App state

glh::RenderPassSettings g_renderpass_settings(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT,
                                            glh::vec4(0.76f,0.71f,0.5f,1.0f), 1);

bool g_run = true;

glh::BufferSet bufs;

glh::DefaultMesh mesh;

// Screen quad shader
const char* sp_vcolors = "screen";
const char* sp_obj     = "screen_obj";
const char* sp_obj_tex = "screen_obj_tex";

float tprev = 0;
float angle = 0.f;
float radial_speed = 1.0f * PIf;

glh::NamedVar<glh::mat4> obj2world(OBJ2WORLD);

glh::Image8 image_test;
glh::Image8 image_bubble;

glh::AssetManagerPtr manager;

// TODO: create texture set from shader program. Assign 
// default checker pattern prior to having loaded texture data.

void load_image()
{

    texture = std::make_shared<glh::Texture>();

    const char* image_test_path = "test_512.png";
    const char* image_buble_path = "bubble.png";

    image_test   = manager->load_image_gl("test_512.png");
    image_bubble = manager->load_image_gl("bubble.png");

    write_image_png(image_test, "out.png");

    texture->assign(image_test, 0);

    // TODO: wrap blending on per drawable basis
    // drawable: program, mesh + uniforms
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void init_vertex_data()
{
    float posdata[] = {
        -0.8f, -0.8f, 0.0f,
        0.8f, -0.8f, 0.0f,
        0.0f, 0.8f, 0.0f
    };
    size_t posdatasize = static_array_size(posdata);

    float normaldata[] = {
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f
    };
    size_t normaldatasize = static_array_size(normaldata);

    float coldata[] = {
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f
    };
    size_t coldatasize = static_array_size(coldata);

    float texdata[] = {
        0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        0.5f, 1.0f, 0.0f
    };
    size_t texdatasize = static_array_size(texdata);

    mesh.get(glh::ChannelType::Position).set(posdata, posdatasize);
    mesh.get(glh::ChannelType::Color).set(coldata, coldatasize);
    mesh.get(glh::ChannelType::Normal).set(normaldata, normaldatasize);
    mesh.get(glh::ChannelType::Texture).set(texdata, texdatasize);
}

bool init(glh::App* app)
{
    glh::GraphicsManager* gm = app->graphics_manager();
    sp_vcolor_handle     = gm->create_program(sp_vcolors, sh_geometry, sh_vertex, sh_fragment);
    sp_vcolor_rot_handle = gm->create_program(sp_obj, sh_geometry, sh_vertex_obj, sh_fragment);
    sp_tex_handle        = gm->create_program(sp_obj_tex, sh_geometry, sh_vertex_obj_tex, sh_fragment_tex);

    init_vertex_data();

    load_image();

    glh::init_bufferset_from_program(bufs, sp_tex_handle);
    glh::assign_by_guessing_names(bufs, mesh);

    return true;
}

bool update(glh::App* app)
{
    float t = (float) app->time();
    angle += (t - tprev) * radial_speed;
    tprev = t;
    Eigen::Affine3f transform;
    transform = Eigen::AngleAxis<float>(angle, glh::vec3(0.f, 0.f, 1.f));
    obj2world = transform.matrix();
    return g_run;
}

void render(glh::App* app)
{
    apply(g_renderpass_settings);


    auto active = glh::make_active(*sp_tex_handle);

    active.bind_vertex_input(bufs.buffers_);
    active.bind_uniform(obj2world);
    active.bind_uniform("Sampler", *texture);

    active.draw();
}

void resize(glh::App* app, int width, int height)
{
    std::cout << "Resize:" << width << " " << height << std::endl;
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
    glh::default_main(app);

    return 0;
}