/** \file glbase.cpp  OpenGL application base. Code with dependencies with the 
 OS interface.

Usage: in application code main, create

int main(int argc, char* argv[])
{
    glh::AppConfig config = glh::app_config_default(my_update, my_render);
    glh::App app(config);
    
    if(app.start()) 
    {
        while(app.update()){}
    }
    return 0;
}

or use

int main(int argc, char* argv[])
{
    glh::AppConfig config = glh::app_config_default(my_update, my_render);
    glh::App app(config);
    glh::default_main(app);
    return 0;
}

or, the minimal set:

int main(int argc, char* argv[])
{
    glh::run_minimal_scene();
    return 0;
}

    \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
 */

// This library is for quick 'hacks' and prototypes. Therefore, we include all the required
// source files here directly for now.


#include "conversion.h"
#include "conversion.c"

#include "stb_image.c"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"


#include "glbase.h"

#include<iostream>
#include <cstdlib>
#include <cassert>


glh::App*      g_app = 0;



namespace {

    ////////////////// Local utility function ////////////////////////

    ////////////// Event management ////////////////

//#define GLH_LOG_EVENTS

    ////// Callback backend wrappers ///////
    void local_push_event(const glh::InputEvent& e){g_app->user_input().push_event(e);}
    void local_mouse_wheel(int pos){g_app->user_input().mouse_wheel = pos;}
    void local_mouse_pos(int x, int y){g_app->user_input().mouse[0] = x; g_app->user_input().mouse[1] = y;}

    ////// Callback functions ///////
    extern "C" void GLFWCALL local_key_callback(int keyid, int action)
    {

        //GLH_LOG_EXPR("Key" << keyid << "action" << action);

        switch(action)
        {
            case GLFW_PRESS:
                local_push_event(glh::InputEvent::key(keyid, glh::Input::Held));
                break;
            case GLFW_RELEASE:
                local_push_event(glh::InputEvent::key(keyid, glh::Input::Released));
                break;
        } 
    }

    glh::Input::MouseButton glfw_mouse_to_glh(int button)
    {
        switch(button)
        {
            case GLFW_MOUSE_BUTTON_LEFT: return glh::Input::LeftButton;
            case GLFW_MOUSE_BUTTON_RIGHT: return glh::Input::RightButton;
            case GLFW_MOUSE_BUTTON_MIDDLE: return glh::Input::MiddleButton;
            default: assert("glfw_mouse_to_glh: Bad data");return glh::Input::LeftButton;
        }
    }

    extern "C" void GLFWCALL local_mousebutton_callback(int glfw_button, int action)
    {
        int button = glfw_mouse_to_glh(glfw_button);
        
        switch(action)
        {
            case GLFW_PRESS:
                local_push_event(glh::InputEvent::mouse_button(button, glh::Input::Held));
                break;
            case GLFW_RELEASE:
                local_push_event(glh::InputEvent::mouse_button(button, glh::Input::Released));
                break;
        } 
    }

    extern "C" void GLFWCALL local_mouseposition_callback(int x, int y)
    {
           //GLH_LOG_EXPR("Mouse:" << x<< " " << y);
           local_push_event(glh::InputEvent::mouse_move(x,y)); 
           local_mouse_pos(x, y);
    }

    extern "C" void GLFWCALL local_mousewheel_callback(int pos)
    {
        local_push_event(glh::InputEvent::mouse_wheel_move(pos));
        local_mouse_wheel(pos);
    }

    extern "C" void GLFWCALL local_windows_resize_callback(int width, int height)
    {
        if(g_app) g_app->resize(width, height);
    }
};


