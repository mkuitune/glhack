/** \file glbase.h Wraps a functionality that enables the development of a modern
 * OpenGL application in a backend agnostic manner.
    \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
 */
#pragma once


#ifdef WIN32
#include<windows.h>
#endif

#include<GL/glew.h>
#include <GL/glfw.h>

#include<functional>
#include<algorithm>
#include<unordered_map>
#include<set>
#include<iostream>

#include "shims_and_types.h"
#include "math_tools.h"

//TODO: Better logging.
bool          glh_logging_active();
std::ostream* glh_get_log_ptr();

#define GLH_LOG_EXPR(expr_param) \
    do { if ( glh_logging_active() ){\
    (*glh_get_log_ptr()) << __FILE__ \
    << " [" << __LINE__ << "] : " << expr_param \
    << ::std::endl;} }while(false)


namespace glh {

class Input
{
public:
    enum Key{
        Unknown = GLFW_KEY_UNKNOWN,    
        Space = GLFW_KEY_SPACE,      
        Special = GLFW_KEY_SPECIAL,    
        Esc = GLFW_KEY_ESC,        
        F1 = GLFW_KEY_F1,         
        F2 = GLFW_KEY_F2,         
        F3 = GLFW_KEY_F3,         
        F4 = GLFW_KEY_F4,         
        F5 = GLFW_KEY_F5,         
        F6 = GLFW_KEY_F6,         
        F7 = GLFW_KEY_F7,         
        F8 = GLFW_KEY_F8,         
        F9 = GLFW_KEY_F9,         
        F10 = GLFW_KEY_F10,        
        F11 = GLFW_KEY_F11,        
        F12 = GLFW_KEY_F12,        
        F13 = GLFW_KEY_F13,        
        F14 = GLFW_KEY_F14,        
        F15 = GLFW_KEY_F15,        
        F16 = GLFW_KEY_F16,        
        F17 = GLFW_KEY_F17,        
        F18 = GLFW_KEY_F18,        
        F19 = GLFW_KEY_F19,        
        F20 = GLFW_KEY_F20,        
        F21 = GLFW_KEY_F21,        
        F22 = GLFW_KEY_F22,        
        F23 = GLFW_KEY_F23,        
        F24 = GLFW_KEY_F24,        
        F25 = GLFW_KEY_F25,        
        Up = GLFW_KEY_UP,         
        Down = GLFW_KEY_DOWN,       
        Left = GLFW_KEY_LEFT,       
        Right = GLFW_KEY_RIGHT,      
        Lshift = GLFW_KEY_LSHIFT,     
        Rshift = GLFW_KEY_RSHIFT,     
        Lctrl = GLFW_KEY_LCTRL,      
        Rctrl = GLFW_KEY_RCTRL,      
        Lalt = GLFW_KEY_LALT,       
        Ralt = GLFW_KEY_RALT,       
        Tab = GLFW_KEY_TAB,        
        Enter = GLFW_KEY_ENTER,      
        Backspace = GLFW_KEY_BACKSPACE,  
        Insert = GLFW_KEY_INSERT,     
        Del = GLFW_KEY_DEL,        
        Pageup = GLFW_KEY_PAGEUP,     
        Pagedown = GLFW_KEY_PAGEDOWN,   
        Home = GLFW_KEY_HOME,       
        End = GLFW_KEY_END,        
        Kp0 = GLFW_KEY_KP_0,       
        Kp1 = GLFW_KEY_KP_1,       
        Kp2 = GLFW_KEY_KP_2,       
        Kp3 = GLFW_KEY_KP_3,       
        Kp4 = GLFW_KEY_KP_4,       
        Kp5 = GLFW_KEY_KP_5,       
        Kp6 = GLFW_KEY_KP_6,       
        Kp7 = GLFW_KEY_KP_7,       
        Kp8 = GLFW_KEY_KP_8,       
        Kp9 = GLFW_KEY_KP_9,       
        KpDivide = GLFW_KEY_KP_DIVIDE,  
        KpMultiply = GLFW_KEY_KP_MULTIPLY,
        KpSubtract = GLFW_KEY_KP_SUBTRACT,
        KpAdd = GLFW_KEY_KP_ADD,     
        KpDecimal = GLFW_KEY_KP_DECIMAL, 
        KpEqual = GLFW_KEY_KP_EQUAL,   
        KpEnter = GLFW_KEY_KP_ENTER,   
        KpNumLock = GLFW_KEY_KP_NUM_LOCK,
        CapsLock = GLFW_KEY_CAPS_LOCK,  
        ScrollLock = GLFW_KEY_SCROLL_LOCK,
        Pause = GLFW_KEY_PAUSE,      
        Lsuper = GLFW_KEY_LSUPER,     
        Rsuper = GLFW_KEY_RSUPER,     
        Menu = GLFW_KEY_MENU,       
        Last = GLFW_KEY_LAST
   };

