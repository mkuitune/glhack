/**\file glh_scene_util.cpp
    Utilities and extensions to extend and manipulate scene data.
    \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/
#pragma once

#include "glh_scenemanagement.h"

namespace glh{

void load_screenquad(vec2 size, glh::DefaultMesh& mesh);
SceneTree::Node* add_quad_to_scene(GraphicsManager* gm, SceneTree& scene, ProgramHandle& program, vec2 dims, SceneTree::Node* parent);

}
