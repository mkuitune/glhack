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
#include "glh_dynamic_graph.h"

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

glh::DynamicSystem      dynamics;
glh::FocusContext       focus_context;
glh::StringNumerator    string_numerator;

#define COLOR_DELTA "ColorDelta"
#define PRIMARY_COLOR "PrimaryColor"
#define SECONDARY_COLOR "SecondaryColor"

glh::DynamicGraph graph;

void add_color_interpolation_to_graph(glh::App* app, glh::SceneTree::Node* node){
    using namespace glh;
    struct noderef_t{DynamicGraph::dynamic_node_ptr_t node; std::string name;};
    auto addnode = [&](noderef_t& n){graph.add_node(n.name, n.node);};

    noderef_t sys    = {DynamicGraph::dynamic_node_ptr_t(new SystemInput(app)), "sys"};
    addnode(sys);
    noderef_t ramp   = {DynamicGraph::dynamic_node_ptr_t(new ScalarRamp()),     string_numerator("ramp")};
    addnode(ramp);

    noderef_t dynvalue   = {DynamicGraph::dynamic_node_ptr_t(new LimitedIncrementalValue(0.0, 0.0, 1.0)), string_numerator("dynvalue")};
    addnode(dynvalue);

    noderef_t offset = {DynamicGraph::dynamic_node_ptr_t(new ScalarOffset()),   string_numerator("offset")};
    addnode(offset);
    noderef_t mix    = {DynamicGraph::dynamic_node_ptr_t(new MixNode()),        string_numerator("mix")};
    addnode(mix);

    std::list<std::string> vars = list(std::string(COLOR_DELTA), 
                                       std::string(PRIMARY_COLOR),
                                       std::string(SECONDARY_COLOR));

    noderef_t nodesource = {DynamicGraph::dynamic_node_ptr_t(new NodeSource(node, vars)),   string_numerator("nodesource")};
    addnode(nodesource);
    noderef_t nodereciever = {DynamicGraph::dynamic_node_ptr_t(new NodeReciever(node)),   string_numerator("nodereciever")};
    addnode(nodereciever);

    graph.add_link(sys.name, GLH_PROPERTY_TIME_DELTA, offset.name, GLH_PROPERTY_INTERPOLANT);
    graph.add_link(nodesource.name, COLOR_DELTA, offset.name, GLH_PROPERTY_SCALE);

    graph.add_link(offset.name, GLH_PROPERTY_INTERPOLANT, dynvalue.name, GLH_PROPERTY_DELTA);
    graph.add_link(dynvalue.name, GLH_PROPERTY_INTERPOLANT,  ramp.name, GLH_PROPERTY_INTERPOLANT);

    graph.add_link(ramp.name, GLH_PROPERTY_INTERPOLANT, mix.name, GLH_PROPERTY_INTERPOLANT);
    graph.add_link(nodesource.name, PRIMARY_COLOR, mix.name, GLH_PROPERTY_1);
    graph.add_link(nodesource.name, SECONDARY_COLOR, mix.name, GLH_PROPERTY_2);

    graph.add_link(mix.name, GLH_PROPERTY_COLOR, nodereciever.name, FIXED_COLOR);
}

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

void init_graph(){
}

void init_single_node_callback_chain(){
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

    struct{vec2 dims; vec3 pos; vec4 color_primary; vec4 color_secondary;} quads[] ={
        {vec2(0.5, 0.5), vec3(-0.5, -0.5, 0.), vec4(COLOR_RED), vec4(COLOR_WHITE) },
        {vec2(0.5, 0.5), vec3(0.5, 0.5, 0.), vec4(COLOR_GREEN), vec4(COLOR_WHITE)},
        {vec2(0.5, 0.5), vec3(-0.5, 0.5, 0.), vec4(COLOR_BLUE), vec4(COLOR_WHITE)},
        {vec2(0.25, 0.25), vec3(0.5, -0.5, 0.), vec4(COLOR_YELLOW),vec4(COLOR_WHITE) }
    };

    size_t quad_count = static_array_size(quads);
    for(auto& s: quads){
        string name = string_numerator("node");
        auto n = add_quad_to_scene(gm, *sp_colored_program, s.dims);
        n->location_ = s.pos;
        n->name_ = name;
        set_material(*n, FIXED_COLOR,    s.color_primary);
        set_material(*n, PRIMARY_COLOR,  s.color_primary);
        set_material(*n, SECONDARY_COLOR,s.color_secondary);
        set_material(*n, COLOR_DELTA, 0.f);
        add(nodes, n);
        add(ui_entities, UiEntity(n));
        add_color_interpolation_to_graph(app, n);
    }
    graph.solve_dependencies();

    for(auto& e:ui_entities){
        render_picker->add(&e);
    }

    init_uniform_data();

    return true;
}

