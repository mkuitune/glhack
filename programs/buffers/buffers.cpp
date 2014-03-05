/** 
    Test buffer management and selection.
*/

#include "glh_app_services.h"

//////////////// Program stuff ////////////////

// TAKE CAMERA INTO USE
// FILTER BASED ON SCENEGRAPH ROOT OR CREATE A SPECIFIC COLLECTION FOR PICKABLE AND INTERACTION LOCKED
//  so NOT EXPLICIT PARAMETERS rather MOVE TO AND BETWEEN specific collecionts


glh::RenderPassSettings g_renderpass_settings(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT,
                                            glh::vec4(0.0f,0.0f,0.0f,1.f), 1);

bool g_run = true;

glh::RenderEnvironment env;                           // TODO get rid of


glh::RenderPickerPtr render_picker;                   // TODO to assets etc

glh::RenderQueue                   selectable_queue;  // TODO Do not use RenderQueues directly, use render passes
glh::RenderQueue                   render_queueue; // TODO Do not use RenderQueues directly, use render passes
glh::RenderQueue                   top_queue; // TODO Do not use RenderQueues directly, use render passes

glh::RenderPass*  selection_pass;
glh::RenderPass*  render_pass;
glh::RenderPass*  top_pass;

glh::AppServices  services;



// TODO to 'techniques' file?
void add_color_interpolation_to_graph(glh::App* app, glh::DynamicGraph& graph, glh::SceneTree::Node* node, std::string target_var_Name){
    using namespace glh;
    auto f = DynamicNodeRef::factory_enumerate_names(graph, app->string_numerator());

    auto sys      = DynamicNodeRef::factory(graph)(new SystemInput(app));
    auto ramp     = f(new ScalarRamp());
    auto dynvalue = f(new LimitedIncrementalValue(0.0, 0.0, 1.0));
    auto offset   = f(new ScalarOffset());
    auto mix      = f(new MixNode());

    std::list<std::string> vars = list(std::string(GLH_COLOR_DELTA), 
                                       std::string(GLH_PRIMARY_COLOR),
                                       std::string(GLH_SECONDARY_COLOR));

    auto nodesource = f(new NodeSource(node, vars));
    auto nodereciever = f(new NodeReciever(node));

    auto lnk = DynamicNodeRef::linker(graph);

    lnk(sys, GLH_PROPERTY_TIME_DELTA, offset, GLH_PROPERTY_INTERPOLANT);
    lnk(nodesource, GLH_COLOR_DELTA, offset, GLH_PROPERTY_SCALE);

    lnk(offset, GLH_PROPERTY_INTERPOLANT, dynvalue, GLH_PROPERTY_DELTA);
    lnk(dynvalue, GLH_PROPERTY_INTERPOLANT,  ramp, GLH_PROPERTY_INTERPOLANT);

    lnk(ramp, GLH_PROPERTY_INTERPOLANT, mix, GLH_PROPERTY_INTERPOLANT);
    lnk(nodesource, GLH_PRIMARY_COLOR, mix, GLH_PROPERTY_1);
    lnk(nodesource, GLH_SECONDARY_COLOR, mix, GLH_PROPERTY_2);

    lnk(mix, GLH_PROPERTY_COLOR, nodereciever, target_var_Name);
}

// TODO to 'techniques' file?
void add_focus_action(glh::App* app, glh::SceneTree::Node* node, glh::FocusContext& focus_context, glh::DynamicGraph& graph){
    using namespace glh;
    auto f = DynamicNodeRef::factory_enumerate_names(graph, app->string_numerator());

    float decspeed = -2.f;
    float incrspeed = 7.f;

    auto focus    = f(new NodeFocusState(node, focus_context));
    auto mix     = f(new MixScalar(decspeed, incrspeed));
    auto noderes  = f(new NodeReciever(node));

    auto lnk = DynamicNodeRef::linker(graph);

    lnk(focus, GLH_PROPERTY_INTERPOLANT, mix, GLH_PROPERTY_INTERPOLANT);
    lnk(mix, GLH_PROPERTY_INTERPOLANT, noderes, GLH_COLOR_DELTA);
}

using glh::vec2i;
using glh::vec2;


