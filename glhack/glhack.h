/** \file glhack.h OpenGL platform agnostic rendering and other utilities. 
Targeted OpenGL version: 3.2. Targeted GLSL version: 1.5.
 
 \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/
#pragma once


#include "glprogramvars.h"
#include "glh_mesh.h"
#include "gltexture.h"
#include "glh_font.h"
#include "math_tools.h"

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


//////////// Environment ////////////

class RenderEnvironment{
public:

    struct ParamMap{
        RenderEnvironment& env_;
        const std::string& name_;
        ParamMap(RenderEnvironment& env, const std::string& name):env_(env), name_(name){}

        void operator=(float f){ env_.set_scalar(name_, f); }
        void operator=(Texture* t){ env_.set_texture2d(name_, t); }
        void operator=(const mat4& mat){ env_.set_mat4(name_, mat); }
        void operator=(const vec4& vec){ env_.set_vec4(name_, vec); }
    };

    std::map<std::string, float>    scalar_;
    std::map<std::string, Texture*> texture2d_;
    std::map<std::string, mat4, std::less<std::string>, Eigen::aligned_allocator<std::pair<std::string, mat4> >> mat4_;
    std::map<std::string, vec4,  std::less<std::string>, Eigen::aligned_allocator<std::pair<std::string, vec4> >> vec4_;

    bool has(cstring& name, ShaderVar::Type type){
             if(type == ShaderVar::Scalar)    return has_key(scalar_, name);
        else if(type == ShaderVar::Vec4)      return has_key(vec4_, name);
        else if(type == ShaderVar::Mat4)      return has_key(mat4_, name);
        else if(type == ShaderVar::Sampler2D) return has_key(texture2d_, name);
        return false;
    }

    std::pair<ShaderVar::Type, bool> has(cstring& name){
             if(has_key(scalar_, name))    return std::make_pair(ShaderVar::Scalar, true);
        else if(has_key(mat4_, name))      return std::make_pair(ShaderVar::Mat4, true);
        else if(has_key(vec4_, name))      return std::make_pair(ShaderVar::Vec4, true);
        else if(has_key(texture2d_, name)) return std::make_pair(ShaderVar::Sampler2D, true);
        else                               return std::make_pair(ShaderVar::TYPE_LAST, false);
    }

    void remove(cstring name){
        scalar_.erase(name);
        mat4_.erase(name);
        vec4_.erase(name);
        texture2d_.erase(name);
    }

    void append(const RenderEnvironment& r){
        scalar_.insert(r.scalar_.begin(), r.scalar_.end());
        texture2d_.insert(r.texture2d_.begin(), r.texture2d_.end());
        mat4_.insert(r.mat4_.begin(), r.mat4_.end());
        vec4_.insert(r.vec4_.begin(), r.vec4_.end());
    }

    float&   get_scalar(cstring name);
    vec4&    get_vec4(cstring name);
    mat4&    get_mat4(cstring name);
    Texture& get_texture2d(cstring name);

    void set_scalar(cstring name, const float val){scalar_[name] = val;}
    void set_vec4(cstring name, const vec4& vec){vec4_[name] = vec;}
    void set_mat4(cstring name, const mat4& mat){mat4_[name] = mat;}
    void set_texture2d(cstring name, Texture* tex){texture2d_[name] = tex;}

    ParamMap operator[](const std::string& str){ return ParamMap(*this, str); }
};

//////////// Renderable settings //////////////

class FullRenderable {
public:
    ProgramHandle*    program_;
    BufferSet         device_buffers_;
    DefaultMesh*      mesh_;

    bool meshdata_on_gpu_;
    bool meshdata_dirty_;

    FullRenderable():program_(0),meshdata_on_gpu_(false), mesh_(0), meshdata_dirty_(false){}

    void bind_program(ProgramHandle& program){
        program_ = &program;
        glh::init_bufferset_from_program(device_buffers_, program_);
    }

    void transfer_vertexdata_to_gpu(){
        if(mesh_){
            glh::assign_by_guessing_names(device_buffers_, *mesh_); // Assign mesh data to buffers
            meshdata_on_gpu_ = true;
            meshdata_dirty_ = false;
        }
    }

    void reset_buffers(){
        device_buffers_.reset();
        meshdata_on_gpu_ = false;
    }

    void resend_data_on_render(){
        meshdata_dirty_ = true;
    }

    void set_mesh(DefaultMesh* meshptr){
        mesh_ = meshptr;
        // TODO handle program bindings etc?
    }

    DefaultMesh* mesh(){return mesh_;}

};

typedef std::shared_ptr<FullRenderable> FullRenderablePtr;


//////////// Graphics context /////////////

// TODO rename to RenderAssetsManager ?

/** GraphicsManager handles opengl assets and instances of adapter classes that
 *  require a global state to function (that e.g. require an initialized GL context,
 *  need to synchronize resource usage and so on.)
 *  Try to allocate all gpu resources through this. That way we can have an index
 *  to query all sorts of data from if wanted.
 */
