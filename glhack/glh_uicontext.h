/**\file glh_uicontext.h Event callbacks, input device state callbacks etc.
    \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/
#pragma once

#include "glh_scenemanagement.h"
#include "glh_timebased_signals.h"
#include "glh_dynamic_graph.h"
#include "geometry.h"

namespace glh{

/** Class that stored modifiers to particular transform channels. */
//template<class T>
//struct TransformModifier{
//
//    enum User{Position = 0, Scale = 1, Rotation = 2};
//    char active_;
//    ExplicitTransform<T> transform_;
//
//    TransformModifier():active_[0]{}
//
//    void apply(ExplicitTransform<T> transform_){
//
//    }
//
//};

typedef std::function<void(vec3, SceneTree::Node*)> MovementMapper; //> Maps a delta vector in normalized device coordinates to node interaction
typedef std::function<MovementMapper(App*, SceneTree*,SceneTree::Node*)> MovementMapperGen;

/**Returns a Workplane that maps the pointer device movement to change in the target coordinates. E.g. mouse on the
   screen to the scene graph. Accepts in a source to target coordinate system mapping matrix and the workplane for the mapping (workplane in
   target coordinate system)*/
//typedef std::function<Workplane(mat4, Plane)> WorkplaneGenerator;

/** Class that draws together several existing lower level system to create a UI context. */
class UiContext{
public:

    // TODO: Probably want to lift techniques away from here at some point.
    struct Technique{
        enum t{ColorInterpolation};
        static void color_interpolation(App& app, DynamicGraph& graph,
                                        StringNumerator& string_numerator, SceneTree::Node* node);
    };



        //TODO: Into a graph node  ?
    static MovementMapper get_default_mapper(App* app, SceneTree* scene,SceneTree::Node* node)
    {
        app;
        scene;
        node;
        return [=](vec3 delta, glh::SceneTree::Node* node){
            delta;
            node;
        };
    }

    // TODO scene to own context
    UiContext(GraphicsManager& manager, glh::App& app, DynamicGraph& graph, StringNumerator& string_numerator, SceneTree& scene):
        manager_(manager),app_(app), graph_(graph),string_numerator_(string_numerator), render_picker_(app), scene_(scene)
    {
        init_assets();

        mouse_current_ = glh::vec2i(0,0);
        mouse_prev_ = glh::vec2i(0,0);

        mapper_gen_ = get_default_mapper;
    }

    void set_movement_mapper_generator(MovementMapperGen mapper){mapper_gen_ = mapper;}

    void init_assets();

    // TODO: Add add_render_queue or such routine
    //void add_node(SceneTree::Node* n){
    //    add(ui_entities_, n);
    //    render_picker_.add(&ui_entities_.back());

    //    // TODO: Remove technique forcing
    //    add_technique(Technique::ColorInterpolation, n);
    //}

    // Techniques - fixed interaction forms.
    void add_technique(Technique::t technique, SceneTree::Node* node){
        if(technique == Technique::ColorInterpolation){
            Technique::color_interpolation(app_, graph_, string_numerator_, node);
        }
    }

    //
    // Old mouse comes here
    //

    struct Event{
        enum t{
            ButtonActivated, ButtonDeactivated, MouseMoved
        };

        t event_;
        Input::AnyButton button_;

        Event(const t& event, const Input::AnyButton& button):event_(event), button_(button){
        }

        Event(const t& event):event_(event), button_(Input::any_button(Input::Custom, 0)){
        }

        bool is(const t& event) const {return event_ == event;}
        bool is(const Event& event)const {return event_ == event.event_ && button_ == event.button_;}
    };

    void mouse_move(int x, int y){
        mouse_prev_ = mouse_current_;
        mouse_current_[0] = x;
        mouse_current_[1] = y;
        events_.push(Event::MouseMoved);
    }

