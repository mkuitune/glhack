/**\file glh_scene_textdisplay.h
    \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/
#pragma once

#include "glh_font.h"
#include "glh_scenemanagement.h"
#include "glh_scene_util.h"
#include "shims_and_types.h"

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

typedef std::unique_ptr<FontManager> FontManagerPtr;

/** Collection of text fields and their rendering options. */
class TextField {
public:
    typedef std::shared_ptr<TextLine> TextLinePtr;
    std::vector<TextLinePtr> text_fields_;

    GlyphCoords glyph_coords_;

    float line_height_; /* Multiples of font size. */

    TextLine& push_line(const std::string& string){
        const size_t line_pos = text_fields_.size();
        text_fields_.emplace_back(std::make_shared<TextLine>(string, line_pos));
        return *text_fields_.back().get();
    }

    void update_line_numbers(){
        for(int i = 0; i < (int) text_fields_.size(); i++) text_fields_[i]->line_number = i;
    }

    /** Insert new line after 'char_pos' at 'row'. */
    TextLine& break_line(const int char_pos, const int row){

        if(row >= (int) size()){
            return push_line("");
        }
        else if(char_pos == 0){
            auto line = std::make_shared<TextLine>("", row + 1);
            insert_before(text_fields_, row, line);
            update_line_numbers();
            return *line;
        }
        else{
            auto new_head = head_to(text_fields_[row]->string, char_pos);
            auto new_tail = tail_from(text_fields_[row]->string, char_pos);

            text_fields_[row]->string = new_head;

            auto line = std::make_shared<TextLine>(new_tail, row + 1);
            insert_after(text_fields_, row, line);
            update_line_numbers();
            return *line;
        }
    }

    void erase_line(const int row){
        if(row < (int) text_fields_.size()){
            remove_at(text_fields_, row);
            update_line_numbers();
        }
    }

    /** Insert new character to line after char_pos. */
    void insert_char_after(const int char_pos, const int row, const char c){
        if(row < (int) text_fields_.size() && row >= 0){
            std::string& str(text_fields_[row]->string);
            if(char_pos <= (int) str.size() && char_pos >= 0){
                if(char_pos == str.size())
                    str.push_back(c);
                else
                    str.insert(char_pos, 1, c);
            }
        }
    }

    TextLine& last(){
        return *text_fields_.back().get();
    }

    TextLine& at(int i){
        return *text_fields_.at(i);
    }

    size_t size(){return text_fields_.size();}

    void erase(TextLine& t){
        auto e = find_if(text_fields_, [&](std::shared_ptr<TextLine>& ptr){return ptr.get() == &t;});
        if(e != text_fields_.end()) text_fields_.erase(e);
    }

    bool empty(){ return text_fields_.size() == 0 || text_fields_.size() == 1 && text_fields_[0]->size() == 0; }
};

void render_glyph_coordinates_to_mesh(FontContext& context, TextField& t,
    BakedFontHandle handle, vec2 origin, double line_height /* Multiples of font height */, Box2 text_field_bounds,
    DefaultMesh& mesh);

void render_glyph_coordinates_to_mesh(FontContext& context, const std::string& row,
    BakedFontHandle handle, vec2 origin, DefaultMesh& mesh);

/** Encodes the modifiers state for interpreting input streams. */
struct Modifiers
{
    bool shift_;
    bool ctrl_;
    Modifiers():shift_(false), ctrl_(false){}
};

//TODO:
// - figure out a better way to implement glyph pane
// - implement a TextLabel that displays a row of text and is not as complex as glyphpane

enum class Movement{ Up, Down, Left, Right, LeftBound, RightBound, None };

class GlyphPane: public SceneObject
{
public:

    GlyphPane(GraphicsManager* gm, const std::string& program_name, const std::string& background_program_name, FontManager* fontmanager, const std::string& pane_name)
        :gm_(gm), glyph_node_(0), line_height_(1.0), fontmanager_(fontmanager),
        font_handle_(invalid_font_handle()), dirty_(true){

        background_mesh_node_ = 0;
        cursor_node_ = 0;
        parent_ = 0;
        pane_root_ = 0;
        glyph_node_ = 0;
        scene_ = 0;

        fontmesh_ = gm_->create_mesh();
        renderable_ = gm_->create_renderable();

        cursor_mesh_ = gm->create_mesh();
        cursor_renderable_ = gm_->create_renderable();

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

        cursor_renderable_->bind_program(*font_program_handle);
        cursor_renderable_->set_mesh(cursor_mesh_);

        name_ = pane_name;

        cursor_index_visual_ = vec2i(0, 0);
        cursor_index_origin_ = vec2i(0, 0);

        cursor_pos_ = vec3(0.0, 0.0, 0.0);
    }

