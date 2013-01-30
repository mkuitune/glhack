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

namespace glh {

////////////// Shaders //////////////////////

/** Typings and a definition for one external shader parameter. */
class ShaderVar {
public:
    enum Mapping{Uniform, StreamIn, StreamOut, MAPPING_LAST};
    enum Type{Vec3, Vec4, Mat4, TYPE_LAST};

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


/** User space storage of shader program variables. */
struct Var_t {
    ShaderVar::Type type_;
    float           data_[16];
    Var_t():type_(ShaderVar::Vec3){clear();}
    Var_t(ShaderVar::Type type):type_(type){clear();}
    Var_t(ShaderVar::Type type, float* data){set(type, data);}

    void clear(){for(auto &d : data_) d = 0.f;}
    void set(ShaderVar::Type type, float* data);
    void assign(GLint location) const;

    static Var_t make_var(ShaderVar::Type type, float* data){return Var_t(type, data);}
};

void assign(const GLuint program, const char* name, const Var_t& var);

typedef std::map<std::string, Var_t> VarMap;

inline const std::string& iname(const VarMap::iterator& i){return i->first;};
inline const Var_t&       ivar(const VarMap::iterator& i){return i->second;};

void set(VarMap& m, const char* var, ShaderVar::Type t, float* data);
VarMap::iterator find_var(VarMap& m, const std::string& name, ShaderVar::Type type);


} // namespace glh
