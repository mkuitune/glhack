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

bool          glh_logging_active(){return true;}
std::ostream* glh_get_log_ptr(){return &std::cout;}


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

#if 1
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
AppConfig app_config_default(AppUpdate update, AppRender render, AppResize resize)
{
    assert(update != nullptr && render != nullptr);

    AppConfig config;
    config.width = 640;
    config.height = 480;
    config.fullscreen = false;
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

        if(glfwOpenWindow(width, height, 0,0,0,0,0,0, GLFW_WINDOW))
        {
            initialized = true;

            // Bind event callbacks
            glfwSetKeyCallback(local_key_callback);
            glfwSetMouseWheelCallback(local_mousewheel_callback);
            glfwSetMousePosCallback(local_mouseposition_callback);
            glfwSetMouseButtonCallback(local_mousebutton_callback);
            glfwSetWindowSizeCallback(local_windows_resize_callback);
        }
    }

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

UserInput& App::user_input()
{
    return user_input_;
}

void App::resize(int width, int height)
{
    if(config_.resize) config_.resize(this, width, height);
}


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

///////////// OpenGL Utilities /////////////

bool check_gl_error()
{
    bool result = true;
    GLenum ErrorCheckValue = glGetError();
    if (ErrorCheckValue != GL_NO_ERROR)
    {
        result = false;
        GLH_LOG_EXPR("GL error:" << gluErrorString(ErrorCheckValue));
    }
    return result;
}


#endif
} // namespace glh
