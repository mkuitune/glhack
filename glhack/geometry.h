/** \file geometry.h Geometry functions and utilities. 
    \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)*/

#pragma once


#include "math_tools.h"
#include <list>

namespace glh {

/////////// Point operations ////////////
/** \section glh_point Point */

/** Container for 3d points. */
template<class T> struct Point{T x, y, z;};
typedef Point<float>  point3;
typedef Point<int>    point3i;
typedef Point<double> point3d;

template<class T>
Eigen::Matrix<T,3,1,0,3,1> sub(const Point<T>& b, const Point<T>& a)
{
    Eigen::Matrix<T,3,1,0,3,1>(b.x - a.x, b.y - a.y, b.z - a.z);
}

/** Cast point into 4-vector, vector representation (4th element is 0). */
template<class T>
Eigen::Matrix<T,4,1,0,4,1> vec_of_point(const Point<T>& b)
{Eigen::Matrix<T,4,1,0,4,1>(b.x, b.y, b.z, 0);}

/** Cast point into 4-vector, point representation (4th element is 1). */
template<class T>
Eigen::Matrix<T,4,1,0,4,1> point_of_point(const Point<T>& b)
{Eigen::Matrix<T,4,1,0,4,1>(b.x, b.y, b.z, 1);}


/////////// Triangle ////////////
/** \section glh_triangle Triangle */

struct Triangle{
    point3 p0, p1, p2;
    Triangle(const point3& a, const point3& b, const point3& c): p0(a), p1(b), p2(c) {}
};


/////////// Plane /////////////
/** \section glh_plane Plane*/
struct Plane{
    double a_,b_,c_,d_; //> ax + by + cz + d = 0
    Plane(double a, double b, double c, double d):a_(a), b_(b), c_(c), d_(d){}
};

/////////// Polygon operations ////////////
/** \section glh_polygon Polygon */

/** A polygon is a list of points. */
template<class T> struct Polygon{std::list<Point<T>> points;};

typedef Polygon<float>  Polygon3;
typedef Polygon<double> Polygon3d;
typedef Polygon<int>    Polygon3i;


/////////// Unstructured mesh ////////////
/** \section glh_unstructured_mesh Unstructured mesh */

/** Interface for meshes that are a set of counter-clockwise triangles with vertex data stored.*/
class UnstructuredMesh
{
public:
    virtual ~UnstructuredMesh(){}
    /** Add face to mesh from polygon. The polygon is triangularized and added to mesh.
        @param p      Polygon to add
        @param normal A vector pointing out from the surface that will be used to 
                      figure out the handedness of the polygon.
    */
    virtual void add_face(const Polygon3d& p, const vec3& normal);

    /** @returns Pointer to vertex data*/
    virtual float* vertices();

    /** @returns The number of triangles.*/
    virtual int triangle_count();
};

UnstructuredMesh* create_mesh();

}