    ~GlyphPane(){
        gm_->release_mesh(fontmesh_);
        gm_->release_renderable(renderable_);
        if(glyph_node_ && parent_ && scene_){
            parent_->remove_child(glyph_node_);
            scene_->finalize(glyph_node_);
        }
    }

    //vec2i get_actual_cursor_index(){
    //    // TODO FIXME implement movement
    //    return ;
    //}

    double height_of_nth_row(int i_visual){
        // TODO FIXME verify this is ok
        return (lineheight_screenunits() * (i_visual));
    }
    double glypheight(){ return font_handle_.second; }
    double lineheight_screenunits(){ return line_height_ * glypheight(); }
    vec2 glyphs_origin(){ return vec2(0.f, glypheight()); }

    void update_cursor_pos_to_end_of_line()
    {
        cursor_index_visual_ = vec2i(0, 0);

        if(!text_field_.glyph_coords_.empty()){
            // there are one more positions for the cursor than there are glyphs per row
            // TODO FIXME

            cursor_index_visual_ = vec2i(text_field_.glyph_coords_.last().size(),
                                         text_field_.glyph_coords_.size() - 1);

        }

        update_cursor_pos();
    }

    /** Set position of cursor. Note! Call only after text_field_ has updated the
    *   glyph coordinates.
    *   Cursor moves in screen indices of the glyphs. 
    *   Changes to layout are triggered when cursor reaches left, right, top or bottom
    *   extremum of the visible glyphs. Then if there are glyphs in the indicated
    *   extremum direction of the TextField left, move the text field bounds
    *   one visible glyph spacing to the direction. In vertical movement the
    *   text field bounds should be locked to the topmost visible row. In
    *   horizontal movement peek the location of the previous glyph and move
    *   the text field to that.
    *
    */
    void update_cursor_pos(){
        GlyphCoords& coords(text_field_.glyph_coords_);

        // TODO FIXME
        cursor_index_visual_ = limit_to_valid_visual_row_indices(cursor_index_visual_);

        // TODO FIXME make sure 
        cursor_pos_[1] = (float) height_of_nth_row(cursor_index_visual_[1]);
        cursor_pos_[0] = text_field_bounds_.min_[0];

        if(cursor_index_visual_[0] > 0){
            quad2d_coord_t& loc = coords.pos(cursor_index_visual_[0] - 1, cursor_index_visual_[1]);

            // TODO 
            int line_len = coords.row_len(cursor_index_visual_[1]);

            if(cursor_index_visual_[0] < line_len){
                quad2d_coord_t& loc2 = coords.pos(cursor_index_visual_[0], cursor_index_visual_[1]);
                cursor_pos_[0] = average(max(loc)[0], min(loc2)[0]);
            }
            else
            {
                cursor_pos_[0] = max(loc)[0];
            }
        }

        cursor_node_->transform_.position_ = cursor_pos_;
    }

    vec2i limit_to_valid_visual_row_indices(const vec2i& ind){
        int x = ind[0], y = ind[1];

        int max_height = text_field_.glyph_coords_.size() - 1;
        y = constrain(y, 0, max_height);

        int row_len = text_field_.glyph_coords_.row_len(y);
        x = constrain(x, 0, row_len);

        return vec2i(x, y);
    }

    void move_cursor(Movement m);

    void init_cursor()
    {
        // Init cursor
        if(handle_valid(font_handle_)){

            vec2 coordinate_origin = glyphs_origin();
            render_glyph_coordinates_to_mesh(fontmanager_->context_, "|", font_handle_, coordinate_origin, *cursor_mesh_);
            //render_glyph_coordinates_to_mesh(fontmanager_->context_, "THIS IS A LONG DEBUG STRING", font_handle_, default_origin_, *cursor_mesh_);
            cursor_renderable_->resend_data_on_render();
        }

        update_cursor_pos_to_end_of_line();
    }