namespace glh{

/////////// Default application implementing functions ///////////

/** The default main. */
void default_main(App& app)
{
    if(app.start()) 
    {
        while(app.update()){}
    }
    app.end();
}

///////// UserInput //////////

void UserInput::push_event(const InputEvent& e)
{
    switch(e.type)
    {
        case InputEvent::MouseMove:
            calleach(mouse_move_callbacks, e.dim1, e.dim2);
            break;
        case InputEvent::MouseWheelMove:
            calleach(mouse_wheel_callbacks, e.dim1);
            break;
        case InputEvent::MouseButton:
            calleach(mouse_button_callbacks, e.id, e.state);
            break;
        case InputEvent::Key:
            calleach(key_callbacks, e.id, e.state);
            break;
    }

    add(events, e);
}

void UserInput::flush_events()
{
    events.clear();
}

///////// App Config //////////
AppConfig app_config_default(AppInit init, AppUpdate update, AppRender render, AppResize resize)
{
    assert(update != nullptr && render != nullptr);

    AppConfig config;
    config.width = 640;
    config.height = 480;
    config.fullscreen = false;
    config.init   = init;
    config.update = update;
    config.render = render;
    config.resize = resize;
    return config;
}

///////// App //////////
App::App(AppConfig config):config_(config)
{
    // Register  callbacks
    
    // Set global hooks 
    g_app = this;

    manager_.reset(make_graphics_manager());
}

bool App::start()
{
    bool initialized = false;

    int width = config_.width;
    int height = config_.height;

    // Create a window, etc
    if(glfwInit())
    {
        // TODO: window size etc

        bool use_fullscreen = false;
        int mode = use_fullscreen ? GLFW_FULLSCREEN : GLFW_WINDOW;

        if(use_fullscreen) {width = 10000; height = 10000;}

        if(glfwOpenWindow(width, height, 0,0,0,0,0,0, mode))
        {
            // Bind event callbacks
            glfwSetKeyCallback(local_key_callback);
            glfwSetMouseWheelCallback(local_mousewheel_callback);
            glfwSetMousePosCallback(local_mouseposition_callback);
            glfwSetMouseButtonCallback(local_mousebutton_callback);
            glfwSetWindowSizeCallback(local_windows_resize_callback);


            // Init glew
            GLint GlewInitResult = glewInit();

            if (GLEW_OK != GlewInitResult) 
            {
                printf("ERROR: %s\n",glewGetErrorString(GlewInitResult));
            }
            else
            {
                initialized = true;

                const GLubyte *renderer    = glGetString( GL_RENDERER );
                const GLubyte *vendor      = glGetString( GL_VENDOR );
                const GLubyte *version     = glGetString( GL_VERSION );
                const GLubyte *glslVersion = glGetString( GL_SHADING_LANGUAGE_VERSION );

                GLint major, minor;
                glGetIntegerv(GL_MAJOR_VERSION, &major);
                glGetIntegerv(GL_MINOR_VERSION, &minor);

                printf("GL Vendor : %s\n", vendor);
                printf("GL Renderer : %s\n", renderer);
                printf("GL Version (string) : %s\n", version);
                printf("GL Version (integer) : %d.%d\n", major, minor);
                printf("GLSL Version : %s\n", glslVersion);
            }
        }
    }

    if(initialized && config_.init) initialized = config_.init(this);

    return initialized;
}

bool App::update()
{
    bool running = true;

    // User interaction etc. 
    if((running = config_.update(this)))
    {
        // Clean event buffer
        user_input_.flush_events();

        // Render scene
        config_.render(this);
        // Swap buffers, call input callbacks
        glfwSwapBuffers();

        if(! glfwGetWindowParam(GLFW_OPENED))
        {
            running = false;
        }
    }
    return running;
}

bool App::end()
{
    glfwTerminate();
    return true;
}

double App::time()
{
    return glfwGetTime();
}

UserInput& App::user_input()
{
    return user_input_;
}

void App::resize(int width, int height)
{
    glViewport(0, 0, width, height);
    if(config_.resize) config_.resize(this, width, height);
}

GraphicsManager* App::graphics_manager(){return manager_.get();}


void add_key_callback(App& app, const UserInput::KeyCallback& cb){
    add(app.user_input().key_callbacks, cb);
}

void add_mouse_button_callback(App& app, const UserInput::KeyCallback& cb){
    add(app.user_input().mouse_button_callbacks, cb);
}
void add_mouse_move_callback(App& app, const UserInput::MouseMoveCallback& cb){
    add(app.user_input().mouse_move_callbacks, cb);
}
void add_mouse_wheel_callback(App& app, const UserInput::MouseWheelCallback& cb){
    add(app.user_input().mouse_wheel_callbacks, cb);
}


//////////////////// Images ////////////////////

namespace {
    void      free_uint8tarray(uint8_t* ptr){if(ptr) delete [] ptr;}
    uint8_t*  alloc_uint8t_array(int size){return new uint8_t[size];}
    void      stb_free_data(uint8_t* ptr){stbi_image_free(ptr);}
} // End anonymous namespace

Image8::Image8():width_(0), height_(0), channels_(0), data_(0), dealloc_(free_uint8tarray){}

Image8::Image8(const int w, const int h, const int chan):
    width_(w), height_(h), channels_(chan), dealloc_(free_uint8tarray)
{
    int datasize = size();
    data_ = alloc_uint8t_array(datasize);
}

/** Takes ownership of data pointer passed to it. */
Image8::Image8(const int w, const int h, const int chan, uint8_t* data, Deallocator dealloc):
     width_(w), height_(h), channels_(chan), data_(data), dealloc_(dealloc){}

Image8::Image8(Image8&& rhs):width_(rhs.width_), height_(rhs.height_), 
    channels_(rhs.channels_), data_(rhs.data_), dealloc_(rhs.dealloc_){
        rhs.data_ = 0;
}

Image8::~Image8(){if(data_) dealloc_(data_);}

Image8& Image8::operator=(Image8&& rhs){
    if(&rhs != this)
    {

        if(data_) dealloc_(data_);

        width_    = rhs.width_;
        height_   = rhs.height_;
        channels_ = rhs.channels_;
        data_     = rhs.data_;
        dealloc_  = rhs.dealloc_;

        rhs.data_ = 0;
    }
    return *this;
}

Image8 load_image(const char* path)
{
    int width     = 0;
    int height    = 0;
    int channels  = 0;

    Image8::Deallocator dealloc = stb_free_data;

    uint8_t* data = stbi_load(path, &width, &height, &channels, 0);

    if(data == 0){
        std::string msg = "Could not open image:" + std::string(path);
        throw GraphicsException(msg);
    }

    return Image8(width, height, channels, data, dealloc);
}

bool write_image_png(const Image8& image, const char* path)
{
    int stride = image.stride();
    int success = stbi_write_png(path, image.width_, image.height_, image.channels_, image.data_, stride);
    return (success == 0);
}

void   flip_vertical(Image8& img)
{
    if(img.empty()) return;
    int stride = img.stride();
    std::vector<uint8_t> tmp_buffer(stride);
    uint8_t* high_ptr;
    uint8_t* low_ptr;
    uint8_t* tmp_ptr(&tmp_buffer[0]);

    for(int h = 0; h < img.height_/2; ++h)
    {
        high_ptr = img.data_ + (h * stride);
        low_ptr =   img.data_ + ((img.height_ - h - 1) * stride);
        memcpy(tmp_ptr, high_ptr, stride);
        memcpy(high_ptr, low_ptr, stride);
        memcpy(low_ptr, tmp_ptr, stride);
    }
}

/////////////////////// Minimal app callbacks ///////////////////////

/** The minimal scene callbacks setup. */
void minimal_scene()
{
    bool run = true;
    glh::RenderPassSettings renderpass_settings(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT, 
                                                glh::vec4(0.76f,0.71f,0.5f,1.0f), 1);

    auto init = [&run](App* app) -> bool
    {
        return true;
    };//> Init callback

    auto update = [&run](App* app) -> bool
    {
        // Update state

        // return runstate
        return run;
    }; //> Update callback

    auto render = [&](App* app)
    {
        // Render all
        apply(renderpass_settings);
    };//> Render callback

    auto resize = [](glh::App* app, int width, int height)
    {
        std::cout << "Resize:" << width << " " << height << std::endl;
    }; //> Resize callback

    auto key_callback = [&run](int key, const glh::Input::ButtonState& s)
    {
        if(key == Input::Esc)
        {
            run = false;
        }
    };//> Input callback

    glh::AppConfig config = glh::app_config_default(init, update, render, resize);
    glh::App app(config);

    GLH_LOG_EXPR("Logger started");
    add_key_callback(app, key_callback);
    glh::default_main(app);
}

} // namespace glh
