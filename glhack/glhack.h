/** \file glhack.h OpenGL platform agnostic rendering and other utilities. 
Targeted OpenGL version: 3.2. Targeted GLSL version: 1.5.
 
 \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/

#pragma once

#ifdef WIN32
#define NOMINMAX
#include<windows.h>
#endif

#include<GL/glew.h>
#include <GL/glfw.h>

#include "glh_typedefs.h"
#include "shims_and_types.h"

#include <list>
#include <string>
#include <vector>
#include <memory>


//TODO: Better logging.
bool          glh_logging_active();
std::ostream* glh_get_log_ptr();

#define GLH_LOG_EXPR(expr_param) \
    do { if ( glh_logging_active() ){\
    (*glh_get_log_ptr()) << __FILE__ \
    << " [" << __LINE__ << "] : " << expr_param \
    << ::std::endl;} }while(false)



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
typedef ShaderProgram* ProgramHandle;

/////////// Shader program functions ///////////

const char* program_name(ProgramHandle p);
bool        valid(ProgramHandle p);

//////////// Graphics context /////////////

DeclInterface(GraphicsManager,
    /** This function will create a shader program based on the source files passed to it*/
    virtual ProgramHandle create_program(cstring& name, cstring& geometry, cstring& vertex, cstring& fragment) = 0;
    /** Find shader by name. If not found return empty handle. */
    virtual ProgramHandle program(cstring& name) = 0;
    virtual void          use_program(ProgramHandle h) = 0;
);

GraphicsManager* make_graphics_manager();


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

///////////// OpenGL Utilities /////////////

/** Check GL error. @return true if no error found. */
bool check_gl_error();

} // namespace glh
