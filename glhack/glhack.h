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
    virtual const ShaderVarList& uniforms() = 0;
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
    void bind_uniform(const std::string& name, const vec3& vec);
    void bind_uniform(const std::string& name, const Texture& tex);

    template<class T> void bind_uniform(const NamedVar<T>& named_var){bind_uniform(named_var.name_, named_var.var_);}

    ProgramHandle* handle_;
private:
    int32_t        component_count_;
};

ActiveProgram make_active(ProgramHandle& h);


//////////// Environment ////////////

class RenderEnvironment{
public:
    std::map<std::string, mat4, std::less<std::string>, Eigen::aligned_allocator<std::pair<std::string, mat4> >> mat4_;
    std::map<std::string, vec4,  std::less<std::string>, Eigen::aligned_allocator<std::pair<std::string, vec4> >> vec4_;
    std::map<std::string, std::shared_ptr<Texture> > texture2d_;

    bool has(cstring name, ShaderVar::Type type){
        if(type == ShaderVar::Vec4)           return has_key(vec4_, name);
        else if(type == ShaderVar::Mat4)      return has_key(mat4_, name);
        else if(type == ShaderVar::Sampler2D) return has_key(texture2d_, name);
        return false;
    }

    vec4&    get_vec4(cstring name);
    mat4&    get_mat4(cstring name);
    Texture& get_texture2d(cstring name);

    void set_vec4(cstring name, const vec4& vec){vec4_[name] = vec;}
    void set_mat4(cstring name, const mat4& mat){mat4_[name] = mat;}
    void set_texture2d(cstring name, std::shared_ptr<Texture> tex){texture2d_[name] = tex;}
};

void program_params_from_env(ActiveProgram& program, RenderEnvironment& env);

//////////// Renderable settings //////////////

DeclInterface(Renderable,
    virtual void render(RenderEnvironment& env) = 0;
);

class FullRenderable: public Renderable {
public:

    typedef std::shared_ptr<DefaultMesh> MeshPtr;

    ProgramHandle*    program_;
    RenderEnvironment material_;
    BufferSet         device_buffers_;
    MeshPtr           mesh_;

    bool meshdata_on_gpu_;

    FullRenderable():meshdata_on_gpu_(false){}

    void bind_program(ProgramHandle& program){
        program_ = &program;
        glh::init_bufferset_from_program(device_buffers_, program_);
    }

    void transfer_vertexdata_to_gpu(){
        if(mesh_.get()){
            glh::assign_by_guessing_names(device_buffers_, *mesh_); // Assign mesh data to buffers
            meshdata_on_gpu_ = true;
        }
    }

    void set_mesh(MeshPtr& mesh){
        mesh_ = mesh;
    }

    virtual void render(RenderEnvironment& env) override {

        if(!mesh_.get()) throw GraphicsException("FullRenderable: trying to render without bound mesh.");

        if(!meshdata_on_gpu_) transfer_vertexdata_to_gpu();

        auto active = glh::make_active(*program_);
        active.bind_vertex_input(device_buffers_.buffers_);

        program_params_from_env(active, env);
        program_params_from_env(active, material_);

        active.draw();
    }
};


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

    struct BlendSettings{
        bool blend_active_;
        GLenum  source_factor_;
        GLenum  dest_factor_;

        BlendSettings():blend_active_(false){}
        BlendSettings(GLenum  source_factor, GLenum  dest_factor):
            blend_active_(true), source_factor_(source_factor), dest_factor_(dest_factor){}

        void apply() const {
            if(blend_active_){
                glEnable(GL_BLEND);
                glBlendFunc(source_factor_, dest_factor_);
            }
            else{
                glDisable(GL_BLEND);
            }
        }
    };

    enum Buffer{Color = GL_COLOR_BUFFER_BIT, Depth = GL_DEPTH_BUFFER_BIT, Stencil = GL_STENCIL_BUFFER_BIT};

    GLuint   clear_mask;
    vec4     clear_color; bool clear_color_set;
    GLclampd clear_depth; bool clear_depth_set;
    BlendSettings blend;  bool blend_set;


    RenderPassSettings(const GLuint clear_mask, const vec4& clear_color, const GLclampd clear_depth);
    RenderPassSettings(BlendSettings& blend_settings);


    void set_buffer_clear(Buffer buffer);
};

void apply(const RenderPassSettings& pass);

} // namespace glh
