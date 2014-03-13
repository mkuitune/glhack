/**\file glh_scenemanagement.cpp
    \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/

#include "glh_scenemanagement.h"
#include "glh_typedefs.h"
#include "glsystem.h"
#include <cstring>

namespace glh {

PathArray string_to_patharray(const std::string& str)
{
    PathArray result;

    size_t last = 0;
    size_t index = str.find("/");

    if(index == std::string::npos){
        result.push_back(str);
    }
    else{
        while(index != std::string::npos){
            result.push_back(std::string(str, last, index - last));
            last = index + 1;
            index = str.find("/", last);
        }
    }

    return result;
}

void set_material(SceneTree::Node& node, RenderEnvironment& material){
    node.material_ = material;}

void append_material(SceneTree::Node& node, RenderEnvironment& material){
    node.material_.append(material);}

void set_material(SceneTree::Node& node, cstring& name, const vec4& var){
    node.material_.set_vec4(name, var);}

void set_material(SceneTree::Node& node, cstring& name, const float var){
    node.material_.set_scalar(name, var);}

SceneTree::iterator begin_iter(SceneTree::Node* node){return SceneTree::tree_iterator(node);}
SceneTree::iterator end_iter(SceneTree::Node* node){return SceneTree::tree_iterator(0);}

//
// Scene filters
//

bool pass_all(SceneTree::Node* node){ return true; }
bool pass_pickable(SceneTree::Node* node){ return node->pickable_; }
bool pass_unpickable(SceneTree::Node* node){ return !node->pickable_; }
bool pass_interaction_unlocked(SceneTree::Node* node){ return !node->interaction_lock_; }
bool pass_interaction_locked(SceneTree::Node* node){ return node->interaction_lock_; }
bool pass_pickable_and_unlocked(SceneTree::Node* node){ return pass_pickable(node) && pass_interaction_unlocked(node); }

}
