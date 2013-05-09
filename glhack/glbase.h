/** \file glbase.h Wraps a functionality that enables the development of a modern
 * OpenGL application. This is the header that is usually included in client programs.
 *
 *  \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
 */
#pragma once

#include "glhack.h"

#include<functional>
#include<algorithm>
#include<unordered_map>
#include<set>
#include<iostream>

#include "math_tools.h"

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

    ArraySet<KeyCallback, funcompare<KeyCallback>>               key_callbacks;
    ArraySet<KeyCallback, funcompare<KeyCallback>>               mouse_button_callbacks;
    ArraySet<MouseMoveCallback, funcompare<MouseMoveCallback>>   mouse_move_callbacks;
    ArraySet<MouseWheelCallback, funcompare<MouseWheelCallback>> mouse_wheel_callbacks;

    vec2i          mouse;
    int            mouse_wheel;
};

class App;

/** Default main loop for a glh app. */
void default_main(App& app);

/* Introductions for application functions. Implement these in your code.*/
typedef std::function<bool(App*)> AppInit;
typedef std::function<bool(App*)> AppUpdate;
typedef std::function<void(App*)> AppRender;
typedef std::function<void(App*, int, int)> AppResize;

/** Application configuration options */
struct AppConfig
{
    int width;
    int height;
    bool fullscreen;

    AppInit   init;
    AppUpdate update;
    AppRender render;
    AppResize resize;
};

/** Return a default config.*/
AppConfig app_config_default(AppInit init, AppUpdate update, AppRender render, AppResize resize);

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

    /** If app initialization succeeded return seconds elapsed since App::start was called */
    double time();

    /** Return config. */
    const AppConfig& config() const {return config_;}

    /** Return reference to user input manager. */
    UserInput& user_input();

    GraphicsManager* graphics_manager();

    /** Signal window resize for app.*/
    void resize(int width, int height);
private:
    AppConfig                        config_;
    UserInput                        user_input_;
    bool                             running_;
    std::shared_ptr<GraphicsManager> manager_;
};

void add_key_callback(App& app, const UserInput::KeyCallback& cb);
void add_mouse_button_callback(App& app, const UserInput::KeyCallback& cb);
void add_mouse_move_callback(App& app, const UserInput::MouseMoveCallback& cb);
void add_mouse_wheel_callback(App& app, const UserInput::MouseWheelCallback& cb);

/** Return orthographic projection for pixel coordinates.
 *  Origin is located at top left corner.
*/
mat4 app_orthographic_pixel_projection(const App* app);


///////////// UiContext //////////////

// Tree of widgets, operation wise. Not a spatial tree, but context tree?
// 

class UiElement{
public:
    virtual void focus_gained() = 0;
    virtual void focus_lost() = 0;
    
    virtual void button_activate() = 0;
    virtual void button_deactivate() = 0;
};



class UiEntity{
public:

    FullRenderable* renderable_;


};
typedef std::shared_ptr<UiEntity> UiEntityPtr; 


/** User interface state. */
class UIContext{
public:
    /*typedef void (*HoverCB)(FullRenderable*);
    typedef void (*ClickCB)(FullRenderable*);
    typedef void (*DragCB)(FullRenderable*, vec2i);*/

    // RegisterKeySink?

#define UICTX_SELECT_NAME "SelectionColor"

    //typedef std::array<float, 4> color_t;

    //static color_t color_t_of_vec4(const vec4& v){
    //    color_t c;
    //    for(int i = 0; i < 4; ++i) c[i] = v[i];
    //    return c;
    //}

    //static vec4 vec4_of_color_t(const color_t& c){
    //    vec4 v;
    //    for(int i = 0; i < 4; ++i) v[i] = c[i];
    //    return v;
    //}

    struct Selectable{
        UiEntity&         entity_;
        //color_t           color_;
        RenderEnvironment material_; // TODO: implement a lighter way to transfer color to program
                                     // without each selectable requiring a map of it's own

                                    // TODO: object to world transforms etc should be passed from original
                                    // material, selection color only from local env.

                                    // TODO: Create a check that FullRenderable does not contain an
                                    // env parameter with the same name for picking color.

        Selectable(UiEntity& e, const vec4& color):entity_(e)
        //    , color_(color)
        {
            material_.set_vec4(UICTX_SELECT_NAME, color);
        }

