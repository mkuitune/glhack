/** 
    Test buffer management and selection.
*/

#include "glh_app_services.h"

glh::RenderPassSettings g_renderpass_settings(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT,
                                            glh::vec4(0.0f,0.0f,0.0f,1.f), 1);

bool g_run = true;

glh::Camera*      camera;

glh::RenderPass*  render_pass;
glh::RenderPass*  top_pass;
glh::RenderPass*  pick3dpass;

glh::AppServices  services;


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

    struct{vec2 dims; vec3 pos; vec4 color_primary; vec4 color_secondary;} quads[] ={
        {vec2(0.5, 0.5), vec3(-0.5, -0.5, 0.), vec4(COLOR_RED), vec4(COLOR_WHITE) },
        {vec2(0.5, 0.5), vec3(0.5, 0.5, 0.), vec4(COLOR_GREEN), vec4(COLOR_WHITE)},
        {vec2(0.5, 0.5), vec3(-0.5, 0.5, 0.), vec4(COLOR_BLUE), vec4(COLOR_WHITE)},
        {vec2(0.25, 0.25), vec3(0.5, -0.5, 0.), vec4(COLOR_YELLOW),vec4(COLOR_WHITE) }
    };

    size_t quad_count = static_array_size(quads);
    for(auto& s : quads){
        string name = app->string_numerator()("node");

        auto parent = services.assets().scene().add_node(services.assets().scene().root());

        auto n = add_quad_to_scene(gm, services.assets().scene(), *sp_colored_program, s.dims, parent);
        parent->transform_.position_ = s.pos;
        n->name_ = name;

        n->material_[GLH_COLOR_ALBEDO] = s.color_primary;
        n->material_[GLH_PRIMARY_COLOR] = s.color_primary;
        n->material_[GLH_SECONDARY_COLOR] = s.color_secondary;
        n->material_[GLH_COLOR_DELTA] = 0.f;

        add_color_interpolation_to_graph(app, services.graph(), n, GLH_COLOR_ALBEDO);
        add_focus_action(app, n, services.selection_world_3d_->focus_context_, services.graph());
    }

    services.graph().solve_dependencies();

    camera = services.assets().create_camera(glh::Camera::Projection::Orthographic);

    render_pass = services.assets().create_render_pass("RenderPass");
    top_pass = services.assets().create_render_pass("TopPass");

    pick3dpass = services.selection_pass_3d_default_;
    pick3dpass->set_camera(camera);

    render_pass->set_camera(camera);
    top_pass->set_camera(camera);

    return true;
}

bool update(glh::App* app)
{
    using namespace glh;

    float t = (float) app->time();

    services.update();

    render_pass->set_queue_filter(glh::pass_interaction_unlocked);
    render_pass->update_queue_filtered(services.assets().scene());

    top_pass->set_queue_filter(pass_interaction_locked);
    top_pass->update_queue_filtered(services.assets().scene());

    pick3dpass->update_queue_filtered(services.assets().scene()); // Set selection pass camera and context

    return g_run;
}

void do_render_pass(glh::App* app){
    using namespace glh;

    GraphicsManager* gm = app->graphics_manager();

    apply(g_renderpass_settings);
    render_pass->render(gm);
    top_pass->render(gm);
}

void render(glh::App* app){

    services.ui_context().update();

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

    GLH_LOG_EXPR("Logger started");
    add_key_callback(app, key_callback);

    // Cannot initialize gl resources here. Must go into init().

    glh::default_main(app);

    return 0;
}
