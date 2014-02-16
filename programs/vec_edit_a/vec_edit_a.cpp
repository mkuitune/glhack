/** 


*/


#include "glbase.h"
#include "asset_manager.h"
#include "glh_generators.h"
#include "glh_image.h"
#include "glh_typedefs.h"
#include "shims_and_types.h"
#include "glh_scene_extensions.h"
#include "iotools.h"
#include <cctype>
#include <vector>

//////////////// Program stuff ////////////////

const char* sh_vertex   = 
"#version 150               \n"
"in vec3 VertexPosition;    "
"in vec3 VertexColor;       "
"out vec3 Color;            "
"void main()                "
"{                          "
"    Color = VertexColor;   "
"    gl_Position = vec4( VertexPosition, 1.0 );"
"}";

#define OBJ2WORLD "ObjectToWorld"

#define CANVASTOCREEN "CanvasToScreen"

const char* sh_vertex_obj   = 
"#version 150               \n"
"uniform mat4 ObjectToWorld;"
"in vec3      VertexPosition;    "
"in vec3      VertexColor;       "
"out vec3 Color;            "
"void main()                "
"{                          "
"    Color = VertexColor;   "
"    gl_Position = ObjectToWorld * vec4( VertexPosition, 1.0 );"
"}";

const char* sh_vertex_obj_pos   = 
"#version 150               \n"
"uniform mat4 ObjectToWorld;"
"in vec3      VertexPosition;    "
"void main()                "
"{                          "
"    gl_Position = ObjectToWorld * vec4( VertexPosition, 1.0 );"
"}";

const char* sh_vertex_obj_tex   = 
"#version 150               \n"
"uniform mat4 ObjectToWorld;"
"in vec3      VertexPosition;    "
"in vec3      TexCoord;"
"out vec3 v_color;            "
"out vec2 v_texcoord;"
"void main()                "
"{                          "
"    v_texcoord = TexCoord.xy;"
"    gl_Position = ObjectToWorld * vec4( VertexPosition, 1.0 );"
"}";


const char* sh_fragment = 
"#version 150                  \n"
"in vec3 Color;                "
"out vec4 FragColor;           "
"void main() {                 "
"    FragColor = vec4(Color, 1.0); "
"}";

const char* sh_fragment_fix_color = 
"#version 150                  \n"
"uniform vec4 ObjColor;        "
"out vec4 FragColor;           "
"void main() {                 "
"    FragColor = ObjColor; "
"}";

const char* sh_fragment_tex = 
"#version 150                  \n"
"uniform sampler2D Sampler;    "
"in vec2 v_texcoord;           "
"out vec4 FragColor;           "
"void main() {                 "
"    vec4 texColor = texture( Sampler, v_texcoord );"
"    FragColor = texColor;"
//"    FragColor = vec4(Color, 1.0); "
"}";

const char* sh_fragment_tex_alpha = 
"#version 150                  \n"
"uniform sampler2D Sampler;    "
"uniform vec4      Albedo;    "
"in vec2 v_texcoord;           "
"out vec4 FragColor;           "
"void main() {                 "
"    vec4 texColor = vec4(Albedo.rgb, Albedo.a * texture( Sampler, v_texcoord ).r);"
"    FragColor = texColor;"
//"    FragColor = vec4(Color, 1.0); "
"}";

const char* sh_geometry = "";

glh::ProgramHandle* sp_vcolor_handle;
glh::ProgramHandle* sp_tex_handle;
glh::ProgramHandle* sp_colorcoded;

// App state

glh::RenderPassSettings g_renderpass_settings(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT,
                                            glh::vec4(0.5f,0.5f,0.5f,1.0f), 1);