    SceneTree::Node* root(){ return pane_root_; }

    virtual const std::string& name() const override  {return name_;}
    virtual EntityType::t entity_type() const override  {return EntityType::GlyphPane;}

    virtual void apply_layout(const Layout& l) override {
        Layout oldlayout = layout_ ;

        layout_ = l;
        text_field_bounds_ = Box2(vec2(0.f, 0.f), layout_.size_);

        if(pane_root_){
            pane_root_->transform_.position_ = increase_dim(layout_.origin_, 0.f);
        }

        if(background_mesh_node_){
            vec2 backround_half = 0.5f * text_field_bounds_.size();
            background_mesh_node_->transform_.position_ = increase_dim(backround_half, 0.f);
        }

        if(oldlayout.size_  != layout_.size_)
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

        init_cursor();
    }

    void attach(SceneTree* scene, SceneTree::Node* parent){
        parent_ = parent;
        scene_ = scene;

        pane_root_ = scene->add_node(parent);
        pane_root_->name_ = name_ + std::string("/Root");
        pane_root_->transform_.position_ = increase_dim(layout_.origin_, 0.f);

        background_mesh_node_ = scene->add_node(pane_root_, background_renderable_);
        background_mesh_node_->name_ = name_ + std::string("/Background");

        cursor_node_ = scene->add_node(pane_root_, cursor_renderable_);
        cursor_node_->name_ = name_ + std::string("/Cursor");

        glyph_node_ = scene->add_node(pane_root_, renderable_);
        glyph_node_->name_ = name_ + std::string("/Glyph");

        apply_layout(layout_);

        //init_background();
    }

    void recieve_characters(int key, Modifiers modifiers);

    void update_representation()
    {
        if(!scene_) return;

        // TODO take layout_ and current top row into account when assigning glyph coordinates
        if(dirty_ && handle_valid(font_handle_)){
            vec2 default_origin = glyphs_origin();
            render_glyph_coordinates_to_mesh(fontmanager_->context_, text_field_, font_handle_, default_origin, line_height_, text_field_bounds_,*fontmesh_);
            renderable_->resend_data_on_render();
        }

        update_cursor_pos();
        dirty_ = false;
    }

    /** Get actual cursor index. */
    vec2i cursor_index_actual(){
        // TODO FIXME use proper mapping
        return  cursor_index_visual_ + cursor_index_origin_;
    }

    // TODO add handle mouse click method. Gets x,y in screen coordinates. Invert the local matrix and 
    // multiply coordinates to get them in the local coordinates for this element. See which row
    // and colums this hit and place cursor accordingly.

    // TODO Paint option along with copy and paste to clipboard.

    Texture*         fonttexture_;
    SceneTree*       scene_;

    // TODO: Implement background texture and texture painting routines to
    // implement the highlight and cursor
    // other option to render highlight: use character mesh. Render character mesh to back with opaque color.
    SceneTree::Node* background_mesh_node_; //> The background for highlights.
    SceneTree::Node* cursor_node_;          //> The cursor node
    SceneTree::Node* parent_;
    SceneTree::Node* pane_root_;
    SceneTree::Node* glyph_node_;

    vec2i            cursor_index_visual_;
    vec2i            cursor_index_origin_;

    vec3             cursor_pos_;

    Box2            text_field_bounds_;

    FontManager*     fontmanager_;
    DefaultMesh*     fontmesh_;
    DefaultMesh*     cursor_mesh_;
    DefaultMesh*     background_mesh_;

    FullRenderable*  cursor_renderable_;
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


class TextLabelImpl;
/** class TextLabel.*/
class TextLabel : public SceneObject{
public:

    TextLabel(GraphicsManager* gm, const std::string& program_name, const std::string& background_program_name, FontManager* fontmanager, const std::string& pane_name)
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

    ~TextLabel(){
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

    virtual const std::string& name() const override  { return name_; }
    virtual EntityType::t entity_type() const override  { return EntityType::GlyphPane; }

    virtual void apply_layout(const Layout& l) override {
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


    TextLabelImpl* impl_;
};

}
