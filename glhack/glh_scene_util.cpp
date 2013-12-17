/**\file glh_scene_util.cpp
    \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/
#pragma once

#include "glh_scene_util.h"


namespace glh{

void load_screenquad(vec2 size, glh::DefaultMesh& mesh)
{
    using namespace glh;

    vec2 halfsize = 0.5f * size;

    vec2 low  = - halfsize;
    vec2 high = halfsize;

    mesh_load_quad_xy(low, high, mesh);
}

SceneTree::Node* add_quad_to_scene(GraphicsManager* gm, SceneTree& scene, ProgramHandle& program, vec2 dims, SceneTree::Node* parent){

    DefaultMesh* mesh = gm->create_mesh();
    load_screenquad(dims, *mesh);

    auto renderable = gm->create_renderable();

    renderable->bind_program(program);
    renderable->set_mesh(mesh);

    return scene.add_node(parent, renderable);
}

}
