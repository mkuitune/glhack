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

glh::UIContextPtr uicontext;

glh::SceneTree   scene;
glh::RenderQueue render_queueue;

void mouse_move_callback(int x, int y)
{
    g_mouse_x = x;
    g_mouse_y = y;
    //std::cout << "Mouse:"  << x << " " << y << std::endl;
}

void init_uniform_data(){
    //env.set_vec4("ObjColor", glh::vec4(0.f, 1.f, 0.f, 0.2f));
    glh::vec4 obj_color_sel = glh::ColorSelection::color_of_id(obj_id);
    env.set_vec4(UICTX_SELECT_NAME,     obj_color_sel);
    env.set_vec4(FIXED_COLOR, glh::vec4(0.28f, 0.024f, 0.024f, 1.0));
}

using glh::vec2i;
using glh::vec2;

void load_screenquad(vec2 pos, vec2 size, glh::DefaultMesh& mesh)
{
    using namespace glh;

    vec2 halfsize = 0.5f * size;

    vec2 low  = pos - halfsize;
    vec2 high = pos + halfsize;

    mesh_load_quad_xy(low, high, mesh);
}

glh::SceneTree::Node* add_quad_to_scene(glh::GraphicsManager* gm, glh::ProgramHandle& program,
                       glh::vec2 pos, glh::vec2 dims){

    glh::DefaultMesh* mesh = gm->create_mesh();
    load_screenquad(pos, dims, *mesh);

    auto renderable = gm->create_renderable();

    renderable->bind_program(program);
    renderable->set_mesh(mesh);

    auto root = scene.root();
    return scene.add_node(root, renderable);
}

bool init(glh::App* app)
{
    using namespace glh;

    GraphicsManager* gm = app->graphics_manager();

    //sp_colored_program = gm->create_program(sp_obj, sh_geometry, sh_vertex_obj, sh_fragment);
    sp_select_program  = gm->create_program(sp_select, sh_geometry, sh_vertex_obj, sh_fragment_selection_color);
    sp_colored_program = gm->create_program(sp_obj, sh_geometry, sh_vertex_obj, sh_fragment_fix_color);

    struct{vec2 dims; vec2 pos; vec4 color;} quads[] ={
        {vec2(0.5, 0.5), vec2(-0.5, -0.5), vec4(COLOR_RED)},
        {vec2(0.5, 0.5), vec2(0.5, 0.5), vec4(COLOR_GREEN)},
        {vec2(0.5, 0.5), vec2(-0.5, 0.5), vec4(COLOR_BLUE)},
        {vec2(0.25, 0.25), vec2(0.5, -0.5), vec4(COLOR_YELLOW)}
    };

    size_t quad_count = static_array_size(quads);

    for(auto& s: quads){
        auto n = add_quad_to_scene(gm, *sp_colored_program, s.pos, s.dims);
        set_material(*n, FIXED_COLOR,  s.color);}

#if 0
    vec2 dims = vec2(0.5, 0.5);
    vec2 pos = vec2(-0.5, -0.5);
    auto n0 = add_quad_to_scene(gm, *sp_colored_program, pos, dims);
    set_material(*n0, FIXED_COLOR,  vec4(COLOR_RED));

    pos = vec2(0.5, 0.5);
    auto n1 = add_quad_to_scene(gm, *sp_colored_program, pos, dims);
    set_material(*n1, FIXED_COLOR,  vec4(COLOR_GREEN));

    pos = vec2(-0.5, 0.5);
    auto n2 = add_quad_to_scene(gm, *sp_colored_program, pos, dims);
    set_material(*n2, FIXED_COLOR,  vec4(COLOR_BLUE));

    pos = vec2(0.5, -0.5); dims *= 0.5f;
    auto n3 = add_quad_to_scene(gm, *sp_colored_program, pos, dims);
    set_material(*n3, FIXED_COLOR,  vec4(COLOR_YELLOW));
#endif

    init_uniform_data();

    return true;
}

bool update(glh::App* app)
{
    using namespace glh;

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


void render_with_selection_pass(glh::App* app)
{
    using namespace glh;

    GraphicsManager* gm = app->graphics_manager();

    int w = app->config().width;
    int h = app->config().height;

    Box<int,2> screen_bounds = make_box2(0, 0, w, h);
    int mousey = h - g_mouse_y;
    Box<int,2> mouse_bounds  = make_box2(g_mouse_x - 1, mousey - 1, g_mouse_x + 2, mousey + 2);
    Box<int,2> read_bounds;
    bool       bounds_ok;

    std::tie(read_bounds, bounds_ok) = intersect(screen_bounds, mouse_bounds);
    auto read_dims = read_bounds.size();


    glDisable(GL_SCISSOR_TEST);
    //apply(g_renderpass_settings);

    //glEnable(GL_SCISSOR_TEST);
    //glScissor(read_bounds.min[0], read_bounds.min[1], read_dims[0], read_dims[1]);


    if(bounds_ok && g_read_color_at_mouse){

        GLenum read_format = GL_BGRA;
        GLenum read_type = GL_UNSIGNED_INT_8_8_8_8_REV;
        const int pixel_count = 9;
        const int pixel_channels = 4;
        uint8_t imagedata[pixel_count * pixel_channels] = {0};
        glFlush();
        glReadBuffer(GL_BACK);
        glReadPixels(read_bounds.min_[0], read_bounds.min_[1], read_dims[0], read_dims[1], read_format, read_type, imagedata);

        const uint8_t* b = &imagedata[4 * pixel_channels];
        const uint8_t* g = b + 1;
        const uint8_t* r = g + 1;
        const uint8_t* a = r + 1;

        char buf[1024];
        sprintf(buf, "r%hhu g%hhu b%hhu a%hhu", *r, *g, *b, *a);

        std::cout << "Mouse read:" << buf << std::endl;

        int read_id = ColorSelection::id_of_color(*r,*g,*b);

        g_read_color_at_mouse = false;
    }


    // ^
    // |   Uses the result from previous render pass

    apply(g_renderpass_settings);
    //apply(g_color_mask_settings);
    //apply(g_blend_settings);
    //gm->render(screenquad_image, env);

}

void render(glh::App* app)
{
    using namespace glh;

    GraphicsManager* gm = app->graphics_manager();

    apply(g_renderpass_settings);
    //render_queueue.render(gm, env);
    render_queueue.render(gm, *sp_select_program, env);
}

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

void mouse_button_callback(int key, const glh::Input::ButtonState& s)
{
    using namespace glh;

    if(s == glh::Input::Held) std::cout << "Mouse down." << std::endl;

    if(key == Input::LeftButton && (s == glh::Input::Held)) g_read_color_at_mouse = true;
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

    uicontext = std::make_shared<glh::UIContext>(app);

    GLH_LOG_EXPR("Logger started");
    add_key_callback(app, key_callback);
    add_mouse_button_callback(app, mouse_button_callback);
    add_mouse_move_callback(app, mouse_move_callback);
    glh::default_main(app);

    return 0;
}
