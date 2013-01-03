/** \file glhack.h OpenGL platform agnostic rendering and other utilities. 
Targeted OpenGL version: 3.2. Targeted GLSL version: 1.5.
 
 \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/

#pragma once

#include "glbase.h"
#include "glh_typedefs.h"

#include <list>
#include <string>
#include <vector>
#include <memory>

namespace glh
{


///////////// Typedefs ////////////

typedef GLuint          bitfield_int;


///////////// OpenGL State management ////////////

bool check_gl_error(const char* msg);


////////////// Shaders //////////////////////

/** Typings and a definition for one external shader parameter. */
class ShaderVar
{
public:
    enum Mapping{Uniform, StreamIn, StreamOut};
    enum Type{Vec3, Vec4, Mat4};

    Mapping     mapping;
    Type        type;
    std::string name;

    GLuint program_location;

    ShaderVar(Mapping m, Type t, cstring& varname):mapping(m), type(t), name(varname), program_location(0){}
};

std::ostream& operator<<(std::ostream& os, const ShaderVar& v);

/** Generate the listing of the input and output variables for the particular shader. */
std::list<ShaderVar> parse_shader_vars(cstring& shader);

/** Opaque pointer to a shader program */
class ShaderProgram;

/** A handle to a ShaderProgram.*/
class ShaderProgramHandle
{
public:

    ShaderProgramHandle(ShaderProgram* program_param);
    ~ShaderProgramHandle();

    bool is_valid();
private:
     ShaderProgram* program; 
};

//////////// Graphics context /////////////

DeclInterface(GraphicsManager,
    /** This function will create a shader program based on the source files passed to it*/
    virtual ShaderProgramHandle create_shader_program(cstring& name, cstring& geometry, cstring& vertex, cstring& fragment) = 0;
    /** Find shader by name. If not found return empty handle. */
    virtual ShaderProgramHandle shader_program(cstring& name) = 0;
);

GraphicsManager* create_graphics_manager();

// TODO: Functions to map vars to shaders through ShaderProgramHandle

///////////// RenderPassSettings ///////////////

/** Settings for an individual render pass. */
class RenderPassSettings
{
public:

    enum Buffer{Color = GL_COLOR_BUFFER_BIT, Depth = GL_DEPTH_BUFFER_BIT, Stencil = GL_STENCIL_BUFFER_BIT};

    GLuint   clear_mask;
    vec4     clear_color;
    GLclampd clear_depth;

    RenderPassSettings(const GLuint clear_mask, const vec4& clear_color, const GLclampd clear_depth);
    void set_buffer_clear(Buffer buffer);
};

void apply(const RenderPassSettings& pass);


///////////// Misc ///////////////

/** Execute a minimal scene. Test that everything builds and runs etc.*/
void minimal_scene();


} // namespace glh
