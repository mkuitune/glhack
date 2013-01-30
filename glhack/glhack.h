/** \file glhack.h OpenGL platform agnostic rendering and other utilities. 
Targeted OpenGL version: 3.2. Targeted GLSL version: 1.5.
 
 \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/
#pragma once


#include "glbuffers.h"
#include "glprogramvars.h"
#include "shims_and_types.h"


#include <list>
#include <string>
#include <vector>
#include <memory>
#include <cstring>

namespace glh {


std::ostream& operator<<(std::ostream& os, const ShaderVar& v);


typedef std::list<ShaderVar> ShaderVarList;

/** Opaque pointer to a shader program */
DeclInterface(ProgramHandle,
    virtual const char* name() = 0;
    //virtual void use() = 0;
    //virtual void bind_vertex_input(NamedBufferHandles& buffers) = 0;
    //virtual void bind_uniforms(VarMap& vmap) = 0;
    //virtual void draw() = 0;
);

/** Reference to a bound program.
    The lifetime of ActiveProgram may not exceed that of the bound program handle. */
class ActiveProgram {
public:
    ActiveProgram();
    void bind_vertex_input(NamedBufferHandles& buffers);
    void bind_uniforms(VarMap& vmap);
    void bind_uniform(const std::string& name, const Var_t& var);
    void draw();

    ProgramHandle* handle_;
private:
    int32_t        component_count_; 
};

ActiveProgram make_active(ProgramHandle& h);

//////////// Graphics context /////////////

DeclInterface(GraphicsManager,
    /** This function will create a shader program based on the source files passed to it*/
    virtual ProgramHandle* create_program(cstring& name, cstring& geometry, cstring& vertex, cstring& fragment) = 0;
    /** Find shader program by name. If not found return empty handle. */
    virtual ProgramHandle* program(cstring& name) = 0;
    // TODO: Add create buffer handle (so gl functions are not used before possible)

);

GraphicsManager* make_graphics_manager();


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

} // namespace glh