    void keyboard_callback(int key, const Input::ButtonState& s)
    {
        Input::AnyButton button = Input::any_button(Input::Keyboard, key);
        buttons_[button] = s;

        if(s == Input::Held) events_.push(Event(Event::ButtonActivated, button));
        else                 events_.push(Event(Event::ButtonDeactivated, button));
    }

    void mouse_button_callback(int key, const glh::Input::ButtonState& s)
    {
        using namespace glh;

        Input::AnyButton button = Input::any_button(Input::Mouse, key);
        buttons_[button] = s;

        if(s == Input::Held) events_.push(Event(Event::ButtonActivated, button));
        else                 events_.push(Event(Event::ButtonDeactivated, button));
    }


    // TODO: Add events!
    // - when draggins stops to element, the element gets the color of the dragged
    // - when an item is selected, assign a new random color to it by pressing r
    // - add new dynamic action when dragged - scale the node a little bit smaller when dragging - scale back when released

    static int brk()
    {
         return 11;
    }

    bool is_left_mouse_button_activated(const Event& e){return e.is(Event(Event::ButtonActivated, Input::any_button(Input::Mouse, Input::MouseButton::LeftButton)));}
    bool is_left_mouse_button_deactivated(const Event& e){return e.is(Event(Event::ButtonDeactivated, Input::any_button(Input::Mouse, Input::MouseButton::LeftButton)));}

    bool is_left_mouse_button_down(){
        return has_pair(buttons_, Input::any_button(Input::Mouse, Input::MouseButton::LeftButton), Input::Held);
    }

    // Must call only after selection context has been filled
    void update(){
        // Handle events
        while(!events_.empty()){
            Event e = events_.top();
            events_.pop();
            //GLH_LOG_EXPR("Event");

            if(is_left_mouse_button_activated(e)){
                for(SceneTree::Node* node: focus_context_.currently_focused_){
                    dragged.push_back(node);
                    node->interaction_lock_ = true;
                    movement_mappers_[node->id_] = mapper_gen_(&app_, &scene_, node);
                    //GLH_LOG_EXPR("added to dragged:" << node->id_ );
                }
            }

            else if(is_left_mouse_button_deactivated(e)){
                // TODO: End drag events.
                // If a dragging stops on a node, figure out
                // a) is there a rule to handle this specific dragged on-to situation
                // b) apply the rule.
                for(auto node:dragged){ 
                    node->interaction_lock_ = false;
                    movement_mappers_.erase(node->id_);
                    //GLH_LOG_EXPR("clean from dragged:" << node->id_ );
                }

                dragged.clear();
               
            }

            // TODO: Can graph operations be used to replace this?

            {
                vec2i delta = mouse_current_ - mouse_prev_;
                vec3 deltaf((float) delta[0], (float) delta[1], 0.f);
                // TODO: Replace with dynamic graph network attached to each renderable node
                if(is_left_mouse_button_down()){
                    for(SceneTree::Node* node: dragged){
                        movement_mappers_[node->id_](deltaf, node);
                        //Transform t(movement_mappers_[node->id_](deltaf, node));
                        //node->transform_.add_to_each_dim(t);
                        //GLH_LOG_EXPR("move:" << node->id_ );
                    }
                }
            }
        }
    }

//
// Member variables
//
    MovementMapperGen mapper_gen_;

    std::map<Input::AnyButton, Input::ButtonState> buttons_;

    std::map<int, MovementMapper> movement_mappers_; //> object-id : workplane

    //TransformGadget transform_gadget_;

    vec2i mouse_current_;
    vec2i mouse_prev_;

    std::stack<Event>             events_;
    std::vector<SceneTree::Node*> dragged;

    SceneTree& scene_;
    GraphicsManager& manager_;
    App&             app_;
    DynamicGraph&    graph_;
    StringNumerator& string_numerator_;

    FocusContext                focus_context_;
    RenderPicker                render_picker_;
    std::list<SceneTree::Node*> ui_entities_;

    std::set<SceneTree::Node*> focused;

    // Resource handles
    glh::ProgramHandle* sp_select_program_;

};

} // namespace glh
