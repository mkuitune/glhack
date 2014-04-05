/**\file glh_uicontext.h Event callbacks, input device state callbacks etc.
    \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/
#pragma once

#include "glh_scenemanagement.h"
#include "glh_timebased_signals.h"
#include "glh_dynamic_graph.h"
#include "geometry.h"
#include "shims_and_types.h"

namespace glh{

    typedef std::function<void(vec3, SceneTree::Node*)> MovementMapper; //> Maps a delta vector in normalized device coordinates to node interaction
    typedef std::function<MovementMapper(App*, const mat4&, SceneTree*, SceneTree::Node*)> MovementMapperGen;

struct SelectionWorld{
    typedef std::vector<SceneTree::Node*> selection_list_t;

    RenderPass       render_pass_;
    selection_list_t selected_list_;
    FocusContext     focus_context_;
    std::set<SceneTree::Node*> dragged_;
    std::string      name_;

    SelectionWorld(const std::string& name):name_(name){
        render_pass_.set_queue_filter(glh::pass_pickable);
        render_pass_.name_ = name_ + "/RenderPass";
    }

    RenderPass& items(){ return render_pass_; }

    void update(SceneTree& scene){
        render_pass_.update_queue_filtered(scene);
        //picker_->update_ids();
    }
};

/** Class that draws together several existing lower level system to create a UI context. */
class UiContext{
public:

    // TODO: Probably want to lift techniques away from here at some point.
    struct Technique{
        enum t{ ColorInterpolation };
        static void color_interpolation(App& app, DynamicGraph& graph,
            StringNumerator& string_numerator, SceneTree::Node* node);
    };

    //TODO: Into a graph node  ?
    static MovementMapper get_default_mapper(App* app, const mat4& mat, SceneTree* scene, SceneTree::Node* node)
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
    UiContext(GraphicsManager& manager, glh::App& app, DynamicGraph& graph, SceneTree& scene):
        manager_(manager), app_(app), graph_(graph), render_picker_(app), scene_(scene)
    {
        init_assets();

        mouse_current_ = glh::vec2i(0, 0);
        mouse_prev_ = glh::vec2i(0, 0);

        mapper_gen_ = get_default_mapper;
    }

    void set_movement_mapper_generator(MovementMapperGen mapper){ mapper_gen_ = mapper; }

    void init_assets();

    // Techniques - fixed interaction forms.
    void add_technique(Technique::t technique, SceneTree::Node* node){
        if(technique == Technique::ColorInterpolation){
            Technique::color_interpolation(app_, graph_, app_.string_numerator(), node);
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

        bool is(const t& event) const { return event_ == event; }
        bool is(const Event& event)const { return event_ == event.event_ && button_ == event.button_; }
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

    static int brk()
    {
        return 11;
    }

    bool is_left_mouse_button_activated(const Event& e){ return e.is(Event(Event::ButtonActivated, Input::any_button(Input::Mouse, Input::MouseButton::LeftButton))); }
    bool is_left_mouse_button_deactivated(const Event& e){ return e.is(Event(Event::ButtonDeactivated, Input::any_button(Input::Mouse, Input::MouseButton::LeftButton))); }

    bool is_left_mouse_button_down(){
        return has_pair(buttons_, Input::any_button(Input::Mouse, Input::MouseButton::LeftButton), Input::Held);
    }

    // Must call only after selection context has been filled TODO: need concept of UI layers - a layer wich stores the camera transform used to render it and which ids belong to it
    void update(){

        // Pick passes
        pick_passes();

        // Handle events
        while(!events_.empty()){
            Event e = events_.top();
            events_.pop();
            //GLH_LOG_EXPR("Event");

            // TODO: Refactor so that these parameters are not in nodes but in map separate from nodes
            //       Try to use closures in the map instead of explicit reference to node pointers.

            if(is_left_mouse_button_activated(e)){

                for(auto& sw:selection_worlds_){

                    if(!keyboard_is_held(Input::Lctrl)) sw.dragged_.clear();

                    for(SceneTree::Node* node : sw.focus_context_.currently_focused_){
                        Camera* active_camera = sw.render_pass_.camera_;
                        if(!active_camera) throw GraphicsException("UiContext::update selection world active camera not set");

                        sw.dragged_.insert(node);
                        node->interaction_lock_ = true;
                        movement_mappers_[node->id_] = mapper_gen_(&app_, active_camera->world_to_screen_.inverse(), &scene_, node);
                        //GLH_LOG_EXPR("added to dragged:" << node->id_ );
                    }
                }
            }

            else if(is_left_mouse_button_deactivated(e)){
                // TODO: End drag events.
                // If a dragging stops on a node, figure out
                // a) is there a rule to handle this specific dragged on-to situation
                // b) apply the rule.
                for(auto& sw : selection_worlds_){
                    if(!keyboard_is_held(Input::Lctrl)){
                        for(auto node : sw.dragged_){
                            node->interaction_lock_ = false;
                            movement_mappers_.erase(node->id_);
                            //GLH_LOG_EXPR("clean from dragged:" << node->id_ );
                        }

                        sw.dragged_.clear();
                    }
                }
            }

            // TODO: Can graph operations be used to replace this?
            // Answer: No, closure bound to mouse state is just fine. Probably should refactor so that
            // the position gets fed to the active closurers list (create an active closures list!)
            // that gets popped when the mouse button is raised (add a closure to mouse button raised
            // state that will erase the closures?)

            {
                vec2i delta = mouse_current_ - mouse_prev_;
                vec3 deltaf((float) delta[0], (float) delta[1], 0.f);

                for(auto& sw : selection_worlds_){
                    if(is_left_mouse_button_down()){
                        for(SceneTree::Node* node : sw.dragged_){
                            movement_mappers_[node->id_](deltaf, node);
                        }
                    }
                }
            }
        }
    }

    SelectionWorld* add_selection_world(const std::string& name, Camera* pass_camera){
        selection_worlds_.emplace_back(name);
        selection_worlds_.back().render_pass_.camera_ = pass_camera;
        return &selection_worlds_.back();
    }

    bool keyboard_is_held(int button){
        return has_pair(buttons_, Input::any_button(Input::Keyboard, button), Input::Held);
    }

    void pick_passes(){

        int picker_x = mouse_current_[0];
        int picker_y = mouse_current_[1];

        for(auto& w : selection_worlds_){
            render_picker_.render_pass_ = &w.render_pass_;
            render_picker_.update_ids();

            glh::FocusContext::Focus focus = w.focus_context_.start_event_handling();

            auto picked = render_picker_.render_selectables(picker_x, picker_y);

            for(auto p : picked){
                focus.on_focus(p);
            }

            focus.update_event_state();
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

    SceneTree&       scene_;
    GraphicsManager& manager_;
    App&             app_;
    DynamicGraph&    graph_;

    //FocusContext                focus_context_;
    RenderPicker                render_picker_;

    // Resource handles
    glh::ProgramHandle* sp_select_program_;

    std::vector<SelectionWorld> selection_worlds_;
};

} // namespace glh
