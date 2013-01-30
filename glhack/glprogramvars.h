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
    mat4            mat4_;
    vec4            vec4_;
    vec3            vec3_;
    ShaderVar::Type type_;

    Var_t():type_(ShaderVar::Vec3){clear();}

    Var_t(const vec3& vec){set_vec3(vec);}
    Var_t(const vec4& vec){set_vec4(vec);}
    Var_t(const mat4& mat){set_mat4(mat);}

    void clear(){vec3_ = vec3::Zero(); vec4_ = vec4::Zero(); mat4_ = mat4::Zero(); }
    void set_vec3(const vec3& vec){type_ = ShaderVar::Vec3; vec3_ = vec;}
    void set_vec4(const vec4& vec){type_ = ShaderVar::Vec4; vec4_ = vec;}
    void set_mat4(const mat4& mat){type_ = ShaderVar::Mat4; mat4_ = mat;}
    void assign(GLint location) const;

    static Var_t make_vec3(const vec3& vec){return Var_t(vec);}
    static Var_t make_vec4(const vec4& vec){return Var_t(vec);}
    static Var_t make_mat4(const mat4& mat){return Var_t(mat);}
};

void assign(const GLuint program, const char* name, const Var_t& var);

typedef std::map<std::string, Var_t> VarMap;

inline const std::string& iname(const VarMap::iterator& i){return i->first;};
inline const Var_t&       ivar(const VarMap::iterator& i){return i->second;};
VarMap::iterator          find_var(VarMap& m, const std::string& name, ShaderVar::Type type);


} // namespace glh
