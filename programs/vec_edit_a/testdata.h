#pragma once

#include <vector>
#include <map>

#include "glh_scenemanagement.h"
#include "glh_typedefs.h"

struct Shape2D{
    float x, y;
};

class Scene{
public:

    glh::ObjectRoster::IdGenerator id_gen_;

    std::map<int, Shape2D> shapes_;

    void insert(float x, float y){
        int newid = id_gen_.new_id();
        shapes_[newid] = {x, y};
    }
};


void display_scene(Scene& scene, glh::SceneTree& tree);

