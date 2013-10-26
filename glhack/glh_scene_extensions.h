/**\file glh_scene_extensions.h
    \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/
#pragma once

#include "glh_font.h"
#include "glh_scenemanagement.h"
#include <vector>

namespace glh{

/** Contains the GPU and non-GPU font assets. */
class FontManager
{
public:
    FontManager(GraphicsManager* gm, const std::string& font_directory):context_(font_directory),
        gm_(gm)
    {
    }

    Texture* get_font_texture(const BakedFontHandle& handle){
        const std::string& font_name(handle.first);
        auto font_texture = font_textures_.find(font_name);
        if(font_texture != font_textures_.end()){ return font_texture->second;}
        else{
            Texture* t = gm_->create_texture();
            font_textures_[font_name] = t;

            Image8* image = context_.get_font_map(handle);
            t->attach_image(*image);

            return t;
        }
    }

    std::map<std::string, Texture*> font_textures_;

    GraphicsManager* gm_;
    FontContext context_;

private:
    FontManager();
    FontManager(const FontManager& old);
    FontManager& operator=(const FontManager& old);
};

/** Collection of text fields and their rendering options. */
class TextField {
public:
    std::vector<std::shared_ptr<TextLine>> text_fields_;

    float line_height_; /* Multiples of font size. */

    TextLine& push_line(const std::string& string){
        const size_t line_pos = text_fields_.size();
        text_fields_.emplace_back(std::make_shared<TextLine>(string, line_pos));
        return *text_fields_.back().get();
    }

    TextLine& last(){
        return *text_fields_.back().get();
    }
};

void render_glyph_coordinates_to_mesh(FontContext& context, TextField& t,
    BakedFontHandle handle, vec2 origin, double line_height /* Multiples of font height */,
    DefaultMesh& mesh);


class GlyphPane
{
public:

    GlyphPane(GraphicsManager* gm, const std::string& program_name, FontManager* fontmanager)
        :gm_(gm), node_(0), line_height_(1.0),fontmanager_(fontmanager),
        origin_(0.f, 0.f), font_handle_(invalid_font_handle()){
        fontmesh_ = gm_->create_mesh();
        renderable_= gm_->create_renderable();

        auto font_program_handle = gm->program(program_name);

        renderable_->bind_program(*font_program_handle);
        renderable_->set_mesh(fontmesh_);
    }

    ~GlyphPane(){
        gm_->release_mesh(fontmesh_);
        gm_->release_renderable(renderable_);
        if(node_ && parent_ && scene_){
            parent_->remove_child(node_);
            scene_->finalize(node_);
        }
    }

    void set_font(const std::string& name, float size){
        FontConfig config = get_font_config(name, size);
        FontContext& context(fontmanager_->context_);
        font_handle_ = context.render_bitmap(config); // May throw GraphicsException
        // fontimage_ = context.get_font_map(font_handle_);
        //write_image_png(*fontimage, "font.png");
        fonttexture_ = fontmanager_->get_font_texture(font_handle_);
    }

    void attach(SceneTree& scene, SceneTree::Node& parent){
        node_ = scene.add_node(&parent, renderable_);
        parent_ = &parent;
        scene_ = &scene;
    }

    void update_representation()
    {
        if(handle_valid(font_handle_)){
            render_glyph_coordinates_to_mesh(fontmanager_->context_, text_field_, font_handle_, origin_, line_height_, *fontmesh_);
            renderable_->reset_buffers();
        }
    }


    Texture*            fonttexture_;
    SceneTree*          scene_;
    SceneTree::Node*    parent_;
    SceneTree::Node*    node_;
    FontManager*        fontmanager_;
    DefaultMesh*        fontmesh_;
    GraphicsManager*    gm_;
    vec2                origin_;
    BakedFontHandle     font_handle_;
    double              line_height_;
    FullRenderable*     renderable_;
    TextField           text_field_;
};


}
