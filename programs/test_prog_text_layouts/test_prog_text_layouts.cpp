/** 
Glyph pane tester.
*/

#include "glh_app_services.h"

#include <cctype>
#include <vector>

// App state

bool g_run = true;

float tprev = 0;
float angle = 0.f;
float radial_speed = 0.0f * PIf;

glh::AppServices  services;
glh::RenderPass*  render_pass;

glh::Camera*      camera;

glh::GlyphPane* glyph_pane;

glh::TextLabel* text_label;


namespace glh{

// Container for nodes that have layout applied to them with a specific rule system.
struct LayoutGroup : public glh::SceneObject
{
    std::vector<SceneObject*> objects_;
    Layout layout_;

    LayoutGroup(const std::string& group_name):group_name_(group_name){}

    virtual const std::string& name() const override{ return group_name_; };
    virtual EntityType::t entity_type() const override { return EntityType::LayoutGroup; };
    virtual void apply_layout(const Layout& l) override{ layout_ = l; };

    /** Apply rule to contained layouts. */
    void apply(){
        vec2 origin = {0.f, 0.f};
        vec2 size = {200.f, 50.f};
        vec2 delta = {0.f, 50.f};
        Layout layout = {origin, size};

        for(auto& obj : objects_){

        }

    }

    std::string group_name_;

};

}

glh::LayoutGroup pane_group("LayoutGroup");


void load_first_glyph_pane(glh::GraphicsManager* gm)
{
    using namespace glh;

    std::string old_goudy("OFLGoudyStMTT.ttf");
    std::string junction("Junction-webfont.ttf");

    float fontsize = 15.0f;
    //float fontsize = 10.0f;

    glyph_pane = services.assets().create_glyph_pane(GLH_TEXTURED_FONT_PROGRAM_SOLID_FILL, GLH_CONSTANT_ALBEDO_PROGRAM);

    glyph_pane->set_font(junction, fontsize);

    glyph_pane->text_field_.push_line("Hello, world.");
    glyph_pane->text_field_.push_line("This is another line, then.");
    glyph_pane->dirty_ = true;
    glyph_pane->update_representation();

    glyph_pane->cursor_node_->material_.set_vec4(GLH_COLOR_ALBEDO, glh::vec4(0.f, 0.f, 0.f, 1.f));
    glyph_pane->background_mesh_node_->material_.set_vec4(GLH_COLOR_ALBEDO, glh::vec4(0.35f, 0.351f, 0.351f, 1.f));

    // TODO: View layouter that aligns glyph_pane
}

void load_first_text_label(glh::GraphicsManager* gm)
{
    using namespace glh;

    std::string old_goudy("OFLGoudyStMTT.ttf");
    std::string junction("Junction-webfont.ttf");

    float fontsize = 15.0f;
    //float fontsize = 10.0f;

    text_label = services.assets().create_text_label(GLH_TEXTURED_FONT_PROGRAM_SOLID_FILL, GLH_CONSTANT_ALBEDO_PROGRAM);

    text_label->set_font(junction, fontsize);

    text_label->text_field_.push_line("Hello, world.");
    text_label->text_field_.push_line("This is another line, then.");
    text_label->dirty_ = true;
    text_label->update_representation();

    text_label->background_mesh_node_->material_.set_vec4(GLH_COLOR_ALBEDO, glh::vec4(0.35f, 0.351f, 0.451f, 1.f));

    // TODO: View layouter that aligns glyph_pane
}

void load_glyph_pane_group(glh::GraphicsManager* gm)
{
    // This will create a glyph pane group with selectable rows, that highlight
    // when mouse cursor is on top of them and that when are clicked will throw
    // a callback.

}

