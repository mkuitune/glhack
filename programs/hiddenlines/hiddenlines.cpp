/** 
- rendertarget onscreen rendering
- mesh rendering
- hiddenlines rendering
- mesh edgelines rendering

*/

#include "glbase.h"


/*
scenegraph:

node 
transform
bounding-box

node tra
*/


class SCNode
{
public:

};


class SceneGraph
{
public:

};

// Shaders

// Screen quad shader
const char* sqs_name     = "screen";
const char* sqs_fragment = "";
const char* sqs_vertex   = "";
const char* sqs_geometry = "";

glh::ProgramHandle* sqs_handle;

// App state

glh::RenderPassSettings g_renderpass_settings(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT,
                                            glh::vec4(0.76f,0.71f,0.5f,1.0f), 1);

bool g_run = true;

bool init(glh::App* app)
{
    glh::GraphicsManager* gm = app->graphics_manager();
    sqs_handle = gm->create_program(sqs_name, sqs_geometry, sqs_vertex, sqs_fragment);

    std::vector<int> iv;
    iv.push_back(1);

    return true;
}

bool update(glh::App*)
{
    return g_run;
}

void render(glh::App*)
{
    apply(g_renderpass_settings);
}

void resize(glh::App* app, int width, int height)
{
    std::cout << "Resize:" << width << " " << height << std::endl;
}

void key_callback(int key, const glh::Input::ButtonState& s)
{
    using namespace glh;
    if(key == Input::Esc)
    {
        g_run = false;
    }
}

int main(int arch, char* argv)
{
    glh::AppConfig config = glh::app_config_default(init, update, render, resize);
    glh::App app(config);


    GLH_LOG_EXPR("Logger started");
    add_key_callback(app, key_callback);
    glh::default_main(app);

    return 0;
}
