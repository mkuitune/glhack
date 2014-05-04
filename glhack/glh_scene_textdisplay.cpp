/**\file glh_scene_textdisplay.cpp
\author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/
#pragma once

#include "glh_scene_textdisplay.h"
#include<limits>

namespace glh{

void transfer_position_data_to_mesh(GlyphCoords& coords,
    glh::DefaultMesh* fontmesh)
{
    using namespace glh;
    // TODO: All of these can be reserved from a arena that is filled per frame
    // (never deallocated, just held for reserve at each frame)
    // Use an arena-allocator getter at app or gm (probably app or app::resources or
    // something like that).

    GlyphCoords::row_coords_container_t& text_coords(coords.coords_);

    std::vector<float> posdata;
    std::vector<float> texdata;

    // transfer tex_coords to mesh
    for(auto& row : text_coords){
        for(auto& iquad : row){
            for(auto ref : iquad.quad_){
                posdata.push_back(ref.pos[0]);
                posdata.push_back(ref.pos[1]);
                posdata.push_back(0.f);

                texdata.push_back(ref.tex[0]);
                texdata.push_back(ref.tex[1]);
                texdata.push_back(0.f);
            }
        }
    }

    if(posdata.size() > 0){
        fontmesh->set(ChannelType::Position, &posdata[0], posdata.size());
        fontmesh->set(ChannelType::Texture, &texdata[0], texdata.size());
    }
    else {
        float* nul = 0;
        fontmesh->set(ChannelType::Position, nul, 0);
        fontmesh->set(ChannelType::Texture, nul, 0);
    }
}

void render_glyph_coordinates_to_mesh(
    FontContext& context,
    TextField& t,
    BakedFontHandle handle,
    vec2 origin,
    double line_height /* Multiples of font height */,
    Box2 text_field_bounds, /* Text field bounds */
    DefaultMesh& mesh){
        t.glyph_coords_.clear();
        float line_number = 0.f;
        float spacing = (float)(line_height * handle.second);

        for(auto&line : t.text_fields_){

            float x = origin[0];
            float y = origin[1] + line_number * spacing;

            float ymin = text_field_bounds.min_[1];
            float ymax = text_field_bounds.max_[1];

            if(!span_is_empty<float>(
                    intersect_spans<float>(Math<float>::span_t(ymin, ymax), Math<float>::span_t(y, y + (float) handle.second)))){

                GlyphCoords::row_coords_t tempcoords;

                context.write_pixel_coords_for_string(line->string, handle, x, y, text_field_bounds, tempcoords);

                GlyphCoords::row_coords_t::iterator first_quad = tempcoords.begin();
                GlyphCoords::row_coords_t::iterator last_quad = tempcoords.end();

                // TODO: Filter quads so they all fit in text_field_bounds. 
                // TODO: Then, figure out a better algorithm to do it

                if(first_quad != last_quad) t.glyph_coords_.emplace_back(first_quad, last_quad);
                
            }
            line_number += 1.0f;
            if(y > ymax) break;
        }

        transfer_position_data_to_mesh(t.glyph_coords_, &mesh);
}

void render_glyph_coordinates_to_mesh(FontContext& context, const std::string& row,
    BakedFontHandle handle, vec2 origin, DefaultMesh& mesh)
{
    GlyphCoords glyph_coords;
    glyph_coords.push_row();
    context.write_pixel_coords_for_string(row, handle, origin[0], origin[1], std::numeric_limits<float>::max(),glyph_coords.last());
    transfer_position_data_to_mesh(glyph_coords, &mesh);
}

// TODO to text util or such.
char us_ascii_tolower(char c){

#define IFN(param, target) case param: return target; break
        switch(c){
            IFN('~', '`');
            IFN('!', '1');
            IFN('@', '2');
            IFN('#', '3');
            IFN('$', '4');
            IFN('%', '5');
            IFN('^', '6');
            IFN('&', '7');
            IFN('*', '8');
            IFN('(', '9');
            IFN(')', '0');
            IFN('_', '-');
            IFN('+', '=');

            IFN('{', '[');
            IFN('}', ']');
            IFN(':', ';');
            IFN('"', '\'');
            IFN('|', '\\');
            IFN('<', ',');
            IFN('>', '.');
            IFN('?', '/');
        default:{
            if(c > 64 && c < 91){
                return c + 32;
            } else return c;
                }
        }
#undef IFN
    }