        void render(GraphicsManager& gm, ProgramHandle& selection_program){
            //TODO: force env to color
            gm.render(*entity_.renderable_, selection_program, entity_.renderable_->material_, material_);
            //entity_->render();
        }
    };


    UIContext(App& app):app_(app){}

    void add(UiEntity* e){
        int id = idgen_.new_id();
        vec4 color = ColorSelection::color_of_id(id);
        selectables_.emplace(id, Selectable(*e, color));
        entity_to_int_[e] = id;
    }

     void remove(UiEntity* e){
        int id = entity_to_int_[e];
        selectables_.erase(id);
        entity_to_int_.erase(e);
        idgen_.release(id);
    }

    std::tuple<Box<int,2>, bool> setup_context(){
         // set up scene
         RenderPassSettings settings(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT,
                                            glh::vec4(0.0f,0.0f,0.0f,1.f), 1);
         // Constrain view box
        const int w = app_.config().width;
        const int h = app_.config().height;

        Box<int,2> screen_bounds = make_box2(0, 0, w, h);
        int mousey = h - pointer_y_;
        Box<int,2> mouse_bounds  = make_box2(pointer_x_ - 1, mousey - 1, pointer_x_ + 2, mousey + 2);
        Box<int,2> read_bounds;
        bool       bounds_ok;

        std::tie(read_bounds, bounds_ok) = intersect(screen_bounds, mouse_bounds);
        auto read_dims = read_bounds.size();

        glDisable(GL_SCISSOR_TEST);
        
        apply(settings);

        glEnable(GL_SCISSOR_TEST);
        glScissor(read_bounds.min[0], read_bounds.min[1], read_dims[0], read_dims[1]);

        return std::make_tuple(read_bounds, bounds_ok);
    }

    int do_picking(Box<int,2>& read_bounds){
        auto read_dims = read_bounds.size();
        GLenum read_format = GL_BGRA;
        GLenum read_type = GL_UNSIGNED_INT_8_8_8_8_REV;
        const int pixel_count = 9;
        const int pixel_channels = 4;
        uint8_t imagedata[pixel_count * pixel_channels] = {0};
        glFlush();
        glReadBuffer(GL_BACK);
        glReadPixels(read_bounds.min[0], read_bounds.min[1], read_dims[0], read_dims[1], read_format, read_type, imagedata);

        const uint8_t* b = &imagedata[4 * pixel_channels];
        const uint8_t* g = b + 1;
        const uint8_t* r = g + 1;
        const uint8_t* a = r + 1;

        //char buf[1024];
        //sprintf(buf, "r%hhu g%hhu b%hhu a%hhu", *r, *g, *b, *a);
        //std::cout << "Mouse read:" << buf << std::endl;

        return ColorSelection::id_of_color(*r,*g,*b);
    }

    void reset_context(){
         glDisable(GL_SCISSOR_TEST);
    }

    void render_selectables(){
        GraphicsManager* gm = app_.graphics_manager();
        Box<int,2> read_bounds;
        bool bounds_ok;

        std::tie(read_bounds, bounds_ok) = setup_context();

        if(bounds_ok){
            for(auto& id_selectable: selectables_){
                // render e
                id_selectable.second.render(*gm, *selection_program_);
            }

            int selected_id = do_picking(read_bounds);

        }

        //Once all the items are rendered do picking
        reset_context();
    }

    void pointer_move_cb(int x, int y){
       pointer_x_ = x;
       pointer_y_ = y;
    }

    static std::function<void(int,int)> get_pointer_move_cb(UIContext& ctx){
        using namespace std::placeholders;
        return std::bind(&UIContext::pointer_move_cb, ctx, _1, _2);
    }

    int pointer_x_;
    int pointer_y_;

    std::map<int, Selectable> selectables_;
    std::map<UiEntity*, int> entity_to_int_;

    ColorSelection::IdGenerator idgen_;

    App& app_;

    ProgramHandle* selection_program_;
};

typedef std::shared_ptr<UIContext> UIContextPtr;


///////////// Misc ///////////////

/** Execute a minimal scene. Test that everything builds and runs etc.*/
void minimal_scene();

double progtime();

class Autotimer{
public:
    double t;
    Autotimer(){t = progtime();}
    void stop(const char* msg)
    {
        double delta = progtime() - t;
        std::cout << msg << " Duration:"  << delta << std::endl;
    }
};

} // namespace glh
