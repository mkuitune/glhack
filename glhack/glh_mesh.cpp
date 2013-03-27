/**\file glh_index.h Maintain relations between names and indices.
    \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/
#include "glh_mesh.h"


namespace glh{


//////////////////////// DefaultMesh ////////////////////////

DefaultMesh::DefaultMesh(){load_defaults(*this);}

VertexChunk& DefaultMesh::get(const ChannelType::s& s){return chunks_[s];}

//void DefaultMesh::update_normals()
//{
//    //Set normalbuffer based on position buffer
//    VertexChunk &normals(*chunks_[ChannelType::Normal]);
//    VertexChunk &pos(*chunks_[ChannelType::Position]);
//    // TODO not needed? Normals should come from external source?
//}

void DefaultMesh::load_defaults(DefaultMesh& d)
{
    d.chunks_.emplace(ChannelType::Position, VertexChunk(glh::BufferSignature(glh::TypeId::Float32, 3)));
    d.chunks_.emplace(ChannelType::Normal,   VertexChunk(glh::BufferSignature(glh::TypeId::Float32, 3)));
    d.chunks_.emplace(ChannelType::Color,    VertexChunk(glh::BufferSignature(glh::TypeId::Float32, 3)));
    d.chunks_.emplace(ChannelType::Texture,  VertexChunk(glh::BufferSignature(glh::TypeId::Float32, 3)));
}


//////////////////////// Mesh utilities ////////////////////////

void mesh_load_screenquad(glh::DefaultMesh& mesh)
{
    float posdata[] = {
        -1.0f, -1.0f, 0.0f,
         1.0f, -1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f,

         1.0f, -1.0f, 0.0f,
         1.0f,  1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f
    };
    size_t posdatasize = static_array_size(posdata);

    float normaldata[] = {
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,

        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f
    };
    size_t normaldatasize = static_array_size(normaldata);

    float coldata[] = {
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,

        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f
    };
    size_t coldatasize = static_array_size(coldata);

    float texdata[] = {
        0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,

        1.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f
    };
    size_t texdatasize = static_array_size(texdata);

    mesh.get(glh::ChannelType::Position).set(posdata, posdatasize);
    mesh.get(glh::ChannelType::Color).set(coldata, coldatasize);
    mesh.get(glh::ChannelType::Normal).set(normaldata, normaldatasize);
    mesh.get(glh::ChannelType::Texture).set(texdata, texdatasize);
}

void mesh_load_screenquad_pixelcoords(float w, float h, glh::DefaultMesh& mesh)
{
    float posdata[] = {
        .0f, .0f, 0.0f,
         w, .0f, 0.0f,
        .0f, h, 0.0f,

         w, .0f, 0.0f,
         w,  h, 0.0f,
        .0f, h, 0.0f
    };
    size_t posdatasize = static_array_size(posdata);

    float normaldata[] = {
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,

        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f
    };
    size_t normaldatasize = static_array_size(normaldata);

    float coldata[] = {
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,

        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f
    };
    size_t coldatasize = static_array_size(coldata);

    float texdata[] = {
        0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,

        1.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f
    };
    size_t texdatasize = static_array_size(texdata);

    mesh.get(glh::ChannelType::Position).set(posdata, posdatasize);
    mesh.get(glh::ChannelType::Color).set(coldata, coldatasize);
    mesh.get(glh::ChannelType::Normal).set(normaldata, normaldatasize);
    mesh.get(glh::ChannelType::Texture).set(texdata, texdatasize);
}


}// end namespace glh
