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

float posdata[] = {
    -0.8f, -0.8f, 0.0f,
    0.8f, -0.8f, 0.0f,
    0.0f, 0.8f, 0.0f
};
size_t posdatasize = sizeof(posdata) / sizeof(*posdata);

float coldata[] = {
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 1.0f
};
size_t coldatasize = sizeof(posdata) / sizeof(*posdata);

glh::VertexChunk poschunk(glh::BufferSignature(glh::TypeId::Float32, 3));
glh::VertexChunk colchunk(glh::BufferSignature(glh::TypeId::Float32, 3));

glh::BufferSet bufs;

glh::VarMap vars;

float tprev = 0;
float angle = 0.f;
float radial_speed = 1.0 * M_PI;

bool init(glh::App* app)
{
    glh::GraphicsManager* gm = app->graphics_manager();
    sp_vcolor_handle     = gm->create_program(sp_vcolors, sh_geometry, sh_vertex, sh_fragment);
    sp_vcolor_rot_handle = gm->create_program(sp_obj, sh_geometry, sh_vertex_obj, sh_fragment);

    // Todo create each bufferhandle automatically
    poschunk.set(posdata, posdatasize);
    colchunk.set(coldata, coldatasize);

    bufs.create_handle("VertexPosition", glh::BufferSignature(glh::TypeId::Float32, 3));
    bufs.create_handle("VertexColor",    glh::BufferSignature(glh::TypeId::Float32, 3));

    bufs.assign("VertexPosition", poschunk);
    bufs.assign("VertexColor", colchunk);

    return true;
}

bool update(glh::App* app)
{
    float t = app->time();
    angle += (t - tprev) * radial_speed;
    tprev = t;
    Eigen::Affine3f transform;
    transform = Eigen::AngleAxis<float>(angle, glh::vec3(0.f, 0.f, 1.f));
    vars["ObjectToWorld"] = transform.matrix();
    return g_run;
}

void render(glh::App* app)
{
    apply(g_renderpass_settings);


    auto active = glh::make_active(*sp_vcolor_rot_handle);

    active.bind_vertex_input(bufs.buffers_);
    active.bind_uniforms(vars);
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
    else if(key == Input::Left){ radial_speed -= 0.1 * M_PI;}
    else if(key == Input::Right){ radial_speed += 0.1 * M_PI;}
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
