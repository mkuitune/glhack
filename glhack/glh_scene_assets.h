/**\file glh_scene_assets.h
    \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/
#pragma once

#include "glh_font.h"
#include "glh_scenemanagement.h"
#include "glh_scene_util.h"
#include "shims_and_types.h"
#include "glh_scene_textdisplay.h"

#include <vector>

namespace glh{

class SceneAssets{
public:

    SceneTree tree_; // TODO: Split SceneTree from SceneAssets?
                     // TODO: Different SceneTrees for different selection types
    std::list<GlyphPane>   glyph_panes_;
    std::list<TextLabel>   text_labels_;
    std::list<Camera>      cameras_;
    std::list<RenderPass>  render_passes_;

    GraphicsManager* gm_;
    FontManager*     font_manager_;
    App*             app_;


    // TODO: App.env: a wrapper for all addressable assets in the app.
    // Contains references to the lower level constructs so it can refer queries onwards, such as
    // "scenes/tree1/node3" would return a pointer with the object type (SceneTreeNode) 

    GlyphPane* create_glyph_pane(const std::string& program_name, const std::string& background_program_name){
        glyph_panes_.emplace_back(gm_, program_name, background_program_name, font_manager_, app_->string_numerator()("GlyphPane"));
        GlyphPane* pane = &glyph_panes_.back();
        SceneTree::Node* root = tree_.root();
        pane->attach(&tree_, root);

        return pane;
    }

    TextLabel* create_text_label(const std::string& program_name, const std::string& background_program_name){
        text_labels_.emplace_back(gm_, program_name, background_program_name, font_manager_, app_->string_numerator()("TextLabel"));
        TextLabel* label = &text_labels_.back();
        SceneTree::Node* root = tree_.root();
        label->attach(&tree_, root);

        return label;
    }

    Camera* create_camera(Camera::Projection projection_type){
         SceneTree::Node* node = tree_.add_node(tree_.root());
         cameras_.emplace_back(node);
         Camera* camera = &cameras_.back();
         camera->name_ = app_->string_numerator()("Camera");
         camera->node_ = node;
         camera->projection_ = projection_type;
         return camera;
    }

    RenderPass* create_render_pass(){
        render_passes_.emplace_back();
        RenderPass* pass = &render_passes_.back();
        pass->name_ = app_->string_numerator()("RenderPass");
        return pass;
    }

    RenderPass* create_render_pass(const std::string& name){
        render_passes_.emplace_back();
        RenderPass* pass = &render_passes_.back();
        pass->name_ = app_->string_numerator()(name);
        return pass;
    }

    // Manages per-frame updates. Basically should be implementable as a DynamicGraph graph (just assigns values to keys). But is not right now.
    void update(){

        tree_.update();
        tree_.apply_to_render_env();

        for(auto& c : cameras_){
            c.update(app_);
        }

        for(auto& g : glyph_panes_){
            g.update_representation();
        }

        for(auto& t : text_labels_){
            t.update_representation();
        }
    }

    SceneTree& scene(){ return tree_; }


private:
    SceneAssets(){}

    void xcept(const std::string& msg){
        throw GraphicsException(std::string("SceneAssets:") + msg);
    }

    void init(App* app, GraphicsManager* gm, FontManager* font_manager){
        app_ = app;
        gm_ = gm;
        font_manager_ = font_manager;

        if(!app_) xcept("Invalid app.");
        if(!gm_) xcept("Invalid graphics manager.");
        if(!font_manager_) xcept("Invalid font  manager.");

    }
public:

    static std::shared_ptr<SceneAssets> create(App* app, GraphicsManager* gm, FontManager* font_manager){
        std::shared_ptr<SceneAssets> assets_(new SceneAssets);
        assets_->init(app, gm, font_manager);
        return assets_;
    }

};

}
