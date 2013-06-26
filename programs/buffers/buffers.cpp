/** 
    Test buffer management and selection.
*/

#include "glbase.h"
#include "asset_manager.h"
#include "glh_generators.h"
#include "glh_image.h"
#include "glh_typedefs.h"
#include "glh_font.h"
#include "shims_and_types.h"
#include "math_tools.h"
#include "glh_scenemanagement.h"
#include "glh_timebased_signals.h"

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

const char* sh_vertex_obj_pos   = 
"#version 150               \n"
"uniform mat4 LocalToWorld;"
"in vec3      VertexPosition;    "
"void main()                "
"{                          "
"    gl_Position = LocalToWorld * vec4( VertexPosition, 1.0 );"
"}";

const char* sh_vertex_obj_tex   = 
"#version 150               \n"
"uniform mat4 LocalToWorld;"
"in vec3      VertexPosition;    "
"in vec3      TexCoord;"
"out vec3 v_color;            "
"out vec2 v_texcoord;"
"void main()                "
"{                          "
"    v_texcoord = TexCoord.xy;"
"    gl_Position = LocalToWorld * vec4( VertexPosition, 1.0 );"
"}";

const char* sh_fragment = 
"#version 150                  \n"
"in vec3 Color;                "
"out vec4 FragColor;           "
"void main() {                 "
"    FragColor = vec4(Color, 1.0); "
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


const char* sh_vertex_obj   = 
"#version 150               \n"
"uniform mat4 LocalToWorld;"
"in vec3      VertexPosition;    "
"in vec3      VertexColor;       "
"out vec3 Color;            "
"void main()                "
"{                          "
"    Color = VertexColor;   "
"    gl_Position = LocalToWorld * vec4( VertexPosition, 1.0 );"
"}";

#define FIXED_COLOR "FixedColor"

const char* sh_fragment_fix_color = 
"#version 150                  \n"
"uniform vec4 FixedColor;"
"out vec4 FragColor;           "
"void main() {                 "
"    FragColor = FixedColor;   "
"}";

const char* sh_fragment_selection_color = 
"#version 150                  \n"
"uniform vec4 " UICTX_SELECT_NAME ";"
"out vec4 FragColor;           "
"void main() {                 "
"    FragColor = " UICTX_SELECT_NAME "; "
"}";


const char* sh_geometry = "";
const char* sp_obj     = "sp_colored";
const char* sp_select  = "sp_selecting";

glh::ProgramHandle* sp_colored_program;
glh::ProgramHandle* sp_select_program;

glh::Texture* texture;

// App state

glh::RenderPassSettings g_renderpass_settings(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT,
                                            glh::vec4(0.0f,0.0f,0.0f,1.f), 1);
//glh::RenderPassSettings g_color_mask_settings(glh::RenderPassSettings::ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE));
//glh::RenderPassSettings g_blend_settings(glh::RenderPassSettings::BlendSettings(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
//glh::RenderPassSettings g_blend_settings(glh::RenderPassSettings::BlendSettings(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

bool g_run = true;

glh::AssetManagerPtr manager;
glh::RenderEnvironment env;

std::shared_ptr<glh::FontContext> fontcontext;

int g_mouse_x;
int g_mouse_y;

glh::ColorSelection::IdGenerator idgen;

int obj_id = glh::ColorSelection::max_id / 2;

glh::RenderPickerPtr render_picker;

glh::SceneTree                     scene;
glh::RenderQueue                   render_queueue;
std::vector<glh::SceneTree::Node*> nodes;
std::list<glh::UiEntity>           ui_entities;

glh::DynamicSystem                 dynamics;
glh::FocusContext                  focus_context;

void init_uniform_data(){
    //env.set_vec4("ObjColor", glh::vec4(0.f, 1.f, 0.f, 0.2f));
    glh::vec4 obj_color_sel = glh::ColorSelection::color_of_id(obj_id);
    env.set_vec4(UICTX_SELECT_NAME,     obj_color_sel);
    env.set_vec4(FIXED_COLOR, glh::vec4(0.28f, 0.024f, 0.024f, 1.0));
}

using glh::vec2i;
using glh::vec2;

void load_screenquad(vec2 size, glh::DefaultMesh& mesh)
{
    using namespace glh;

    vec2 halfsize = 0.5f * size;

    vec2 low  = - halfsize;
    vec2 high = halfsize;

    mesh_load_quad_xy(low, high, mesh);
}