bool init(glh::App* app)
{
    const char* config_file = "config.mp";

    services.init(app, config_file);

    glh::GraphicsManager* gm = app->graphics_manager();


    load_first_glyph_pane(gm);
    load_first_text_label(gm);

    render_pass = services.assets_->create_render_pass();

    render_pass->env_.set_vec4(GLH_COLOR_ALBEDO, glh::vec4(0.28f, 0.024f, 0.024f, 1.0));
    render_pass->env_.set_texture2d("Sampler", glyph_pane->fonttexture_);// todo: then env.set "Sampler" texture into glyph_pane TODO move this into GlyphPane

    render_pass->add_settings(glh::RenderPassSettings(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT,
        glh::Color(0.5f, 0.5f, 0.5f, 1.0f), 1));
    render_pass->add_settings(glh::RenderPassSettings(glh::RenderPassSettings::BlendSettings(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)));

    camera = services.assets_->create_camera(glh::Camera::Projection::PixelSpaceOrthographic);

    render_pass->set_camera(camera);

    return true;
}

Eigen::Affine3f rotating_transform(glh::App* app){
    float t = (float) app->time();
    angle += (t - tprev) * radial_speed;
    tprev = t;
    Eigen::Affine3f transform;
    transform = Eigen::AngleAxis<float>(angle, glh::vec3(0.f, 0.f, 1.f));
    return transform;
}

bool update(glh::App* app)
{
    using namespace glh;

    services.update();

    rotating_transform(app);

    mat4 screen_to_view = app_orthographic_pixel_projection(app); 

    float albedo_alpha = 1.0f;
    render_pass->env_.set_vec4(GLH_COLOR_ALBEDO, glh::vec4(0.28f, 0.024f, 0.024f, albedo_alpha));

    glyph_pane->glyph_node_->material_.set_vec4(GLH_COLOR_ALBEDO, glh::vec4(1.f, 1.f, 1.f, 1.f));

    float cursor_albedo = (float) abs(sin(5.0 * app->time()));
    glyph_pane->cursor_node_->material_.set_vec4(GLH_COLOR_ALBEDO, glh::vec4(0.f, 0.f, 0.f, cursor_albedo));

    Layout l = {{300.f, 10.f}, {300.f, 300.f}};

    glyph_pane->apply_layout(l);

    Layout l2 = {{10.f, 10.f}, {300.f, 100.f}};

    text_label->apply_layout(l2);



    render_pass->set_queue_filter(glh::pass_all);
    render_pass->update_queue_filtered(services.assets_->tree_);

    return g_run;
}

void render_font(glh::App* app)
{
    // TODO FIX renderpass settings, non-obvious
    glh::GraphicsManager* gm = app->graphics_manager();

    render_pass->render(gm);
}

void resize(glh::App* app, int width, int height)
{
    std::cout << "Resize:" << width << " " << height << std::endl;
}

int g_mouse_x;
int g_mouse_y;

void mouse_move_callback(int x, int y)
{
    g_mouse_x = x;
    g_mouse_y = y;

    std::cout << "Mouse:"  << x << " " << y << std::endl;
}

glh::Modifiers modifiers;

void key_callback(int key, const glh::Input::ButtonState& s)
{
    using namespace glh;

    if(key == Input::Esc) { g_run = false;}
    if(key == Input::Lshift || key == Input::Rshift){
        if(s == glh::Input::Held)
            modifiers.shift_ = true;
        else modifiers.shift_ = false;
    }
    if(key == Input::Rctrl || key == Input::Lctrl){
        if(s == glh::Input::Held)
            modifiers.ctrl_ = true;
        else modifiers.ctrl_ = false;
    }
    else if(key == Input::Left){ radial_speed -= 0.1f * PIf;}
    else if(key == Input::Right){ radial_speed += 0.1f * PIf;}

    // TODO: Add repeat functionality for characters fed into glyph pane.

    if(s == Input::Held){
        glyph_pane->recieve_characters(key, modifiers);
    }
}

int main(int arch, char* argv)
{
    glh::AppConfig config = glh::app_config_default(init, update, render_font, resize);

    config.width = 1024;
    config.height = 640;

    glh::App app(config);

    GLH_LOG_EXPR("Logger started");
    add_key_callback(app, key_callback);
    add_mouse_move_callback(app, mouse_move_callback);
    glh::default_main(app);
    services.finalize();
    return 0;
}
