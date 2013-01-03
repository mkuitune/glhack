/** \file geometry.cpp Geometry functions and utilities. 
    \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/

#include "geometry.h"
#include <vector>

namespace glh
{

/** Interface for meshes that are just triangle soup.*/
class UnstructuredMeshImp : public UnstructuredMesh
{
public:



    virtual ~UnstructuredMeshImp(){}

    virtual void add_face(const Polygon3d& p, const vec3& normal) override
    {
        // First figure out the winding direction for the polygon



        // Then proceed to triangulate the polygon by ear clipping


    }

    /** @return Pointer to vertex data*/
    virtual float* vertices() override
    {
        return (float*) &triangles_[0];
    }

    /** @return The number of triangles.*/
    virtual int triangle_count() override
    {
        return triangles_.size();
    }

    std::vector<Triangle> triangles_;
};

UnstructuredMesh* create_mesh()
{
    UnstructuredMeshImp* mesh = new UnstructuredMeshImp();
    return mesh;
}

}