std::set<glh::SceneTree::Node*> focused;

void mouse_move_node(glh::App* app, glh::SceneTree::Node* node, vec2i delta)
{
    glh::mat4 screen_to_view = app_orthographic_pixel_projection(app);

    // should be replaced with view specific change vector...
    // projection of the view change vector onto the workplane...
    // etc.

    glh::vec4 v(delta[0], delta[1], 0, 0);
    glh::vec3 nd = glh::decrease_dim<float, 4>(screen_to_view * v);

    node->location_ = node->location_ + nd;
}

// TODO: Figure out a more elegant formulation for this.
// State machines are state machines...
struct Mouse{

    struct Event{
        enum t{LeftButtonActivated, LeftButtonDeactivated, MouseMoved};
    };

    std::stack<Event::t> events_;
    std::vector<glh::SceneTree::Node*> grabbed;

    bool left_button_down_;


    Mouse():left_button_down_(false){
        current_ = glh::vec2i(0,0);
        prev_ = glh::vec2i(0,0);
    }

    void left_button_is_down(){
        if(!left_button_down_){
            left_button_down_ = true;
            events_.push(Event::LeftButtonActivated);
        }
    }

    void left_button_up(){
        left_button_down_ = false;
        events_.push(Event::LeftButtonDeactivated);
    }

    void move(int x, int y){
        prev_ = current_;
        current_[0] = x;
        current_[1] = y;
        events_.push(Event::MouseMoved);
    }

    // Must call only after selection context has been filled
    void update(){
        // Handle events
        while(!events_.empty()){
            Event::t e = events_.top();
            events_.pop();
            if(e == Event::LeftButtonActivated){
                for(glh::SceneTree::Node* node: focused){
                    grabbed.push_back(node);
                }
            }
            else if(e == Event::LeftButtonDeactivated){
                grabbed.clear();
            }
            else if(e == Event::MouseMoved){
                vec2i delta = current_ - prev_;
                for(glh::SceneTree::Node* node: grabbed){
                    mouse_move_node(app_, node, delta);
                }
            }
        }
    }

    glh::App* app_;

    glh::vec2i current_;
    glh::vec2i prev_;
}; 

Mouse mouse;


bool update(glh::App* app)
{
    using namespace glh;

    float t = (float) app->time();

    dynamics.update(t);

    graph.execute();

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
        auto picked = render_picker->render_selectables(env, g_mouse_x, g_mouse_y);

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
    node->material_.scalar_[COLOR_DELTA] = 5.0f;
    focused.insert(node);
}

void node_focus_lost(glh::App* app, glh::SceneTree::Node* node){
    std::cout << "Focus lost:" << node->name_<< std::endl;
    node->material_.scalar_[COLOR_DELTA] = -1.0f;
    focused.erase(node);
}

void render(glh::App* app){
    do_selection_pass(app);
    do_render_pass(app);

    // Kinda not rendering anymore, now were doing UI handling.
    // TODO: Figure this out. All this is because picking is tied to rendering.
    // Maybe need third pass. Or then do update after render pass?
    if(focus_context.event_handling_done_){
        for(auto& g:focus_context.focus_gained_){
            node_focus_gained(app, g->node_);
        }

        for(auto& l:focus_context.focus_lost_){
            node_focus_lost(app, l->node_);
        }
    }

    mouse.update();
}

void resize(glh::App* app, int width, int height){
    std::cout << "Resize:" << width << " " << height << std::endl;
}


void mouse_move_callback(int x, int y)
{
    g_mouse_x = x;
    g_mouse_y = y;
    //std::cout << "Mouse:"  << x << " " << y << std::endl;
    g_read_color_at_mouse = true;
    mouse.move(x,y);
}

void mouse_button_callback(int key, const glh::Input::ButtonState& s)
{
    using namespace glh;

    if(s == glh::Input::Held){
        std::cout << "Mouse down." << std::endl;
        mouse.left_button_is_down();
    }
    else if(s == glh::Input::Released){
        mouse.left_button_up();
    }

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

    mouse.app_ = &app;
    render_picker = std::make_shared<glh::RenderPicker>(app);

    GLH_LOG_EXPR("Logger started");
    add_key_callback(app, key_callback);
    add_mouse_button_callback(app, mouse_button_callback);
    add_mouse_move_callback(app, mouse_move_callback);
    glh::default_main(app);

    return 0;
}
