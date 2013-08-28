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

    UiContext(GraphicsManager& manager, glh::App& app, DynamicGraph& graph, StringNumerator& string_numerator):
        manager_(manager),app_(app), graph_(graph),string_numerator_(string_numerator), render_picker_(app){
        init_assets();
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

};


} // namespace glh