DeclInterface(GraphicsManager,

    /** This function will create a shader program based on the source files passed to it*/
    virtual ProgramHandle* create_program(cstring& name, cstring& geometry, cstring& vertex, cstring& fragment) = 0;
    
    /** Find shader program by name. If not found return empty handle. */
    virtual ProgramHandle* program(cstring& name) = 0;
    // TODO: Add create buffer handle (so gl functions are not used before possible)

    /** Render renderable. */
    virtual void render(FullRenderable& r, RenderEnvironment& material, RenderEnvironment& env) = 0;

    /** Render renderable, override program and material.*/
    virtual void render(FullRenderable& r, ProgramHandle& program, RenderEnvironment& material, RenderEnvironment& env) = 0;

    /** Allocate texture unit interface.*/
    virtual Texture* create_texture() = 0;

    virtual void remove_from_gpu(Texture* t) = 0;

    virtual DefaultMesh*    create_mesh() = 0;
    virtual FullRenderable* create_renderable() = 0;

    virtual void release_mesh(DefaultMesh*) = 0;
    virtual void release_renderable(FullRenderable*) = 0;

    // TODO free mesh, free renderable


    /** Mark asset dirty. */
    // TODO: Need this? virtual void image8_data_modified(int texture_unit, Image8& image);

);

GraphicsManager* make_graphics_manager();


///////////// RenderPassSettings ///////////////

/** Settings for an individual render pass. */
class RenderPassSettings
{
public:
    struct DepthMask{
        DepthMask():flag_(GL_FALSE){}
        DepthMask(GLboolean flag):flag_(flag){}
        GLboolean flag_;
        void apply() const {glDepthMask(flag_);}
    };

    struct ColorMask{
        GLboolean r_;GLboolean g_; GLboolean b_; GLboolean a_;
        ColorMask(GLboolean r, GLboolean g, GLboolean b, GLboolean a):
            r_(r),g_(g),b_(b),a_(a){}
        ColorMask():
            r_(GL_FALSE),g_(GL_FALSE),b_(GL_FALSE),a_(GL_FALSE){}
        void apply() const {glColorMask(r_,g_,b_,a_);}
    };

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

    enum Buffer{ BufferColor = GL_COLOR_BUFFER_BIT, BufferDepth = GL_DEPTH_BUFFER_BIT, BufferStencil = GL_STENCIL_BUFFER_BIT };


    Color clear_color; bool clear_color_set;
    GLclampd clear_depth; bool clear_depth_set;
    BlendSettings blend;  bool blend_set;
    DepthMask depth_mask; bool depth_mask_set;
    ColorMask color_mask; bool color_mask_set;
    GLuint   clear_mask;

    RenderPassSettings(const GLuint clear_mask, const Color& clear_color, const GLclampd clear_depth);
    RenderPassSettings(BlendSettings& blend_settings);
    RenderPassSettings(DepthMask& depth_mask_param);
    RenderPassSettings(ColorMask& color_mask_param);
    RenderPassSettings();

    void set_buffer_clear(Buffer buffer);

    static RenderPassSettings empty();
};

void apply(const RenderPassSettings& pass);

} // namespace glh
