/**\file glh_mesh.h 
    \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/
#pragma once
#include "glh_typedefs.h"
#include "glbuffers.h"
#include "shims_and_types.h"

namespace glh{

typedef std::map<ChannelType::s, VertexChunk> ChunkMap;


class DefaultMesh {
public:
    ChunkMap chunks_;

    DefaultMesh();

    VertexChunk& get(const ChannelType::s& s);
    //void         update_normals();

    static void load_defaults(DefaultMesh& d);
};

typedef std::shared_ptr<DefaultMesh> DefaultMeshPtr;

/** Load screenquad data to mesh using default channels. */
void mesh_load_screenquad(glh::DefaultMesh& mesh);
void mesh_load_quad_xy(vec2 low, vec2 high, glh::DefaultMesh& mesh);
void mesh_load_screenquad(float w, float h, glh::DefaultMesh& mesh);

}