char us_ascii_toupper(char c){

#define IFN(param, target) case target: return param; break
    switch(c){
        IFN('~', '`');
        IFN('!', '1');
        IFN('@', '2');
        IFN('#', '3');
        IFN('$', '4');
        IFN('%', '5');
        IFN('^', '6');
        IFN('&', '7');
        IFN('*', '8');
        IFN('(', '9');
        IFN(')', '0');
        IFN('_', '-');
        IFN('+', '=');

        IFN('{', '[');
        IFN('}', ']');
        IFN(':', ';');
        IFN('"', '\'');
        IFN('|', '\\');
        IFN('<', ',');
        IFN('>', '.');
        IFN('?', '/');
    default:{    
        if(c > 96 && c < 123){
            return c - 32;
        } else return c;
            }
    }
#undef IFN
}

char key_to_char(int key, Modifiers modifiers){
    char c = 0;
    if(key < CHAR_MAX && key > 0) c = static_cast<char>(key);
    if(c) c = modifiers.shift_ ? us_ascii_toupper(c) : us_ascii_tolower(c);
    return c;
}


Movement key_to_movement(int key){
    if(key == Input::Left) return Movement::Left;
    else if(key == Input::Right) return Movement::Right;
    else if(key == Input::Up) return Movement::Up;
    else if(key == Input::Down) return Movement::Down;
    else return Movement::None;
}

void GlyphPane::move_cursor(Movement m){
    // TODO FIXME take into account visible area moves
    if(m == Movement::Up){
        cursor_index_visual_[1]--;
    }
    else if(m == Movement::Down){
        cursor_index_visual_[1]++;
    }
    else if(m == Movement::Left){
        if(cursor_index_visual_[0] > 0){
            cursor_index_visual_[0]--;

        } else if(cursor_index_visual_[1] > 0){
            cursor_index_visual_[1]--;

            // Moving rows. Snap index to beginning
            // TODO FIXME take into accoutn visual != actual
            // TODO add possible movement to bounds
            auto actual = cursor_index_actual();
            cursor_index_visual_[0] = text_field_.glyph_coords_.at(actual[1]).size();
        }
    }
    else if(m == Movement::Right){
        cursor_index_visual_[0]++;
    }
    else if(m == Movement::RightBound){
        // TODO FIXME better mapping between visual and actual coordinates
        // actual coordinates should be computed actually always from visual coordinates!
        auto actual = cursor_index_actual();
        cursor_index_visual_[0] = text_field_.glyph_coords_.at(actual[1]).size();
    }
    else if(m == Movement::LeftBound){
        cursor_index_visual_[0] = 0;
    }

    update_cursor_pos();
}

void GlyphPane::recieve_characters(int key, Modifiers modifiers)
{
    // TODO: For indices that are passed onto text field, add the
    // current row - take into account a technique to scroll the text.
    
    Movement movement = key_to_movement(key);
    char c = key_to_char(key, modifiers);

    if(key == Input::Space){
        c = ' ';
    }

    if(key == Input::Tab){
        // TODO
    }

    if(key == Input::Key::Enter){
        auto actual = cursor_index_actual();
        text_field_.break_line(actual[0], actual[1]);
        cursor_index_visual_[1]++;
        cursor_index_visual_[0] = 0;
        dirty_ = true;
    }
    else if(movement != Movement::None){
        if(modifiers.ctrl_)
        {
            if(movement == Movement::Left) movement = Movement::LeftBound;
            if(movement == Movement::Right) movement = Movement::RightBound;
        }
        move_cursor(movement);
    }
    else if(any_of(key, Input::Del, Input::Backspace)){
        if(text_field_.size() > 0){
            if(cursor_index_visual_[0] == 0){
                // At beginning of line before first letter
                if(cursor_index_visual_[1] > 0){
                    // Join lines.
                    auto actual = cursor_index_actual();
                    const int prev_row = actual[1] - 1;
                    const int rm_row = actual[1];
                    const int new_cursor_x = text_field_.at(prev_row).size();

                    text_field_.at(prev_row).string += text_field_.at(rm_row).string;

                    text_field_.erase_line(rm_row);

                    cursor_index_visual_[1]--;
                    cursor_index_visual_[0] = new_cursor_x;
                    dirty_ = true;
                }
            }
            else if(cursor_index_visual_[0] > 0){
                // If not at first position
                auto actual = cursor_index_actual();
                text_field_.at(actual[1]).string.erase(actual[0] - 1, 1);
                cursor_index_visual_[0]--;
                dirty_ = true;
            }
        }
    }
    else if(c){
        auto actual = cursor_index_actual();
        text_field_.insert_char_after(actual[0], actual[1], c);
        cursor_index_visual_[0]++;
        dirty_ = true;
    }
}


class TextLabelImpl{
public:

