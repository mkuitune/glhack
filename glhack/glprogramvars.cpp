/** \file glprogramvars.cpp OpenGL shader program type utilities. 
Targeted OpenGL version: 3.2. Targeted GLSL version: 1.5.
 
 \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/
#include "glprogramvars.h"

namespace glh {

/////////////////// ShaderVar ///////////////////
size_t ShaderVar::type_count(Type t){
    switch(t){
        case Vec3: return 3;
        case Vec4: return 4;
        case Mat4: return 16;
        case TYPE_LAST:
        default: return 0;
    }
}

/////////////////// Var_t ///////////////////

void Var_t::assign(GLint location) const{
         if(type_ == ShaderVar::Vec3) glUniform3fv(location, 1, vec3_.data());
    else if(type_ == ShaderVar::Vec4) glUniform4fv(location, 1, vec4_.data());
    else if(type_ == ShaderVar::Mat4) glUniformMatrix4fv(location, 1, GL_FALSE, mat4_.data());
}

void assign(const GLuint program, const char* name, const Var_t& var){
    GLint location = glGetUniformLocation(program, name);
    if(location != -1) var.assign(location);
    else               assert("Applying to non-existing location");
}

VarMap::iterator find_var(VarMap& m, const std::string& name, ShaderVar::Type type){
    auto i = m.find(name);
    if(i != m.end() && ivar(i).type_ == type) return i;
    else                                      return m.end();
}

} // namespace glh

