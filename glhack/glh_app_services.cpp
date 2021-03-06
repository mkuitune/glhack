/**\file glh_app_services.cpp
    \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/
#pragma once

#include "glh_app_services.h"
#include "glh_default_assets.h"

namespace glh{

static MovementMapper get_pixel_space_movement_mapper(App* app, const mat4& view_to_world_p, SceneTree* scene, SceneTree::Node* node)
{
    mat4 view_to_world(view_to_world_p);
    return [=](vec3 delta, SceneTree::Node* node){
        auto parent = scene->get(node->parent_id_);

        mat4 screen_to_view = app_orthographic_pixel_projection(app);

        mat4 world_to_object = parent->local_to_world_.inverse();

        mat4 screen_to_object = world_to_object * view_to_world * screen_to_view;

        vec4 v(delta[0], delta[1], 0, 0);

        vec3 nd = decrease_dim<float, 4>(screen_to_object * v);

        Transform t = Transform::position(nd);
        t.scale_ = vec3(0.f, 0.f, 0.f);
        parent->transform_.add_to_each_dim(t);

        // Below is a debug rotation routine that should be removed
        Transform t2 = Transform();
        t2.scale_ = vec3(0.f, 0.f, 0.f);
        float wz = delta[0] < 0.f ? 0.05f : -0.05f;
        t2.rotation_.z() = wz;

        // Apply transform to node
        // TODO: Add optional transform callback to node
        // TODO: Preferably nodes would not be fat containers but a 'compiled' collection data that is then processed per frame

        node->transform_.add_to_each_dim(t2);
    };
}

void AppServices::init_font_manager()
{
    GraphicsManager* gm = app_->graphics_manager();

    std::string fontpath = manager_->fontpath();

    if(!directory_exists(fontpath.c_str())){
        throw GraphicsException(std::string("Font directory not found:") + fontpath);
    }

    fontmanager_.reset(new FontManager(gm, fontpath));
}


void AppServices::init(App* app, const char* config_file)
{
    app_ = app;
    GraphicsManager* gm = app_->graphics_manager();

    manager_ = make_asset_manager(config_file);

    init_font_manager();

    assets_ = SceneAssets::create(app_, gm, fontmanager_.get());

    if(!assets_.get()){ throw GraphicsException("AppServices: Could not init assets_"); }

    load_default_programs_glsl150(gm);

    default_camera = assets_->create_camera(Camera::Projection::Orthographic);

    ui_context_.reset(new glh::UiContext(*gm, *app, graph_, assets_->scene()));

    if(!ui_context_.get()){ throw GraphicsException("AppServices: Could not init ui_context_"); }

    ui_context_->set_movement_mapper_generator(get_pixel_space_movement_mapper);

    //render_picker_ = std::make_shared<glh::RenderPicker>(*app);
    //if(!render_picker_.get()){ throw GraphicsException("AppServices: Could not init render_picker_");}
    //render_picker_->selection_program_ = gm->program(GLH_COLOR_PICKER_PROGRAM);

    ui_context_->render_picker_.selection_program_ = gm->program(GLH_COLOR_PICKER_PROGRAM);

    selection_world_2d_ = ui_context_->add_selection_world("SelectionWorld2D", default_camera);
    selection_world_3d_ = ui_context_->add_selection_world("SelectionWorld3D", default_camera);
    selection_pass_2d_default_ = &selection_world_2d_->render_pass_;
    selection_pass_3d_default_ = &selection_world_3d_->render_pass_;

    //render_picker_pass_ = assets_->create_render_pass("RenderPickerPass");

    //picker_service_.init(render_picker_.get(), render_picker_pass_, &ui_context().focus_context_);

}

}