glh::SceneTree::Node* add_quad_to_scene(glh::GraphicsManager* gm, glh::ProgramHandle& program, glh::vec2 dims){

    glh::DefaultMesh* mesh = gm->create_mesh();
    load_screenquad(dims, *mesh);

    auto renderable = gm->create_renderable();

    renderable->bind_program(program);
    renderable->set_mesh(mesh);

    auto root = scene.root();
    return scene.add_node(root, renderable);
}

bool init(glh::App* app)
{
    using namespace glh;
    using std::string;

    GraphicsManager* gm = app->graphics_manager();

    //sp_colored_program = gm->create_program(sp_obj, sh_geometry, sh_vertex_obj, sh_fragment);
    sp_select_program  = gm->create_program(sp_select, sh_geometry, sh_vertex_obj, sh_fragment_selection_color);
    sp_colored_program = gm->create_program(sp_obj, sh_geometry, sh_vertex_obj, sh_fragment_fix_color);

    render_picker->selection_program_ = sp_select_program;

    struct{vec2 dims; vec3 pos; vec4 color;} quads[] ={
        {vec2(0.5, 0.5), vec3(-0.5, -0.5, 0.), vec4(COLOR_RED)},
        {vec2(0.5, 0.5), vec3(0.5, 0.5, 0.), vec4(COLOR_GREEN)},
        {vec2(0.5, 0.5), vec3(-0.5, 0.5, 0.), vec4(COLOR_BLUE)},
        {vec2(0.25, 0.25), vec3(0.5, -0.5, 0.), vec4(COLOR_YELLOW)}
    };

    size_t quad_count = static_array_size(quads);
    int ind =  0;
    for(auto& s: quads){
        string name = string("node") + std::to_string(ind++);
        auto n = add_quad_to_scene(gm, *sp_colored_program, s.dims);
        n->location_ = s.pos;
        n->name_ = name;
        set_material(*n, FIXED_COLOR,  s.color);
        add(nodes, n);
        add(ui_entities, UiEntity(n));
    }

    for(auto& e:ui_entities){
        render_picker->add(&e);
    }

    init_uniform_data();

    return true;
}

bool update(glh::App* app)
{
    using namespace glh;

    float t = (float) app->time();

    dynamics.update(t);

    Eigen::Affine3f transform;
    transform = Eigen::AngleAxis<float>(0.f, glh::vec3(0.f, 0.f, 1.f));

    mat4 screen_to_view = app_orthographic_pixel_projection(app);

    env.set_mat4(GLH_LOCAL_TO_WORLD, transform.matrix());

    env.set_vec4("Albedo", glh::vec4(0.28f, 0.024f, 0.024f, 1.0));

    scene.update();
    scene.apply_to_renderables();

    render_queueue.clear();
    render_queueue.add(scene);

    return g_run;
}

bool g_read_color_at_mouse = false;

void do_selection_pass(glh::App* app){
    using namespace glh;

    glh::FocusContext::Focus focus = focus_context.start_event_handling();

    if(g_read_color_at_mouse){
        auto picked = render_picker->render_selectables(env);

        for(auto& p: picked){
            focus.on_focus(p);
        }
        focus.update_event_state();
        g_read_color_at_mouse = false;
    }
}

void do_render_pass(glh::App* app){
    using namespace glh;

    GraphicsManager* gm = app->graphics_manager();

    apply(g_renderpass_settings);
    //render_queueue.render(gm, env);
    render_queueue.render(gm, *sp_colored_program, env);
}

std::map<glh::SceneTree::Node*, glh::vec4, std::less<glh::SceneTree::Node*>,
Eigen::aligned_allocator<std::pair<glh::UiEntity*, glh::vec4>>> prev_color;


void node_focus_gained(glh::App* app, glh::SceneTree::Node* node){
    std::cout << "Focus gained:" << node->name_<< std::endl;
    glh::vec4 true_color = node->material_.vec4_[FIXED_COLOR];;
    prev_color[node] =  true_color;
    float apptime = (float) app->time();
    glh::vec4 target_color(COLOR_WHITE);
    
    glh::vec4* address = &node->material_.vec4_[FIXED_COLOR];

    glh::array4 origcol(true_color);
    glh::array4 targcol(target_color);

    auto updater = [=](float t){
        glh::vec4 tru = origcol.to_vec();
        glh::vec4 targ = targcol.to_vec();

        bool is_done = false;
        float dur = 0.2f;
        float delta = (t - apptime) / dur;
        if(delta < 1.0f){*address = glh::lerp(delta, tru, targ);}
        else is_done = true;
        return is_done;
    };

    dynamics.add(node, updater); 
}