glh::RenderPassSettings g_blend_settings(glh::RenderPassSettings::BlendSettings(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

bool g_run = true;

std::shared_ptr<glh::DefaultMesh> mesh(new glh::DefaultMesh()); // ram vertex data

// Screen quad shader
const char* sp_vcolors = "screen";
const char* sp_obj     = "screen_obj";
const char* sp_obj_tex = "screen_obj_tex";
const char* font_program_name = "screen_obj_font";
const char* sp_obj_colored = "screen_obj_colored";

float tprev = 0;
float angle = 0.f;
float radial_speed = 0.0f * PIf;

namespace glh{

/** Container for the services initializable after GL context exist. */
class AppServices{
public:

    App*            app_;

    FontManagerPtr  fontmanager_;
    AssetManagerPtr manager_;

    std::shared_ptr<SceneAssets>  assets_;

    SceneTree::Node* scene_ps_;
    SceneTree::Node* scene_ndc_;

    // TODO: RenderQueue

    void init_font_manager()
    {
        GraphicsManager* gm = app_->graphics_manager();

        std::string fontpath = manager_->fontpath();

        if(!directory_exists(fontpath.c_str())){
            throw GraphicsException(std::string("Font directory not found:") + fontpath);
        }

        fontmanager_.reset(new FontManager(gm, fontpath));
    }

    void init(App* app, const char* config_file)
    {
        app_ = app;
        GraphicsManager* gm = app_->graphics_manager();

        manager_ = make_asset_manager(config_file);

        init_font_manager();

        // TODO: Create 2d pixel space camera whose projection matrix is read from app
        // TODO: Create 3d view space camera whose projection matrix is read from app
        // TODO: Use the 2d camera as the camera for the gui renderpass
        // TODO: Make this compile after that

        assets_ = SceneAssets::create(app_, gm, fontmanager_.get());

        auto root = assets_->tree_.root();
        scene_ps_ = assets_->tree_.add_node(root);
        scene_ps_->name_ = "PS"; // Pixel space
        scene_ndc_ = assets_->tree_.add_node(root);
        scene_ndc_->name_ = "NDC"; // Normalized device coordinates

    }

    void update(){
        mat4 screen_to_view = app_orthographic_pixel_projection(app_);
        // move projections to local entities etc
        // update render 
        // scene_ps_->
    }

    FontManager* fontmanager(){ return fontmanager_.get(); }
};

}

glh::AppServices services;

glh::RenderEnvironment env;


glh::FullRenderable font_renderable;


glh::TextLine text_line;

glh::SceneTree   scene;
glh::RenderQueue render_queueue;


std::shared_ptr<glh::GlyphPane> glyph_pane;

int glyph_pane_update_interval;
int glyph_pane_update_interval_limit;

void load_font_resources(glh::GraphicsManager* gm)
{
    using namespace glh;

    std::string old_goudy("OFLGoudyStMTT.ttf");
    std::string junction("Junction-webfont.ttf");

    float fontsize = 15.0f;

    glyph_pane = std::make_shared<GlyphPane>(gm, font_program_name, services.fontmanager());

    glyph_pane->set_font(junction, fontsize);

    glyph_pane->origin_ = vec2(100.f, 100.f);
    glyph_pane->text_field_.push_line("Hello, world.");
    glyph_pane->text_field_.push_line("This is another line, then.");
    glyph_pane->update_representation();
	glyph_pane->dirty_ = true;
    SceneTree::Node* root = scene.root();
    glyph_pane->attach(&scene, root);
}

    //
    // Proto:
    // Implement a directory browser
    //
    // Do two text fields. Select with mouse which one to feed text to.
    // Do a 'field is active' visualization
    // In effect, wrap glyph pane/text consumer into a class
    // Create class to map keyboard events to a nice stream that above can consume.
    // Implement backspace/del.

void init_uniform_data(){
    env.set_vec4("ObjColor", glh::vec4(0.f, 1.f, 0.f, 1.f));
    env.set_vec4("Albedo", glh::vec4(0.28f, 0.024f, 0.024f, 1.0));
    env.set_texture2d("Sampler", glyph_pane->fonttexture_);// todo: then env.set "Sampler" texture into glyph_pane
}

// TODO: Render background.
// TODO: Render cursor.
// TODO: Move cursor with arrow keys
// TODO: Move cursor by clicking with mouse
// TODO: Color picker
// TODO: Repeat
// TODO: us_ascii mapper to text utility

bool init(glh::App* app)
{
    const char* config_file = "config.mp";

    services.init(app, config_file);

    glh::GraphicsManager* gm = app->graphics_manager();

    //fonttexture = gm->create_texture();

    gm->create_program(font_program_name, sh_geometry, sh_vertex_obj_tex, sh_fragment_tex_alpha);

   // Add screenquad to service
   // mesh_load_screenquad(*mesh);
    int width = app->config().width;
    int height = app->config().height;
    mesh_load_screenquad((float) width, (float) height, *mesh);

    //mesh_load_screenquad(0.5f, 0.5f, *mesh);

    load_font_resources(gm);

    init_uniform_data();

    // render backgroudn

    return true;
}


bool update(glh::App* app)
{
    using namespace glh;

    float t = (float) app->time();
    angle += (t - tprev) * radial_speed;
    tprev = t;
    Eigen::Affine3f transform;
    transform = Eigen::AngleAxis<float>(angle, glh::vec3(0.f, 0.f, 1.f));

    mat4 screen_to_view = app_orthographic_pixel_projection(app); 
    // TODO: Use this automatically as the root projection in the 2D scenegraph
    // TODO: Similarly updating projection for the 3D scenegraph

    env.set_mat4(OBJ2WORLD, screen_to_view);

    float albedo_alpha = 1.0f - (t - floor(t));
    env.set_vec4("Albedo", glh::vec4(0.28f, 0.024f, 0.024f,albedo_alpha));

    glyph_pane_update_interval = (glyph_pane_update_interval + 1) % glyph_pane_update_interval_limit;
    if(glyph_pane->dirty_ && glyph_pane_update_interval== 0){
        glyph_pane->update_representation();
    }// TODO add this to render handler for the glyph pane: Text buffer gets updates,
    // these updates are forwarded to glyph pane at specific intervals

    glyph_pane->node_->material_.set_vec4("Albedo", glh::vec4(1.f, 1.f, 1.f,1.f));

    render_queueue.clear();
    render_queueue.add(scene, [](SceneTree::Node* n){return true;});

    return g_run;
}

void render_font(glh::App* app)
{
    glh::GraphicsManager* gm = app->graphics_manager();
    apply(g_renderpass_settings);
    apply(g_blend_settings);
    //font_renderable.reset_buffers(); 'light torture'

    //gm->render(*glyph_pane->renderable_, env, env);
    render_queueue.render(gm, env);
}

void resize(glh::App* app, int width, int height)
{
    std::cout << "Resize:" << width << " " << height << std::endl;
}

void print_mouse_position()
{
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
    else if(key == Input::Left){ radial_speed -= 0.1f * PIf;}
    else if(key == Input::Right){ radial_speed += 0.1f * PIf;}
    else if(s == Input::Held){
        glyph_pane->recieve_characters(key, modifiers);
    }
}

int main(int arch, char* argv)
{
    glyph_pane_update_interval = 0;
    glyph_pane_update_interval_limit = 3;

    glh::AppConfig config = glh::app_config_default(init, update, render_font, resize);

    config.width = 1024;
    config.height = 640;



    glh::App app(config);



    GLH_LOG_EXPR("Logger started");
    add_key_callback(app, key_callback);
    add_mouse_move_callback(app, mouse_move_callback);
    glh::default_main(app);

    // TODO resource manager
    glyph_pane.reset();

    return 0;
}
