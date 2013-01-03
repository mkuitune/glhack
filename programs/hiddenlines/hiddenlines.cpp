/** ... */

#include "glhack.h"


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


glh::RenderPassSettings g_renderpass_settings(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT,
                                            glh::vec4(0.76f,0.71f,0.5f,1.0f), 1);

bool g_run = true;

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
    glh::AppConfig config = glh::app_config_default(update, render, resize);
    glh::App app(config);

    GLH_LOG_EXPR("Logger started");
    add_key_callback(app, key_callback);
    glh::default_main(app);
    return 0;
}
