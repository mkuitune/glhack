/** \file glhack.h OpenGL platform agnostic rendering and other utilities. 
Targeted OpenGL version: 3.2. Targeted GLSL version: 1.5.
 
 \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/
#pragma once


#include "glprogramvars.h"
#include "glh_mesh.h"
#include "gltexture.h"

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
    virtual const ShaderVarList& inputs() = 0;
    //virtual void use() = 0;
    //virtual void bind_vertex_input(NamedBufferHandles& buffers) = 0;
    //virtual void bind_uniforms(VarMap& vmap) = 0;
    //virtual void draw() = 0;
);

GLuint program_handle(const ProgramHandle* handle_);

/** Add handle per each input buffer found in program.*/
void init_bufferset_from_program(BufferSet& bufs, ProgramHandle* h);

/** Map chunk of type ChannelType::<name> to input known by 'name' if
 *  First three digits of <name> match with any position in 'name'.*/
void assign_by_guessing_names(BufferSet& bufs, DefaultMesh& mesh);

/** Reference to a bound program.
    The lifetime of ActiveProgram may not exceed that of the bound program handle. */
class ActiveProgram {
public:
    ActiveProgram();
    void bind_vertex_input(NamedBufferHandles& buffers);
    void draw();
    void bind_uniform(const std::string& name, const mat4& mat);
    void bind_uniform(const std::string& name, const vec4& vec);
    void bind_uniform(const std::string& name, const Texture& tex);

    template<class T> void bind_uniform(const NamedVar<T>& named_var){bind_uniform(named_var.name_, named_var.var_);}

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
