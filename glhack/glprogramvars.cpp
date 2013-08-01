/** \file glprogramvars.cpp OpenGL shader program type utilities. 
Targeted OpenGL version: 3.2. Targeted GLSL version: 1.5.
 
 \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/
#include "glprogramvars.h"

namespace glh {

/////////////////// ShaderVar ///////////////////
size_t ShaderVar::type_count(Type t){
    switch(t){
        case Scalar: return 1;
        case Vec2:   return 2;
        case Vec3:   return 3;
        case Vec4:   return 4;
        case Mat4:   return 16;
        case Sampler2D:
        case TYPE_LAST:
        default: return 0;
    }
}

ShaderTypeTokens shader_type_tokens()
{
    ShaderTypeTokens map;
    add(map, ShaderVar::Scalar, "float")
            (ShaderVar::Vec2,   "vec2")
            (ShaderVar::Vec3,   "vec3")
            (ShaderVar::Vec4,   "vec4")
            (ShaderVar::Mat4,   "mat4")
            (ShaderVar::Sampler2D, "sampler2D");

    return map;
}

ShaderMappingTokens shader_mapping_tokens()
{
    ShaderMappingTokens map;
    add(map, ShaderVar::Uniform, "uniform")
            (ShaderVar::StreamIn, "in")
            (ShaderVar::StreamOut, "out");

    return map;
}



} // namespace glh

