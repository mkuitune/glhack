/**\file glh_scene_extensions.h
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
    std::vector<std::shared_ptr<TextLine>> text_fields_;

    GlyphCoords glyph_coords_;

    float line_height_; /* Multiples of font size. */

    TextLine& push_line(const std::string& string){
        const size_t line_pos = text_fields_.size();
        text_fields_.emplace_back(std::make_shared<TextLine>(string, line_pos));
        return *text_fields_.back().get();
    }

    TextLine& last(){
        return *text_fields_.back().get();
    }

    size_t size(){return text_fields_.size();}

    void erase(TextLine& t){
        auto e = find_if(text_fields_, [&](std::shared_ptr<TextLine>& ptr){return ptr.get() == &t;});
        if(e != text_fields_.end()) text_fields_.erase(e);
    }

    bool empty(){ return text_fields_.size() == 0 || text_fields_.size() == 1 && text_fields_[0]->size() == 0; }
};

void render_glyph_coordinates_to_mesh(FontContext& context, TextField& t,
    BakedFontHandle handle, vec2 origin, double line_height /* Multiples of font height */,
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
// - add mesh for row background
// - embed Glyphpane in specific shape, bind to background node (outside glyphpane)
// - add a mesh for cursor
// - add a datum for cursor, an interface for moving it (accept mouse position, accept
//   cursor key, etc.
// - 

enum class Movement{ Up, Down, Left, Right, None };

class GlyphPane: public SceneObject
{
public:

    GlyphPane(GraphicsManager* gm, const std::string& program_name, const std::string& background_program_name, FontManager* fontmanager, const std::string& pane_name)
        :gm_(gm), glyph_node_(0), line_height_(1.0), fontmanager_(fontmanager),
        font_handle_(invalid_font_handle()), dirty_(true){

        text_field_bounds_ = Box2f(vec2(100.f, 100.f), vec2(400.f, 500.f));

        default_origin_ = vec2(0.f, 0.f);
        fontmesh_ = gm_->create_mesh();
        renderable_ = gm_->create_renderable();

        cursor_mesh_ = gm->create_mesh();
        cursor_renderable_ = gm_->create_renderable();

        auto background_program_handle = gm->program(background_program_name);
        background_renderable_ = gm_->create_renderable();
        background_renderable_->bind_program(*background_program_handle);
        background_mesh_ = gm->create_mesh();
        load_screenquad(text_field_bounds_.size(), *background_mesh_);
        background_renderable_->set_mesh(background_mesh_);

        auto font_program_handle = gm->program(program_name);

        renderable_->bind_program(*font_program_handle);
        renderable_->set_mesh(fontmesh_);

        cursor_renderable_->bind_program(*font_program_handle);
        cursor_renderable_->set_mesh(cursor_mesh_);

        name_ = pane_name;

        cursor_pos_index_ = vec2i(0, 0);
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

    float height_of_nth_row(int i){ return (float) (line_height_ * font_handle_.second * (i)); }

    //void update_cursor_pos_to_end_of_line()
    //{

    //    cursor_pos_index_[0] = 0;
    //    cursor_pos_index_[1] = 0;

    //    if(!text_field_.glyph_coords_.empty()){

    //        // there are one more positions for the cursor than there are glyphs per row
    //        cursor_pos_index_[0] = text_field_.glyph_coords_.last().size();
    //        cursor_pos_index_[1] = text_field_.glyph_coords_.size();

    //        std::tuple<vec2, vec2> last;

    //        if(!text_field_.glyph_coords_.last().empty())
    //            last = text_field_.glyph_coords_.pos(cursor_pos_index_[1]);
    //        else
    //            last = std::make_tuple(vec2(0.f, 0.f), vec2(0.f, 0.f));

    //        float cursor_y = height_of_nth_row(text_field_.text_fields_.size() - 1);

    //        cursor_pos_[0] = std::get<0>(last)[0];
    //        //cursor_pos[1] = std::get<0>(last)[1];
    //        cursor_pos_[1] = cursor_y;
    //    }
    //    else{
    //        cursor_pos_[0] = default_origin_[0];
    //        cursor_pos_[1] = default_origin_[1];
    //    }

    //    update_cursor_pos();
    //}

    //void update_cursor_pos(){

    //    cursor_node_->transform_.position_ = cursor_pos_;
    //}

    void update_cursor_pos_to_end_of_line()
    {
        cursor_pos_index_[0] = 0;
        cursor_pos_index_[1] = 0;

        if(!text_field_.glyph_coords_.empty()){
            // there are one more positions for the cursor than there are glyphs per row
            cursor_pos_index_[0] = text_field_.glyph_coords_.last().size()/4;
            cursor_pos_index_[1] = text_field_.glyph_coords_.size() - 1;
        }

        update_cursor_pos();
    }

    void update_cursor_pos(){
        GlyphCoords& coords(text_field_.glyph_coords_);

        cursor_pos_[1] = height_of_nth_row(cursor_pos_index_[1]);
        cursor_pos_[0] = default_origin_[0];

        if(cursor_pos_index_[0] > 0){
            std::tuple<vec2, vec2> loc = coords.pos(cursor_pos_index_[0], cursor_pos_index_[1]);
            cursor_pos_[0] = std::get<0>(loc)[0];
        }

        cursor_node_->transform_.position_ = cursor_pos_;
    }

    vec2i limit_to_valid_visual_row_indices(const vec2i& ind){
        int x = ind[0], y = ind[1];

        int max_height = text_field_.glyph_coords_.size() - 1;
        y = constrain(y, 0, max_height);

        int row_len = text_field_.glyph_coords_.row_len(y) / 4;
        x = constrain(x, 0, row_len);

        return vec2i(x, y);
    }

    void move_cursor(Movement m){
        if(m == Movement::Up)         cursor_pos_index_[1]--;
        else if(m == Movement::Down)  cursor_pos_index_[1]++;
        else if(m == Movement::Left)  cursor_pos_index_[0]--;
        else if(m == Movement::Right) cursor_pos_index_[0]++;

        cursor_pos_index_ = limit_to_valid_visual_row_indices(cursor_pos_index_);

        update_cursor_pos();
    }

    void init_cursor()
    {
        // Init cursor
        if(handle_valid(font_handle_)){
            render_glyph_coordinates_to_mesh(fontmanager_->context_, "|", font_handle_, default_origin_, *cursor_mesh_);
            //render_glyph_coordinates_to_mesh(fontmanager_->context_, "THIS IS A LONG DEBUG STRING", font_handle_, default_origin_, *cursor_mesh_);
            cursor_renderable_->resend_data_on_render();
        }

        update_cursor_pos_to_end_of_line();
    }

    SceneTree::Node* root(){ return pane_root_; }

    virtual const std::string& name() const override  {return name_;}
    virtual EntityType::t entity_type() const override  {return EntityType::GlyphPane;}

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

        background_mesh_node_ = scene->add_node(pane_root_, background_renderable_);
        background_mesh_node_->name_ = name_ + std::string("/Background");

        vec2 backround_half = 0.5f * text_field_bounds_.size();
        background_mesh_node_->transform_.position_ = vec3(backround_half[0], backround_half[1], 0.f);

        cursor_node_ = scene->add_node(pane_root_, cursor_renderable_);
        cursor_node_->name_ = name_ + std::string("/Cursor");

        glyph_node_ = scene->add_node(pane_root_, renderable_);
        glyph_node_->name_ = name_ + std::string("/Glyph");
        //init_background();
    }

    void recieve_characters(int key, Modifiers modifiers);

    void update_representation()
    {
        if(dirty_ && handle_valid(font_handle_)){
            vec2 default_origin(0.f, 0.f);
            render_glyph_coordinates_to_mesh(fontmanager_->context_, text_field_, font_handle_, default_origin, line_height_, *fontmesh_);
            renderable_->resend_data_on_render();

            update_cursor_pos_to_end_of_line();
        }

        dirty_ = false;
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

    vec2i            cursor_pos_index_;
    vec3             cursor_pos_;

    vec2             default_origin_; // TODO do we need this when layout finished

    Box2f            text_field_bounds_;

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
};


class SceneAssets{
public:

    SceneTree tree_; // TODO: Split SceneTree from SceneAssets?

    std::list<GlyphPane>                                glyph_panes_;
    std::list<Camera> cameras_;
    std::list<RenderPass>                               render_passes_;

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
