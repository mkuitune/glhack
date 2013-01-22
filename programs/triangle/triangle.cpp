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
// Screen quad shader
const char* sqs_name     = "screen";

const char* sqs_vertex   = 
"#version 150               \n"
"in vec3 VertexPosition;    "
"in vec3 VertexColor;       "
"out vec3 Color;            "
"void main()                "
"{                          "
"    Color = VertexColor;   "
"    gl_Position = vec4( VertexPosition, 1.0 );"
"}";

const char* sqs_fragment = 
"#version 150                 \n"
"in vec3 Color;                "
"out vec4 FragColor;           "
"void main() {                 "
"    FragColor = vec4(Color, 1.0); "
"}";

const char* sqs_geometry = "";

glh::ProgramHandle sqs_handle;

// App state

glh::RenderPassSettings g_renderpass_settings(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT,
                                            glh::vec4(0.76f,0.71f,0.5f,1.0f), 1);

bool g_run = true;

float pos[] = {
    -0.8f, -0.8f, 0.0f,
    0.8f, -0.8f, 0.0f,
    0.0f, 0.8f, 0.0f
};

float color[] = {
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 1.0f
};

GLuint vboHandles[2];
GLuint posHandle;
GLuint colHandle;

GLuint vaoHandle;

bool init(glh::App* app)
{
    glh::GraphicsManager* gm = app->graphics_manager();
    sqs_handle = gm->create_program(sqs_name, sqs_geometry, sqs_vertex, sqs_fragment);

    glGenBuffers(2, vboHandles);

    posHandle = vboHandles[0];
    colHandle = vboHandles[1];

    glBindBuffer(GL_ARRAY_BUFFER, posHandle);
    glBufferData(GL_ARRAY_BUFFER, 9 * sizeof(float), pos, GL_STATIC_DRAW);
    
    glBindBuffer(GL_ARRAY_BUFFER, colHandle);
    glBufferData(GL_ARRAY_BUFFER, 9 * sizeof(float), color, GL_STATIC_DRAW);

    glGenVertexArrays(1, &vaoHandle);
    glBindVertexArray(vaoHandle);

    // map these to sharedprogram assigned indices
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, posHandle);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLubyte*) 0);
   
    glBindBuffer(GL_ARRAY_BUFFER, colHandle);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (GLubyte*) 0);

    return true;
}

bool update(glh::App* app)
{
    return g_run;
}

void render(glh::App* app)
{
    apply(g_renderpass_settings);

    glBindVertexArray(vaoHandle);
    glDrawArrays(GL_TRIANGLES, 0, 3);
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

    config.width = 1024;
    config.height = 640;

    glh::App app(config);

    GLH_LOG_EXPR("Logger started");
    add_key_callback(app, key_callback);
    glh::default_main(app);

    return 0;
}
