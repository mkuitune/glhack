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
#include "glh_uicontext.h"


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
"uniform mat4 LocalToWorld;" // TODO add world to screen
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

glh::ObjectRoster::IdGenerator idgen;

int obj_id = glh::ObjectRoster::max_id / 2;

glh::RenderPickerPtr render_picker;

glh::SceneTree                     scene;
glh::RenderQueue                   selectable_queue;
glh::RenderQueue                   render_queueue;
glh::RenderQueue                   top_queue;
std::vector<glh::SceneTree::Node*> nodes;

glh::DynamicSystem      dynamics;
glh::StringNumerator    string_numerator;

std::unique_ptr<glh::UiContext> ui_context;

#define COLOR_DELTA "ColorDelta"
#define PRIMARY_COLOR "PrimaryColor"
#define SECONDARY_COLOR "SecondaryColor"

glh::DynamicGraph graph;

void add_color_interpolation_to_graph(glh::App* app, glh::SceneTree::Node* node){
    using namespace glh;
    auto f = DynamicNodeRef::factory(graph);

    auto sys      = f(new SystemInput(app), "sys");
    auto ramp     = f(new ScalarRamp(), string_numerator("ramp"));
    auto dynvalue = f(new LimitedIncrementalValue(0.0, 0.0, 1.0), string_numerator("dynvalue"));
    auto offset   = f(new ScalarOffset(), string_numerator("offset"));
    auto mix      = f(new MixNode(), string_numerator("mix"));

    std::list<std::string> vars = list(std::string(COLOR_DELTA), 
                                       std::string(PRIMARY_COLOR),
                                       std::string(SECONDARY_COLOR));

    auto nodesource = f(new NodeSource(node, vars),string_numerator("nodesource"));
    auto nodereciever = f(new NodeReciever(node),string_numerator("nodereciever"));

    auto lnk = DynamicNodeRef::linker(graph);

    lnk(sys, GLH_PROPERTY_TIME_DELTA, offset, GLH_PROPERTY_INTERPOLANT);
    lnk(nodesource, COLOR_DELTA, offset, GLH_PROPERTY_SCALE);

    lnk(offset, GLH_PROPERTY_INTERPOLANT, dynvalue, GLH_PROPERTY_DELTA);
    lnk(dynvalue, GLH_PROPERTY_INTERPOLANT,  ramp, GLH_PROPERTY_INTERPOLANT);

    lnk(ramp, GLH_PROPERTY_INTERPOLANT, mix, GLH_PROPERTY_INTERPOLANT);
    lnk(nodesource, PRIMARY_COLOR, mix, GLH_PROPERTY_1);
    lnk(nodesource, SECONDARY_COLOR, mix, GLH_PROPERTY_2);

    lnk(mix, GLH_PROPERTY_COLOR, nodereciever, FIXED_COLOR);
}

void add_focus_action(glh::App* app, glh::SceneTree::Node* node, glh::FocusContext& focus_context, glh::DynamicGraph& graph, glh::StringNumerator& string_numerator){
    using namespace glh;
    auto f = DynamicNodeRef::factory(graph);

    float decspeed = -2.f;
    float incrspeed = 7.f;

    auto focus    = f(new NodeFocusState(node, focus_context), string_numerator("node_focus"));
    auto mix     = f(new MixScalar(decspeed, incrspeed), string_numerator("mix"));
    auto noderes  = f(new NodeReciever(node), string_numerator("node_reciever"));

    auto lnk = DynamicNodeRef::linker(graph);

    lnk(focus, GLH_PROPERTY_INTERPOLANT, mix, GLH_PROPERTY_INTERPOLANT);
    lnk(mix, GLH_PROPERTY_INTERPOLANT, noderes, COLOR_DELTA);
}

void init_uniform_data(){
    //env.set_vec4("ObjColor", glh::vec4(0.f, 1.f, 0.f, 0.2f));
    glh::vec4 obj_color_sel = glh::ObjectRoster::color_of_id(obj_id);
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

    ui_context.reset(new glh::UiContext(*gm, *app, graph, string_numerator));

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
        n->transform_.position_ = s.pos;
        n->name_ = name;
        set_material(*n, FIXED_COLOR,    s.color_primary);
        set_material(*n, PRIMARY_COLOR,  s.color_primary);
        set_material(*n, SECONDARY_COLOR,s.color_secondary);
        set_material(*n, COLOR_DELTA, 0.f);
        add(nodes, n);
        add_color_interpolation_to_graph(app, n);
        add_focus_action(app, n, ui_context->focus_context_, graph, string_numerator);
    }
    graph.solve_dependencies();

    init_uniform_data();

    return true;
}


bool pass_pickable(glh::SceneTree::Node* node){return node->pickable_;}
bool pass_unpickable(glh::SceneTree::Node* node){return !node->pickable_;}
bool pass_interaction_unlocked(glh::SceneTree::Node* node){return !node->interaction_lock_;}
bool pass_interaction_locked(glh::SceneTree::Node* node){return node->interaction_lock_;}

bool pass_pickable_and_unlocked(glh::SceneTree::Node* node){return pass_pickable(node) && pass_interaction_unlocked(node);}

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
    render_queueue.add(scene, pass_interaction_unlocked);

    selectable_queue.clear();
    selectable_queue.add(scene, pass_pickable_and_unlocked);
    render_picker->attach_render_queue(&selectable_queue);

    top_queue.clear();
    top_queue.add(scene, pass_interaction_locked);
    
    return g_run;
}

void do_selection_pass(glh::App* app){
    using namespace glh;

    glh::FocusContext::Focus focus = ui_context->focus_context_.start_event_handling();

    vec2i mouse = ui_context->mouse_current_;

    auto picked = render_picker->render_selectables(env, mouse[0], mouse[1]);
    
    for(auto p: picked){
        focus.on_focus(p);
    }
    focus.update_event_state();

}

void do_render_pass(glh::App* app){
    using namespace glh;

    GraphicsManager* gm = app->graphics_manager();

    apply(g_renderpass_settings);
    render_queueue.render(gm, *sp_colored_program, env);
    top_queue.render(gm, *sp_colored_program, env);
}

std::map<glh::SceneTree::Node*, glh::vec4, std::less<glh::SceneTree::Node*>,
Eigen::aligned_allocator<std::pair<glh::UiEntity*, glh::vec4>>> prev_color;

void render(glh::App* app){

    ui_context->update();

    do_selection_pass(app);
    do_render_pass(app);

}

void resize(glh::App* app, int width, int height){
    std::cout << "Resize:" << width << " " << height << std::endl;
}

void mouse_button_callback(int key, const glh::Input::ButtonState& s)
{
    using namespace glh;

    if(s == glh::Input::Held){
        std::cout << "Mouse down." << std::endl;
        ui_context->left_button_is_down();
    }
    else if(s == glh::Input::Released){
        ui_context->left_button_up();
    }
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

    // Cannot initialize gl resources here. Must go into init().

    glh::default_main(app);

    return 0;
}
