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

// Screen quad shader
const char* sp_vcolors = "screen";
const char* sp_obj     = "screen_obj";

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

const char* sh_fragment = 
"#version 150                  \n"
"in vec3 Color;                "
"out vec4 FragColor;           "
"void main() {                 "
"    FragColor = vec4(Color, 1.0); "
"}";

const char* sh_geometry = "";

glh::ProgramHandle* sp_vcolor_handle;
glh::ProgramHandle* sp_vcolor_rot_handle;

// App state

glh::RenderPassSettings g_renderpass_settings(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT,
                                            glh::vec4(0.76f,0.71f,0.5f,1.0f), 1);

bool g_run = true;

glh::VertexChunk poschunk(glh::BufferSignature(glh::TypeId::Float32, 3));
glh::VertexChunk normalchunk(glh::BufferSignature(glh::TypeId::Float32, 3));
glh::VertexChunk colchunk(glh::BufferSignature(glh::TypeId::Float32, 3));
glh::VertexChunk texchunk(glh::BufferSignature(glh::TypeId::Float32, 2));

glh::BufferSet bufs;


float tprev = 0;
float angle = 0.f;
float radial_speed = 1.0f * PIf;

glh::NamedVar<glh::mat4> obj2world(OBJ2WORLD);

glh::Image8 image;

void load_image()
{
    const char* image_path = "test_512.png";
    image = glh::load_image(image_path);
    write_image_png(image, "out.png");
}

void init_vertex_data()
{
    float posdata[] = {
        -0.8f, -0.8f, 0.0f,
        0.8f, -0.8f, 0.0f,
        0.0f, 0.8f, 0.0f
    };
    size_t posdatasize = sizeof(posdata) / sizeof(*posdata);

    float normaldata[] = {
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f
    };
    size_t normaldatasize = sizeof(normaldata) / sizeof(*normaldata);

    float coldata[] = {
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f
    };
    size_t coldatasize = sizeof(posdata) / sizeof(*posdata);

    float texdata[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        0.5f, 1.0f
    };
    size_t texdatasize = sizeof(texdata) / sizeof(*texdata);

    // Todo create each bufferhandle automatically
    poschunk.set(posdata, posdatasize);
    colchunk.set(coldata, coldatasize);
    texchunk.set(texdata, texdatasize);
    normalchunk.set(normaldata, normaldatasize);
}

bool init(glh::App* app)
{
    glh::GraphicsManager* gm = app->graphics_manager();
    sp_vcolor_handle     = gm->create_program(sp_vcolors, sh_geometry, sh_vertex, sh_fragment);
    sp_vcolor_rot_handle = gm->create_program(sp_obj, sh_geometry, sh_vertex_obj, sh_fragment);

    init_vertex_data();

    bufs.create_handle("VertexPosition", glh::BufferSignature(glh::TypeId::Float32, 3));
    bufs.create_handle("VertexColor",    glh::BufferSignature(glh::TypeId::Float32, 3));

    bufs.assign("VertexPosition", poschunk);
    bufs.assign("VertexColor", colchunk);

    load_image();

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


    auto active = glh::make_active(*sp_vcolor_rot_handle);

    active.bind_vertex_input(bufs.buffers_);
    active.bind_uniform(obj2world);
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
}

int main(int arch, char* argv)
{
    glh::AppConfig config = glh::app_config_default(init, update, render, resize);

    config.width = 1024;
    config.height = 640;

    glh::App app(config);

    GLH_LOG_EXPR("Logger started");
    add_key_callback(app, key_callback);
    glh::default_main(app);

    return 0;
}
