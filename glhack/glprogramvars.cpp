/** \file glprogramvars.cpp OpenGL shader program type utilities. 
Targeted OpenGL version: 3.2. Targeted GLSL version: 1.5.
 
 \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/
#include "glprogramvars.h"

namespace glh {

/////////////////// ShaderVar ///////////////////
size_t ShaderVar::type_count(Type t){
    switch(t){
        case Vec2: return 2;
        case Vec3: return 3;
        case Vec4: return 4;
        case Mat4: return 16;
        case Sampler2D:
        case TYPE_LAST:
        default: return 0;
    }
}

ShaderTypeTokens shader_type_tokens()
{
    ShaderTypeTokens map;
    add(map, ShaderVar::Vec2, "vec2")
            (ShaderVar::Vec3, "vec3")
            (ShaderVar::Vec4, "vec4")
            (ShaderVar::Mat4, "mat4")
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

void assign(const GLuint program, const char* name, const vec3& vec){
    GLint location = glGetUniformLocation(program, name);
    if(location != -1) glUniform3fv(location, 1, vec.data());
    //else               assert("Applying to non-existing location");
}
void assign(const GLuint program, const char* name, const vec4& vec){
    GLint location = glGetUniformLocation(program, name);
    if(location != -1) glUniform4fv(location, 1, vec.data());
    //else               assert("Applying to non-existing location");
}
void assign(const GLuint program, const char* name, const mat4& mat){
    GLint location = glGetUniformLocation(program, name);
    if(location != -1) glUniformMatrix4fv(location, 1, GL_FALSE, mat.data());
    //else               assert("Applying to non-existing location");
}
void assign(const GLuint program, const char* name, const Texture& tex){
    GLint location = glGetUniformLocation(program, name);
    if(location != -1) glUniform1i(location,  tex.texture_unit_);
    //else               assert("Applying to non-existing location");
}


} // namespace glh

