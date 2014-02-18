/**\file glh_scene_extensions.h
    \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/
#pragma once

#include "glh_font.h"
#include "glh_scenemanagement.h"
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


class GlyphPane : public SceneObject
{
public:

    GlyphPane(GraphicsManager* gm, const std::string& program_name, FontManager* fontmanager)
        :gm_(gm), node_(0), line_height_(1.0),fontmanager_(fontmanager),
        origin_(0.f, 0.f), font_handle_(invalid_font_handle()), dirty_(true){
        fontmesh_ = gm_->create_mesh();
        renderable_= gm_->create_renderable();

        auto font_program_handle = gm->program(program_name);

        renderable_->bind_program(*font_program_handle);
        renderable_->set_mesh(fontmesh_);

        init_background();
    }

    ~GlyphPane(){
        gm_->release_mesh(fontmesh_);
        gm_->release_renderable(renderable_);
        if(node_ && parent_ && scene_){
            parent_->remove_child(node_);
            scene_->finalize(node_);
        }
    }

    void init_background()
    {

    }

    virtual const std::string& name() const override  {return name_;}
    virtual EntityType::t entity_type() const override  {return EntityType::GlyphPane;}

    void set_font(const std::string& name, float size){
        FontConfig config = get_font_config(name, size);
        FontContext& context(fontmanager_->context_);
        font_handle_ = context.render_bitmap(config); // May throw GraphicsException
        // fontimage_ = context.get_font_map(font_handle_);
        //write_image_png(*fontimage, "font.png");
        fonttexture_ = fontmanager_->get_font_texture(font_handle_);
    }

    void attach(SceneTree* scene, SceneTree::Node* parent){
        node_ = scene->add_node(parent, renderable_);
        parent_ = parent;
        scene_ = scene;
    }

    void recieve_characters(int key, Modifiers modifiers);

    void update_representation()
    {
        if(handle_valid(font_handle_)){
            render_glyph_coordinates_to_mesh(fontmanager_->context_, text_field_, font_handle_, origin_, line_height_, *fontmesh_);
            renderable_->resend_data_on_render();
        }

        dirty_ = false;
    }


    // TODO add handle mouse click method. Gets x,y in screen coordinates. Invert the local matrix and 
    // multiply coordinates to get them in the local coordinates for this element. See which row
    // and colums this hit and place cursor accordingly.

    // TODO Paint option along with copy and paste to clipboard.


    Texture*         fonttexture_;
    SceneTree*       scene_;
    SceneTree::Node* parent_;
    SceneTree::Node* node_;

    // TODO: Implement background texture and texture painting routines to
    // implement the highlight and cursor
    SceneTree::Node* background_mesh; //> The background for highlights.
    SceneTree::Node* cursor;          //> The 

    FontManager*     fontmanager_;
    DefaultMesh*     fontmesh_;
    GraphicsManager* gm_;
    vec2             origin_;
    BakedFontHandle  font_handle_;
    double           line_height_;
    FullRenderable*  renderable_;
    TextField        text_field_;

    std::string      name_;

    bool             dirty_;
};


class SceneAssets{
public:

    SceneTree tree_; // TODO: Split SceneTree from SceneAssets?

    std::list<GlyphPane>  glyph_panes_;
    std::list<Camera>     cameras_;
    std::list<RenderPass> render_passes_;

    GraphicsManager* gm_;
    FontManager*     font_manager_;
    App*             app_;


    // TODO: App.env: a wrapper for all addressable assets in the app.
    // Contains references to the lower level constructs so it can refer queries onwards, such as
    // "scenes/tree1/node3" would return a pointer with the object type (SceneTreeNode) 

    GlyphPane* create_glyph_pane(const std::string& name, const std::string& program_name){
        glyph_panes_.emplace_back(gm_, program_name, font_manager_);
        GlyphPane* pane = &glyph_panes_.back();
        pane->name_ = name;
        return pane;
    }

     Camera* create_camera(const std::string& name){
         cameras_.emplace_back();
         auto camera = &cameras_.back();
         camera->name_ = name;
         return camera;
    }

    RenderPass* create_render_pass(const std::string& name){
        render_passes_.emplace_back();
        RenderPass* pass = &render_passes_.back();
        pass->name_ = name;
        return pass;
    }

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