    TextLabelImpl(GraphicsManager* gm, const std::string& program_name, const std::string& background_program_name, FontManager* fontmanager, const std::string& pane_name)
        :gm_(gm), glyph_node_(0), line_height_(1.0), fontmanager_(fontmanager),
        font_handle_(invalid_font_handle()), dirty_(true){

        background_mesh_node_ = 0;
        parent_ = 0;
        pane_root_ = 0;
        glyph_node_ = 0;
        scene_ = 0;

        fontmesh_ = gm_->create_mesh();
        renderable_ = gm_->create_renderable();

        // Create background renderable and mesh
        auto background_program_handle = gm->program(background_program_name);
        background_renderable_ = gm_->create_renderable();
        background_renderable_->bind_program(*background_program_handle);

        background_mesh_ = gm->create_mesh();
        background_renderable_->set_mesh(background_mesh_);

        apply_layout({{100.f, 100.f}, {100.f, 300.f}});

        auto font_program_handle = gm->program(program_name);

        renderable_->bind_program(*font_program_handle);
        renderable_->set_mesh(fontmesh_);

        name_ = pane_name;
    }

    ~TextLabelImpl(){
        gm_->release_mesh(fontmesh_);
        gm_->release_renderable(renderable_);
        if(glyph_node_ && parent_ && scene_){
            parent_->remove_child(glyph_node_);
            scene_->finalize(glyph_node_);
        }
    }


    double height_of_nth_row(int i_visual){
        return (lineheight_screenunits() * (i_visual));
    }
    double glypheight(){ return font_handle_.second; }
    double lineheight_screenunits(){ return line_height_ * glypheight(); }
    vec2 glyphs_origin(){ return vec2(0.f, glypheight()); }


    SceneTree::Node* root(){ return pane_root_; }

    virtual const std::string& name() const { return name_; }
    virtual EntityType::t entity_type() const  { return EntityType::GlyphPane; }

    virtual void apply_layout(const Layout& l) {
        Layout oldlayout = layout_;

        layout_ = l;
        text_field_bounds_ = Box2(vec2(0.f, 0.f), layout_.size_);

        if(pane_root_){
            pane_root_->transform_.position_ = increase_dim(layout_.origin_, 0.f);
        }

        if(background_mesh_node_){
            vec2 backround_half = 0.5f * text_field_bounds_.size();
            background_mesh_node_->transform_.position_ = increase_dim(backround_half, 0.f);
        }

        if(oldlayout.size_ != layout_.size_)
        {
            update_background_mesh();
        }

        // need to udpate text field as well!
        update_representation();
    }

    void update_background_mesh(){
        load_screenquad(text_field_bounds_.size(), *background_mesh_);

        if(background_mesh_node_){
            background_mesh_node_->renderable_->resend_data_on_render();
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

    void attach(SceneTree* scene, SceneTree::Node* parent){
        parent_ = parent;
        scene_ = scene;

        pane_root_ = scene->add_node(parent);
        pane_root_->name_ = name_ + std::string("/Root");
        pane_root_->transform_.position_ = increase_dim(layout_.origin_, 0.f);

        background_mesh_node_ = scene->add_node(pane_root_, background_renderable_);
        background_mesh_node_->name_ = name_ + std::string("/Background");

        glyph_node_ = scene->add_node(pane_root_, renderable_);
        glyph_node_->name_ = name_ + std::string("/Glyph");

        apply_layout(layout_);

    }

    void update_representation()
    {
        if(!scene_) return;

        // TODO take layout_ and current top row into account when assigning glyph coordinates
        if(dirty_ && handle_valid(font_handle_)){
            vec2 default_origin = glyphs_origin();
            render_glyph_coordinates_to_mesh(fontmanager_->context_, text_field_, font_handle_, default_origin, line_height_, text_field_bounds_, *fontmesh_);
            renderable_->resend_data_on_render();
        }

        dirty_ = false;
    }

    Texture*         fonttexture_;
    SceneTree*       scene_;

    SceneTree::Node* background_mesh_node_; //> The background for highlights.
    SceneTree::Node* parent_;
    SceneTree::Node* pane_root_;
    SceneTree::Node* glyph_node_;

    Box2             text_field_bounds_;

    FontManager*     fontmanager_;
    DefaultMesh*     fontmesh_;
    DefaultMesh*     background_mesh_;

    FullRenderable*  renderable_;
    FullRenderable*  background_renderable_;

    GraphicsManager* gm_;
    BakedFontHandle  font_handle_;
    double           line_height_; // Multiple of font height

    TextField        text_field_;

    std::string      name_;

    bool             dirty_;

    Layout           layout_;
};


}
