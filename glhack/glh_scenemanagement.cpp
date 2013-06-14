/**\file glh_scenemanagement.cpp
    \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/

#include "glh_scenemanagement.h"

#include "glh_typedefs.h"

#include "glsystem.h"


namespace glh {

void set_material(SceneTree::Node& node, RenderEnvironment& material){
    if(auto r = node.renderable()){
        r->material_ = material;}
}

void set_material(SceneTree::Node& node, cstring& name, const vec4& var){
    if(auto r = node.renderable()){
        r->material_.set_vec4(name, var);}
}


}
