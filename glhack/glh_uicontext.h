/**\file glh_uicontext.h Event callbacks, input device state callbacks etc.
    \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/
#pragma once

#include "glh_scenemanagement.h"
#include "glh_timebased_signals.h"
#include "glh_dynamic_graph.h"

namespace glh{

/** Class that draws together several existing lower level system to create a UI context. */
class UiContext{
public:

    // TODO: Probably want to lift techniques away from here at some point.
    struct Technique{
        enum t{ColorInterpolation};
        static void color_interpolation(App& app, DynamicGraph& graph,
                                        StringNumerator& string_numerator, SceneTree::Node* node);
    };


    UiContext(GraphicsManager& manager, glh::App& app, DynamicGraph& graph, StringNumerator& string_numerator):
        manager_(manager),app_(app), graph_(graph),string_numerator_(string_numerator), render_picker_(app){
        init_assets();

        left_button_down_ = false;
        mouse_current_ = glh::vec2i(0,0);
        mouse_prev_ = glh::vec2i(0,0);
    }
    
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
        enum t{LeftButtonActivated, LeftButtonDeactivated, MouseMoved};
    };



    void left_button_is_down(){
        if(!left_button_down_){
            left_button_down_ = true;
            events_.push(Event::LeftButtonActivated);
        }
    }

    void left_button_up(){
        left_button_down_ = false;
        events_.push(Event::LeftButtonDeactivated);
    }

    void move(int x, int y){
        mouse_prev_ = mouse_current_;
        mouse_current_[0] = x;
        mouse_current_[1] = y;
        events_.push(Event::MouseMoved);
    }

    // TODO: Add events!
    // - when draggins stops to element, the element gets the color of the dragged
    // - when an item is selected, assign a new random color to it by pressing r
    // - add new dynamic action when dragged - scale the node a little bit smaller when dragging - scale back when released

    
    //TODO: Into a graph node 
    static void mouse_move_node(glh::App& app, glh::SceneTree::Node* node, vec2i delta)
    {
        glh::mat4 screen_to_view   = app_orthographic_pixel_projection(&app);
        glh::mat4 view_to_world    = glh::mat4::Identity();
        glh::mat4 world_to_object  = glh::mat4::Identity();
        glh::mat4 screen_to_object = world_to_object * view_to_world * screen_to_view ;
    
        // should be replaced with view specific change vector...
        // projection of the view change vector onto the workplane...
        // etc.
    
        glh::vec4 v((float) delta[0], (float) delta[1], 0, 0);
        glh::vec3 nd = glh::decrease_dim<float, 4>(screen_to_object * v);
    
        node->transform_.position_ = node->transform_.position_ + nd;
    }


    // Must call only after selection context has been filled
    void update(){
        // Handle events
        while(!events_.empty()){
            Event::t e = events_.top();
            events_.pop();
            if(e == Event::LeftButtonActivated){
                for(SceneTree::Node* node: focus_context_.currently_focused_){
                    dragged.push_back(node);
                    node->interaction_lock_ = true;
                }
            }
            else if(e == Event::LeftButtonDeactivated){
                // TODO: End drag events.
                // If a dragging stops on a node, figure out
                // a) is there a rule to handle this specific dragged on-to situation
                // b) apply the rule.
                for(auto node:dragged) node->interaction_lock_ = false;
                dragged.clear();
            }
            else if(e == Event::MouseMoved){
                vec2i delta = mouse_current_ - mouse_prev_;
                // TODO: Replace with dynamic graph network attached to each renderable node
                for(SceneTree::Node* node: dragged){
                    mouse_move_node(app_, node, delta);
                }
            }
        }
    }

//
// Member variables
//
    vec2i mouse_current_;
    vec2i mouse_prev_;

    std::stack<Event::t> events_;
    std::vector<SceneTree::Node*> dragged;


    bool left_button_down_;

    GraphicsManager& manager_;
    App&             app_;
    DynamicGraph&    graph_;
    StringNumerator& string_numerator_;

    FocusContext        focus_context_;
    RenderPicker        render_picker_;
    std::list<SceneTree::Node*> ui_entities_;

    std::set<SceneTree::Node*> focused;

    // Resource handles
    glh::ProgramHandle* sp_select_program_;

};

} // namespace glh
