/** \file glprogramvars.h OpenGL shader program type utilities. 
Targeted OpenGL version: 3.2. Targeted GLSL version: 1.5.
 
 \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/
#pragma once


#include "glbuffers.h"
#include "shims_and_types.h"
#include "gltexture.h"

#include <tuple>
#include <list>
#include <string>
#include <vector>
#include <memory>
#include <cstring>
#include <type_traits>

namespace glh {

////////////// Shaders //////////////////////

/** Typings and a definition for one external shader parameter. */
class ShaderVar {
public:
    enum Mapping{Uniform, StreamIn, StreamOut, MAPPING_LAST};
    enum Type{Scalar, Vec2, Vec3, Vec4, Mat4, Sampler2D, TYPE_LAST};

    Mapping     mapping;
    Type        type;
    std::string name;

    GLuint program_location;
    int texture_unit_; //> Texture unit to use, specific for a program.

    ShaderVar(Mapping m, Type t, cstring& varname):mapping(m), type(t), name(varname), program_location(0){}

    static size_t type_count(Type t);

    template<class FUN>
    static void for_Mapping(FUN f)
    {
        f(Uniform);
        f(StreamIn);
        f(StreamOut);
    }

    typedef std::tuple<TypeId::t, int32_t> type_dim;

    static type_dim typeid_and_dim(Type t)
    {
        switch(t){
            case Scalar: return std::make_tuple<TypeId::t, int32_t>(TypeId::Float32, 1);
            case Vec2: return std::make_tuple<TypeId::t, int32_t>(TypeId::Float32, 2);
            case Vec3: return std::make_tuple<TypeId::t, int32_t>(TypeId::Float32, 3);
            case Vec4: return std::make_tuple<TypeId::t, int32_t>(TypeId::Float32, 4);
            default:  assert(!"Unsupported type"); return std::make_tuple<TypeId::t, int32_t>(TypeId::Float32, 0);
        }
    }

};


typedef std::list<ShaderVar> ShaderVarList;

typedef BiMap<ShaderVar::Type, std::string> ShaderTypeTokens;
typedef BiMap<ShaderVar::Mapping, std::string> ShaderMappingTokens;

ShaderTypeTokens    shader_type_tokens();
ShaderMappingTokens shader_mapping_tokens();

} // namespace glh