bool init(glh::App* app)
{
    using namespace glh;
    using std::string;

    GraphicsManager* gm = app->graphics_manager();

    const char* config_file = "config.mp";
    services.init(app, config_file);

    auto sp_colored_program = gm->program(GLH_CONSTANT_ALBEDO_PROGRAM);
    render_picker->selection_program_ = gm->program(GLH_COLOR_PICKER_PROGRAM);

    struct{vec2 dims; vec3 pos; vec4 color_primary; vec4 color_secondary;} quads[] ={
        {vec2(0.5, 0.5), vec3(-0.5, -0.5, 0.), vec4(COLOR_RED), vec4(COLOR_WHITE) },
        {vec2(0.5, 0.5), vec3(0.5, 0.5, 0.), vec4(COLOR_GREEN), vec4(COLOR_WHITE)},
        {vec2(0.5, 0.5), vec3(-0.5, 0.5, 0.), vec4(COLOR_BLUE), vec4(COLOR_WHITE)},
        {vec2(0.25, 0.25), vec3(0.5, -0.5, 0.), vec4(COLOR_YELLOW),vec4(COLOR_WHITE) }
    };

    size_t quad_count = static_array_size(quads);
    for(auto& s: quads){
        string name = app->string_numerator()("node");

        auto parent = services.assets().scene().add_node(services.assets().scene().root());

        auto n = add_quad_to_scene(gm, services.assets().scene(), *sp_colored_program, s.dims, parent);
        parent->transform_.position_ = s.pos;
        n->name_ = name;
        set_material(*n, GLH_COLOR_ALBEDO, s.color_primary);
        set_material(*n, GLH_PRIMARY_COLOR,  s.color_primary);
        set_material(*n, GLH_SECONDARY_COLOR,s.color_secondary);
        set_material(*n, GLH_COLOR_DELTA, 0.f);
        add_color_interpolation_to_graph(app, services.graph(), n, GLH_COLOR_ALBEDO);
        add_focus_action(app, n, services.ui_context().focus_context_, services.graph());
    }
    services.graph().solve_dependencies();

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

    services.graph().execute();

    services.assets().scene().update();
    services.assets().scene().apply_to_render_env();
    
    render_queueue.clear();
    render_queueue.add(services.assets().scene(), pass_interaction_unlocked);

    selectable_queue.clear();
    selectable_queue.add(services.assets().scene(), pass_pickable_and_unlocked);
    render_picker->attach_render_queue(&selectable_queue);

    top_queue.clear();
    top_queue.add(services.assets().scene(), pass_interaction_locked);
    
    return g_run;
}

void do_selection_pass(glh::App* app){
    using namespace glh;

    glh::FocusContext::Focus focus = services.ui_context().focus_context_.start_event_handling();
    vec2i mouse = services.ui_context().mouse_current_;

    auto picked = render_picker->render_selectables(env, mouse[0], mouse[1]); // Render picker to ui_context?
                                                                              // Lift 'do selection pass' to some technique file?
    
    for(auto p: picked){
        focus.on_focus(p);
    }
    focus.update_event_state();

}

void do_render_pass(glh::App* app){
    using namespace glh;

    GraphicsManager* gm = app->graphics_manager();

    apply(g_renderpass_settings);
    render_queueue.render(gm, *gm->program(GLH_CONSTANT_ALBEDO_PROGRAM), env);
    top_queue.render(gm, *gm->program(GLH_CONSTANT_ALBEDO_PROGRAM), env);
}

std::map<glh::SceneTree::Node*, glh::vec4, std::less<glh::SceneTree::Node*>,
Eigen::aligned_allocator<std::pair<glh::UiEntity*, glh::vec4>>> prev_color;

void render(glh::App* app){

    services.ui_context().update();

    do_selection_pass(app);
    do_render_pass(app);

}

void resize(glh::App* app, int width, int height){
    std::cout << "Resize:" << width << " " << height << std::endl;
}

void key_callback(int key, const glh::Input::ButtonState& s)
{
    using namespace glh;
    if(key == Input::Esc) { g_run = false;}
}

int main(int arch, char* argv)
{
    glh::AppConfig config = glh::app_config_default(init, update, render, resize);
    config.width = 1024;
    config.height = 640;

    glh::App app(config);

    render_picker = std::make_shared<glh::RenderPicker>(app);

    GLH_LOG_EXPR("Logger started");
    add_key_callback(app, key_callback);

    // Cannot initialize gl resources here. Must go into init().

    glh::default_main(app);

    return 0;
}