    enum MouseButton{LeftButton = 0, RightButton = 1, MiddleButton = 2};
    enum ButtonState{Held,Released};


    // TODO: maps for key states: std::unordered_map<int, callback_fun>
};

/** Contains one input device event */
class InputEvent
{
public:

    enum EventType{MouseMove, MouseWheelMove, MouseButton, Key};

    EventType type;

    int dim1;                      //> MouseMove:x; MouseWheelMove: position
    int dim2;                      //> MouseMove:y;
    Input::ButtonState state;      //> MouseButton, Key: ButtonState
    int id;                        //> Button identifier
    
    static InputEvent mouse_move(const int x, const int y){return InputEvent(MouseMove, x, y,0);}
    static InputEvent mouse_wheel_move(const int x){return InputEvent(MouseWheelMove, x, 0,0);}
    static InputEvent mouse_button(const int id, const Input::ButtonState s){return InputEvent(MouseButton, s, id);}
    static InputEvent key(const int id, const Input::ButtonState s){return InputEvent(Key, s,id);}

    InputEvent(){}
    InputEvent(const EventType t, const int d1, const int d2, const int idv):type(t), dim1(d1), dim2(d2), id(idv){}
    InputEvent(const EventType t, const Input::ButtonState s, int idv):type(t), dim1(0), dim2(0), state(s), id(idv){}
};

/** Class to manage user input */
class UserInput
{
public:

    typedef Pool<InputEvent> EventContainer;

    typedef std::function<void(int x, int y)>                   MouseMoveCallback;
    typedef std::function<void(int x)>                          MouseWheelCallback;
    typedef std::function<void(int, const Input::ButtonState&)> KeyCallback;

    /** Add event to event queue. */
    void push_event(const InputEvent& e);
    /** Remove all events. */
    void flush_events();

    EventContainer events;

    ArraySet<KeyCallback, funcompare<KeyCallback>>             key_callbacks;
    ArraySet<KeyCallback, funcompare<KeyCallback>>             mouse_button_callbacks;
    ArraySet<MouseMoveCallback, funcompare<MouseMoveCallback>> mouse_move_callbacks;
    ArraySet<MouseWheelCallback, funcompare<MouseWheelCallback>> mouse_wheel_callbacks;

    vec2i          mouse;
    int            mouse_wheel;
};

class App;

/** Default main loop for a glh app. */
void default_main(App& app);

/* Introductions for application functions. Implement these in your code.*/
typedef std::function<bool(App*)> AppUpdate;
typedef std::function<void(App*)> AppRender;
typedef std::function<void(App*, int, int)> AppResize;

/** Application configuration options */
struct AppConfig
{
    int width;
    int height;
    bool fullscreen;

    AppUpdate update;
    AppRender render;
    AppResize resize;
};

/** Return a default config.*/
AppConfig app_config_default(AppUpdate update, AppRender render, AppResize resize);

/** Contains the application state. Note: There can be only one App alive per
 * program, so this should be used as a singleton.*/
class App
{
public:
    App(AppConfig config);

    /** Initialize the program state. */
    bool start();
    /** Call the update and render callbacks and swap buffers.*/
    bool update();
    /** Finalize resources. */
    bool end();

    /** Return config. */
    AppConfig& config(){return config_;}

    /** Return reference to user input manager. */
    UserInput& user_input();
   
    /** Signal window resize for app.*/
    void resize(int width, int height);
private:
    AppConfig config_;
    UserInput user_input_;
    bool      running_;
};

void add_key_callback(App& app, const UserInput::KeyCallback& cb);
void add_mouse_button_callback(App& app, const UserInput::KeyCallback& cb);
void add_mouse_move_callback(App& app, const UserInput::MouseMoveCallback& cb);
void add_mouse_wheel_callback(App& app, const UserInput::MouseWheelCallback& cb);

///////////// OpenGL Utilities /////////////

/** Check GL error. @return true if no error found. */
bool check_gl_error();

} // namespace glh
