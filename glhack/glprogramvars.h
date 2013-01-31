/** \file glprogramvars.h OpenGL shader program type utilities. 
Targeted OpenGL version: 3.2. Targeted GLSL version: 1.5.
 
 \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/
#pragma once


#include "glbuffers.h"
#include "shims_and_types.h"

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
    enum Type{Vec2, Vec3, Vec4, Mat4, TYPE_LAST};

    Mapping     mapping;
    Type        type;
    std::string name;

    GLuint program_location;

    ShaderVar(Mapping m, Type t, cstring& varname):mapping(m), type(t), name(varname), program_location(0){}

    static size_t type_count(Type t);

    template<class FUN>
    static void for_Mapping(FUN f)
    {
        f(Uniform);
        f(StreamIn);
        f(StreamOut);
    }
};


typedef std::list<ShaderVar> ShaderVarList;

typedef BiMap<ShaderVar::Type, std::string> ShaderTypeTokens;
typedef BiMap<ShaderVar::Mapping, std::string> ShaderMappingTokens;

ShaderTypeTokens    shader_type_tokens();
ShaderMappingTokens shader_mapping_tokens();

void assign(const GLuint program, const char* name, const vec4& vec);
void assign(const GLuint program, const char* name, const mat4& mat);

template<class T> struct NamedVar{
    const std::string name_;
    T                 var_;
    NamedVar(const char* name):name_(name){}
    T& operator=(const T& t){var_ = t; return var_;}
};

template<class T> void named_assign(const GLuint program, const T& named_var){assign(program, named_var.name_.c_str(), named_var.var_);}

} // namespace glh
