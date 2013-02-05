/**\file glh_mesh.h 
    \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/
#pragma once
#include "glh_typedefs.h"
#include "glbuffers.h"
#include "shims_and_types.h"

namespace glh{

typedef std::map<ChannelType::s, std::shared_ptr<VertexChunk>> ChunkMap;


class DefaultMesh {
public:
    ChunkMap chunks_;

    DefaultMesh(){load_defaults(*this);}

    VertexChunk& get(const ChannelType::s& s){return *chunks_[s];}

    void update_normals()
    {
        //Set normalbuffer based on position buffer
        VertexChunk &normals(*chunks_[ChannelType::Normal]);
        VertexChunk &pos(*chunks_[ChannelType::Position]);
        // TODO

    }

    static void load_defaults(DefaultMesh& d)
    {
        d.chunks_[ChannelType::Position] = std::make_shared<VertexChunk>(glh::BufferSignature(glh::TypeId::Float32, 3));
        d.chunks_[ChannelType::Normal]   = std::make_shared<VertexChunk>(glh::BufferSignature(glh::TypeId::Float32, 3));
        d.chunks_[ChannelType::Color]    = std::make_shared<VertexChunk>(glh::BufferSignature(glh::TypeId::Float32, 3));
        d.chunks_[ChannelType::Texture]  = std::make_shared<VertexChunk>(glh::BufferSignature(glh::TypeId::Float32, 3));
    }
};

}
