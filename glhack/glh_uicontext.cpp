/**\file glh_uicontext.cpp Event callbacks, input device state callbacks etc.
    \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/


#include "glh_uicontext.h"

namespace glh{

const char* uictx_sp_select  = "sp_selecting";
const char* sh_geometry = "";

const char* uictx_sh_vertex_obj   = 
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

const char* uictx_sh_fragment_selection_color = 
"#version 150                  \n"
"uniform vec4 " UICTX_SELECT_NAME ";"
"out vec4 FragColor;           "
"void main() {                 "
"    FragColor = " UICTX_SELECT_NAME "; "
"}";


void UiContext::init_assets(){

    add_mouse_move_callback(app_, std::bind( &UiContext::move, this, std::placeholders::_1, std::placeholders::_2));

    sp_select_program_  = manager_.create_program(uictx_sp_select, sh_geometry,
            uictx_sh_vertex_obj, uictx_sh_fragment_selection_color);

    render_picker_.selection_program_ = sp_select_program_;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// Techniques
///////////////////////////////////////////////////////////////////////////////////////////////////////////

#define COLOR_DELTA     "ColorDelta"
#define PRIMARY_COLOR   "PrimaryColor"
#define SECONDARY_COLOR "SecondaryColor"
#define FIXED_COLOR     "FixedColor"

void UiContext::Technique::color_interpolation(App& app, DynamicGraph& graph, StringNumerator& string_numerator, SceneTree::Node* node){
    auto sys      = DynamicNodeRef(new SystemInput(&app), "sys", graph);
    auto ramp     = DynamicNodeRef(new ScalarRamp(), string_numerator("ramp"), graph);
    auto dynvalue = DynamicNodeRef(new LimitedIncrementalValue(0.0, 0.0, 1.0), string_numerator("dynvalue"), graph);
    auto offset   = DynamicNodeRef(new ScalarOffset(), string_numerator("offset"), graph);
    auto mix      = DynamicNodeRef(new MixNode(), string_numerator("mix"), graph);

    std::list<std::string> vars = list(std::string(COLOR_DELTA), 
                                       std::string(PRIMARY_COLOR),
                                       std::string(SECONDARY_COLOR));

    auto nodesource = DynamicNodeRef(new NodeSource(node, vars),string_numerator("nodesource"), graph);
    auto nodereciever = DynamicNodeRef(new NodeReciever(node),string_numerator("nodereciever"), graph);

    graph.add_link(sys.name_, GLH_PROPERTY_TIME_DELTA, offset.name_, GLH_PROPERTY_INTERPOLANT);
    graph.add_link(nodesource.name_, COLOR_DELTA, offset.name_, GLH_PROPERTY_SCALE);

    graph.add_link(offset.name_, GLH_PROPERTY_INTERPOLANT, dynvalue.name_, GLH_PROPERTY_DELTA);
    graph.add_link(dynvalue.name_, GLH_PROPERTY_INTERPOLANT,  ramp.name_, GLH_PROPERTY_INTERPOLANT);

    graph.add_link(ramp.name_, GLH_PROPERTY_INTERPOLANT, mix.name_, GLH_PROPERTY_INTERPOLANT);
    graph.add_link(nodesource.name_, PRIMARY_COLOR, mix.name_, GLH_PROPERTY_1);
    graph.add_link(nodesource.name_, SECONDARY_COLOR, mix.name_, GLH_PROPERTY_2);

    graph.add_link(mix.name_, GLH_PROPERTY_COLOR, nodereciever.name_, FIXED_COLOR);
}


} // namespace glh