void node_focus_lost(glh::App* app, glh::SceneTree::Node* node){
    std::cout << "Focus lost:" << node->name_<< std::endl;

    node->material_.vec4_[FIXED_COLOR] = prev_color[node];
    node->scale_ = glh::vec3(1.f,1.f,1.f);
    prev_color.erase(node);
    dynamics.remove(node); 
}

void render(glh::App* app)
{
    do_selection_pass(app);
    do_render_pass(app);

    if(focus_context.event_handling_done_){
        for(auto& g:focus_context.focus_gained_){
            node_focus_gained(app, g->node_);
        }

        for(auto& l:focus_context.focus_lost_){
            node_focus_lost(app, l->node_);
        }
    }
}

#if 0
void render(glh::App* app)
{
    do_selection_pass(app);
    do_render_pass(app);

    if(focus_context.event_handling_done_){
        for(auto& g:focus_context.focus_gained_){
            std::cout << "Focus gained:" << g->node_->name_<< std::endl;
            glh::vec4 true_color = g->node_->renderable_->material_.vec4_[FIXED_COLOR];;
            prev_color[ g->node_] =  true_color;
            float apptime = (float) app->time();
            glh::vec4 target_color(COLOR_WHITE);
            
            glh::vec4* address = &g->node_->renderable_->material_.vec4_[FIXED_COLOR];

            glh::array4 origcol(true_color);
            glh::array4 targcol(target_color);

            auto updater = [=](float t){
                glh::vec4 tru = origcol.to_vec();
                glh::vec4 targ = targcol.to_vec();

                bool is_done = false;
                float dur = 1.0f;
                float delta = (t - apptime) / dur;
                if(delta < 1.0f){*address = glh::lerp(delta, tru, targ);}
                else is_done = true;
                return is_done;
            };

            dynamics.add(g->node_, updater); 
        }

        for(auto& l:focus_context.focus_lost_){
            std::cout << "Focus lost:" << l->node_->name_<< std::endl;

           l->node_->renderable_->material_.vec4_[FIXED_COLOR] = prev_color[l->node_];
           prev_color.erase(l->node_);
           dynamics.remove(l->node_); 

        }
    }
}
#endif

void resize(glh::App* app, int width, int height)
{
    std::cout << "Resize:" << width << " " << height << std::endl;
}

void print_mouse_position()
{
}

void read_color_at_mouse(){
    //std::cout << "Mouse:"  << x << " " << y << std::endl;
    
}

void mouse_move_callback(int x, int y)
{
    g_mouse_x = x;
    g_mouse_y = y;
    //std::cout << "Mouse:"  << x << " " << y << std::endl;
    render_picker->pointer_move_cb(x,y);
    g_read_color_at_mouse = true;
}

void mouse_button_callback(int key, const glh::Input::ButtonState& s)
{
    using namespace glh;

    if(s == glh::Input::Held) std::cout << "Mouse down." << std::endl;

    //if(key == Input::LeftButton && (s == glh::Input::Held)) g_read_color_at_mouse = true;
}

void key_callback(int key, const glh::Input::ButtonState& s)
{
    using namespace glh;
    if(key == Input::Esc) { g_run = false;}
}

int main(int arch, char* argv)
{
    glh::AppConfig config = glh::app_config_default(init, update, render, resize);
    //glh::AppConfig config = glh::app_config_default(init, update, render_with_selection_pass, resize);
    config.width = 1024;
    config.height = 640;

    const char* config_file = "config.mp";

    manager = glh::make_asset_manager(config_file);

    glh::App app(config);

    render_picker = std::make_shared<glh::RenderPicker>(app);

    GLH_LOG_EXPR("Logger started");
    add_key_callback(app, key_callback);
    add_mouse_button_callback(app, mouse_button_callback);
    add_mouse_move_callback(app, mouse_move_callback);
    glh::default_main(app);

    return 0;
}
