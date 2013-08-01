/** \file glhack.cpp OpenGL platform agnostic rendering and other utilities. 
    \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)*/

#include "glhack.h"
#include "shims_and_types.h"
#include "glbase.h"

#include <stack>
#include <list>
#include <sstream>
#include <iostream>
#include <regex>
#include <memory>
#include <algorithm>

namespace glh
{
/////////////// Forward declaration ///////////////


ShaderVarList parse_shader_vars(cstring& shader);
class GraphicsManagerInt;

/** Synchronize texture data to GPU. Return handle to texture object to which the texture is bound to. If
    texture cannot be activated will throw a GraphicsException.
    @param texture_unit Texture unit to which the texture will be bound to.
    @param t            Texture to use.
*/
void graphics_manager_use_texture(GraphicsManagerInt& manager, int texture_unit, Texture& t);

///////////// Shaders ///////////////

const ShaderMappingTokens g_shader_mappings = shader_mapping_tokens();
const ShaderTypeTokens g_shader_types = shader_type_tokens();

std::ostream& operator<<(std::ostream& os, const ShaderVar& v){
    cstring& mapstr(g_shader_mappings.map().find(v.mapping)->second);
    cstring& typestr(g_shader_types.map().find(v.type)->second);
    // TODO: shorter key/value accessor for BiMap

    os << mapstr << " " << typestr << " " << v.name;
    return os;
}

template<class T>
void token_strings_to_regex(std::ostream& os, const T& tokens)
{
    os << "(";
    auto last = tokens.end();
    last--;
    for(auto i = tokens.begin(); i != last; ++i) os << i->second << "|";
    os << last->second << ")";
}

std::regex get_shader_variable_regex(const ShaderMappingTokens& mappings,
                                     const ShaderTypeTokens& types)
{
    std::ostringstream sstr;
    token_strings_to_regex(sstr, mappings); // First group, mappings
    sstr << "\\s+";
    token_strings_to_regex(sstr, types);    // Second group, types
    sstr << "\\s+(\\w+)";                   // Third group, name

    return std::regex(sstr.str());
}

std::list<ShaderVar> parse_shader_vars(cstring& shader)
{
    std::list<ShaderVar> vars;
    std::regex var_regex      = get_shader_variable_regex(g_shader_mappings, g_shader_types);
    std::list<TextLine> lines = string_split(shader.c_str(), "\n;");

    for(auto& line : lines)
    {
        std::cmatch match;
        if(std::regex_search(line.begin(), line.end(), match, var_regex))
        {
            auto        mapping = g_shader_mappings.inverse_map().find(match[1]);
            auto        type    = g_shader_types.inverse_map().find(match[2]);
            std::string name    = match[3];
            add(vars, ShaderVar(mapping->second, type->second, name));
        }
    }

    return vars;
}

class ShaderProgram;

bool release_program(ShaderProgram& program);

bool verify_matching_types(ShaderVar::Type stype, TypeId::t t, int32_t components){
    bool result = false;
         if(stype == ShaderVar::Scalar && t == TypeId::Float32 && components == 1) result = true;
    else if(stype == ShaderVar::Vec2 && t == TypeId::Float32 && components == 2)   result = true;
    else if(stype == ShaderVar::Vec3 && t == TypeId::Float32 && components == 3)   result = true;
    else if(stype == ShaderVar::Vec4 && t == TypeId::Float32 && components == 4)   result = true;
    return result;
}

GLenum buffertype_to_gltype(TypeId::t t){
    switch(t)
    {
        case TypeId::Float32: return GL_FLOAT;
        default:                  return 0;
    }
}

/** At this point an unholy mess containing all data relevant to a shader program management. '
    The lifetime of the wrapped program object is tied to the lifetime of this object: 
    do not allocate from stack (usually).
*/
class ShaderProgram : public ProgramHandle{
public:
    GraphicsManagerInt* manager_;

    std::string name_;

    ShaderVarList vertex_input_vars;
    ShaderVarList uniform_vars;

    std::string geometry_shader;
    std::string vertex_shader;
    std::string fragment_shader;

    GLuint program_handle;
    GLuint fragment_handle;
    GLuint vertex_handle;
    GLuint geometry_handle;

    ShaderProgram(GraphicsManagerInt* manager, cstring& program_name):manager_(manager), name_(program_name), program_handle(0),
        fragment_handle(0), vertex_handle(0), geometry_handle(0)
    {}

    ~ShaderProgram(){release_program(*this);}

    void reset_vars(){
        vertex_input_vars  = ShaderVarList();
        uniform_vars       = ShaderVarList();
    }

    virtual const char* name() override {return name_.c_str();}

    virtual const ShaderVarList& inputs() override {return vertex_input_vars;}
    virtual const ShaderVarList& uniforms() override {return uniform_vars;}

    void use() {glUseProgram(program_handle);}

    void bind_vertex_input(NamedBufferHandles& buffers)
    {
        for(auto& bi : buffers)
        {
            auto ivar = std::find_if(vertex_input_vars.begin(), vertex_input_vars.end(),
                                  [&bi](const ShaderVar& v)->bool{return (v.name == bi.first);});

            if(ivar == vertex_input_vars.end()) continue;

            BufferHandle& bufferhandle(*bi.second);

            int32_t       components = bufferhandle.components();
            TypeId::t     btype      = bufferhandle.mapped_sig_.type_;
            auto          gltype     = buffertype_to_gltype(btype);
            GLsizei       stride     = 0;
            const GLvoid* offset     = 0;

            assert(verify_matching_types(ivar->type, btype, components));

            glEnableVertexAttribArray(ivar->program_location);
            bufferhandle.bind(GL_ARRAY_BUFFER);
            glVertexAttribPointer(ivar->program_location, components, gltype, GL_FALSE, stride, offset);
        }
    }

    bool has_uniform(const std::string& name, const ShaderVar::Type t){
         for(auto &u: uniform_vars) if(t == u.type && u.name == name) return true;
         return false;
    }

    ShaderVar* get_uniform(const std::string& name, const ShaderVar::Type t){
        ShaderVar* result = 0;
        for(auto &u: uniform_vars){
            if(t == u.type && u.name == name){
                result = &u;
                break;
            }
        }
        return result;
    }

    void allocate_texture_units(){
        int first_index = GL_TEXTURE0 + 0;
        std::list<ShaderVar*> rvars;
        for(auto &u:uniform_vars) if(u.type == ShaderVar::Sampler2D) rvars.push_back(&u);
        rvars.sort([](ShaderVar* a, ShaderVar* b){return elements_are_ordered(a->name, b->name);});
        for(auto &u:rvars) u->texture_unit_ = first_index++;
    }

    void assign_uniform_locations()
    {
        for(auto& u: uniform_vars){
            u.program_location = glGetUniformLocation(program_handle, u.name.c_str());
        }
    }


#if 0
    void bind_uniforms(VarMap& vmap)
    {
        for(auto &u: uniform_vars)
        {
            auto var = find_var(vmap, u.name, u.type);
            if(var != vmap.end()) assign(program_handle, u.name.c_str(), var->second);
        }
    }

    bool has_uniform(const std::string& name, const Var_t& var){
         for(auto &u: uniform_vars) if(var.type_ == u.type && u.name == name) return true;
         return false;
    }

    void bind_uniform(const std::string& name, const Var_t& var){
        for(auto &u: uniform_vars){
            if(var.type_ == u.type && u.name == name) assign(program_handle, u.name.c_str(), var);
        }
    }
#endif
};

typedef std::shared_ptr<ShaderProgram> ShaderProgramPtr;

ShaderProgram* shader_program(ProgramHandle* handle){return static_cast<ShaderProgram*>(handle);}
const ShaderProgram* shader_program(const ProgramHandle* handle){return static_cast<const ShaderProgram*>(handle);}


GLuint program_handle(const ProgramHandle* handle_)
{
    const ShaderProgram* sp = shader_program(handle_);
    return sp->program_handle;
}

/** Add handle per each input buffer found in program.*/
void init_bufferset_from_program(BufferSet& bufs, ProgramHandle* h)
{
    const ShaderVarList& inputs = h->inputs();
    for(auto& i: inputs){
        glh::TypeId::t type;
        int32_t      dim;
        std::tie(type, dim) = ShaderVar::typeid_and_dim(i.type);
        bufs.create_handle(i.name, glh::BufferSignature(type, dim)); 
    }
}

/** Map chunk of type ChannelType::<name> to input known by 'name' if
 *  First three digits of <name> match with any position in 'name'.*/
// TODO: create a binder object that takes references for these entities and handles the binding
// without extensive name lookup
void assign_by_guessing_names(BufferSet& bufs, DefaultMesh& mesh)
{
   for(auto&b : bufs.buffers_){
            if(contains(b.first,"pos")  || contains(b.first,"Pos"))  bufs.assign(b.first, mesh.get(ChannelType::Position));
       else if(contains(b.first,"norm") || contains(b.first,"Norm")) bufs.assign(b.first, mesh.get(ChannelType::Normal));
       else if(contains(b.first,"tex")  || contains(b.first,"Tex"))  bufs.assign(b.first, mesh.get(ChannelType::Texture));
       else if(contains(b.first,"col")  || contains(b.first,"Col"))  bufs.assign(b.first, mesh.get(ChannelType::Color));
   }
}


//////////////////// ActiveProgram ///////////////////
namespace {

// TODO: Use stored location ShaderVar::program_location.

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

void assign_sampler_uniform(const GLuint program, const char* name, int texture_unit){
    GLint location = glGetUniformLocation(program, name);
    if(location != -1) glUniform1i(location, texture_unit);
    //else               assert("Applying to non-existing location");
}
}

const int g_component_max_count = INT32_MAX;


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
    void bind_uniform(const std::string& name, Texture& tex); // TODO: Defer texture upload. Upload only at draw() command.

    ProgramHandle* handle_;
private:
    int32_t        component_count_;
};


ActiveProgram::ActiveProgram():component_count_(g_component_max_count){}

void ActiveProgram::bind_vertex_input(NamedBufferHandles& buffers){
    ShaderProgram* sp = shader_program(handle_);
    sp->bind_vertex_input(buffers);

    for(auto &b: buffers) component_count_ = std::min(component_count_, b.second->component_count());
}

void ActiveProgram::bind_uniform(const std::string& name, const mat4& mat){
    ShaderProgram* sp = shader_program(handle_);
    if(sp->has_uniform(name, ShaderVar::Mat4)) assign(sp->program_handle, name.c_str(), mat);
}

void ActiveProgram::bind_uniform(const std::string& name, const vec4& vec){
    ShaderProgram* sp = shader_program(handle_);
    if(sp->has_uniform(name, ShaderVar::Vec4)) assign(sp->program_handle, name.c_str(), vec);
}

void ActiveProgram::bind_uniform(const std::string& name, const vec3& vec){
    ShaderProgram* sp = shader_program(handle_);
    if(sp->has_uniform(name, ShaderVar::Vec3)) assign(sp->program_handle, name.c_str(), vec);
}

void ActiveProgram::bind_uniform(const std::string& name, Texture& tex){
    ShaderProgram* sp = shader_program(handle_);
    ShaderVar* uniform_var = sp->get_uniform(name, ShaderVar::Sampler2D);

    if(uniform_var){
        int texture_unit = uniform_var->texture_unit_;

        GraphicsManagerInt& manager(*sp->manager_);
        graphics_manager_use_texture(manager, texture_unit, tex);
        assign_sampler_uniform(sp->program_handle, name.c_str(), texture_unit);
    }
}

void ActiveProgram::draw(){
    if(component_count_ != g_component_max_count) glDrawArrays(GL_TRIANGLES, 0, component_count_);
}


namespace
{
    ///////////////// Shader program utilities //////////////////////
    enum Compilable {Shader = 0, Program = 1};

    /** Parse variables from shaders to lists. */
    void init_var_lists(ShaderProgram& program)
    {
        ShaderVarList geometry_list = parse_shader_vars(program.geometry_shader);
        ShaderVarList vertex_list   = parse_shader_vars(program.vertex_shader);
        ShaderVarList fragment_list = parse_shader_vars(program.fragment_shader);

        auto to_uniforms = [&](ShaderVarList& cont){
            for(auto &v : cont) if(v.mapping == ShaderVar::Uniform) program.uniform_vars.push_back(v);
        };

        program.reset_vars();

        int input_index = 0;
        for(auto& v : vertex_list){
            if(v.mapping == ShaderVar::StreamIn){
                v.program_location = input_index++;
                program.vertex_input_vars.push_back(v);
            }
        }

        to_uniforms(geometry_list );
        to_uniforms(vertex_list );
        to_uniforms(fragment_list );
    }

    void log_shader_program_error(cstring& name, GLuint handle, Compilable comp)
    {
        decltype(glGetShaderiv)      lenfuns[] = {glGetShaderiv, glGetProgramiv};
        decltype(glGetShaderInfoLog) logfuns[] = {glGetShaderInfoLog, glGetProgramInfoLog};
        const char* domain[] = {"Shader", "Program"};

        GLH_LOG_EXPR(domain[comp] << " compilation failed:" << name);

        GLint log_length;

        lenfuns[comp](handle, GL_INFO_LOG_LENGTH, &log_length);

        if(log_length > 0)
        {
            AlignedArray<char> log(log_length, '\0');
            GLsizei written;
            logfuns[comp](handle, log_length, &written, log.data());
            GLH_LOG_EXPR(domain[comp] << " log:\n" << log.data());
        }
    }

    void log_program_error(cstring& name, GLuint handle){log_shader_program_error(name, handle, Program);}
    void log_shader_error(cstring& name, GLuint handle){log_shader_program_error(name, handle, Shader);}

    bool compile_shaders(ShaderProgram& program)
    {
        auto compile_shader = [] (GLenum shader_type, GLuint& handle, const char* name, cstring& source)->bool
        {
            bool result = true;

            if(source.length() > 0)
            {
                handle = glCreateShader(shader_type);
                const char* src[] = {source.c_str()};
                glShaderSource(handle, 1, src, NULL);
                glCompileShader(handle);

                GLint result;
                glGetShaderiv(handle, GL_COMPILE_STATUS, &result);

                if(result == GL_FALSE)
                {
                    log_shader_error(name, handle);
                }
            }

            return result;
        };

        bool result = false;

        bool geometry_result = compile_shader(GL_GEOMETRY_SHADER, program.geometry_handle, "geometry shader",program.geometry_shader);
        bool vertex_result   = compile_shader(GL_VERTEX_SHADER, program.vertex_handle, "vertex shader",program.vertex_shader);
        bool fragment_result = compile_shader(GL_FRAGMENT_SHADER, program.fragment_handle, "fragment shader",program.fragment_shader);

        if(geometry_result && vertex_result && fragment_result)
        {
            if(program.geometry_handle) glAttachShader(program.program_handle, program.geometry_handle);
            if(program.vertex_handle)   glAttachShader(program.program_handle, program.vertex_handle);
            if(program.fragment_handle) glAttachShader(program.program_handle, program.fragment_handle);

            result = true;
        }

        return result;
    }

    void bind_program_input_locations(ShaderProgram& program)
    {
        const ShaderVarList& vars(program.vertex_input_vars);

        for(auto &v : vars)
        {
            glBindAttribLocation(program.program_handle, v.program_location, v.name.c_str());
        }
    }

    bool compile_program(ShaderProgram& program)
    {
        bool result = false;

        program.program_handle = glCreateProgram();

        if(program.program_handle != 0)
        {
            bool shader_result = compile_shaders(program);

            if(shader_result)
            {
                init_var_lists(program);

                bind_program_input_locations(program);

                glLinkProgram(program.program_handle);

                // Verify link status
                GLint status;
                glGetProgramiv(program.program_handle, GL_LINK_STATUS, &status);

                if(status != GL_FALSE)
                {
                    glUseProgram(program.program_handle);
                    result = true;
                }
                else
                {
                    log_program_error(program.name(), program.program_handle);
                }
            }
        }

        return result;
    }
}// end anonymoys namespace

bool release_program(ShaderProgram& program)
{
    auto finalize_shader = [](GLuint program_handle,  GLuint shader_handle)
    {
        if(shader_handle != 0)
        {
            if(program_handle != 0) glDetachShader(program_handle, shader_handle);
            glDeleteShader(shader_handle);
        }
    };

    bool result = false;

    glUseProgram(0);

    finalize_shader(program.program_handle, program.vertex_handle);
    finalize_shader(program.program_handle, program.geometry_handle);
    finalize_shader(program.program_handle, program.fragment_handle);

    if(program.program_handle != 0) glDeleteProgram(program.program_handle);

    result = true;
    //result = check_gl_error(); // TODO enable

    if(!result)
    {
        GLH_LOG_EXPR("Shader program finalization failed");
    }

    program.program_handle = 0;

    return result;
}

ShaderProgram* create_shader_program(GraphicsManagerInt* manager, cstring& name, cstring& geometry_shader, cstring& vertex_shader, cstring& fragment_shader)
{
    ShaderProgram* program = new ShaderProgram(manager, name);
    bool result = true;

    program->geometry_shader = geometry_shader;
    program->vertex_shader   = vertex_shader;
    program->fragment_shader = fragment_shader;

    /* Then compile the shader */
    result = compile_program(*program);

    // If something is wrong, delete program
    if(!result)
    {
        delete program;
        program = 0;
    }
    else
    {
        // Transfer params to uniforms
        program->allocate_texture_units();
        program->assign_uniform_locations();
    }

    return program;
}


//////////// Environment /////////////

float&   RenderEnvironment::get_scalar(cstring name){return scalar_[name];}
vec4&    RenderEnvironment::get_vec4(cstring name){return vec4_[name];}
mat4&    RenderEnvironment::get_mat4(cstring name){return mat4_[name];}
Texture& RenderEnvironment::get_texture2d(cstring name){
    return *texture2d_[name];
}

void program_params_from_env(ActiveProgram& program, RenderEnvironment& env){
    for(auto& input: program.handle_->uniforms()){
        if(env.has(input.name, input.type)) {
            if(input.type == ShaderVar::Vec4)
                program.bind_uniform(input.name, env.get_vec4(input.name));
            else if(input.type == ShaderVar::Mat4)
                program.bind_uniform(input.name, env.get_mat4(input.name));
            else if(input.type == ShaderVar::Sampler2D)
            {
                Texture& tex(env.get_texture2d(input.name));
                program.bind_uniform(input.name, tex);
            }
        }
    }
}


//////////// Graphics context /////////////

class GraphicsManagerInt : public GraphicsManager
{
public:

    std::map<std::string, ShaderProgramPtr> programs_;
    std::list<BufferHandlePtr>              bufferhandles_;
    std::list<TexturePtr>                   textures_;
    std::list<DefaultMeshPtr>               meshes_;
    std::list<FullRenderablePtr>            renderables_;


    ShaderProgram* current_program_;

//////// Internal: Texture unit stuff //////// 

    GLuint gen_texture_object(){
        GLuint handle;
        glGenTextures(1, &handle); // TODO move
        return handle;
    }

    void free_texture_object(GLuint handle){
        glDeleteTextures(1, &handle);
    }

    GraphicsManagerInt():current_program_(0){}
    ~GraphicsManagerInt(){}

    virtual ProgramHandle* create_program(cstring& name, cstring& geometry, cstring& vertex, cstring& fragment) override
    {
        ShaderProgram* p = create_shader_program(this, name, geometry, vertex, fragment);

        if(p) programs_[name] = ShaderProgramPtr(p);
        else throw GraphicsException("Program creation failed.");

        ProgramHandle* h = p;
        return h;
    }

     virtual ProgramHandle* program(cstring& name) override
     {
         auto pi = programs_.find(name);
         ProgramHandle* p = 0;
         if(pi != programs_.end()) p = pi->second.get();
         return p;
     }

     virtual Texture* create_texture() override
     {
         textures_.push_back(std::make_shared<glh::Texture>());
         return textures_.rbegin()->get();
     }

    ActiveProgram GraphicsManagerInt::make_active(ProgramHandle* handle){

        ActiveProgram a;

        ShaderProgram* sp = shader_program(handle);
        // TODO: cache current program, do not change program if current is in use.
        if(current_program_ != sp) {
            sp->use();
            current_program_ = sp;
        }

        a.handle_ = handle;
        return a;
    }

    //virtual void GraphicsManagerInt::render(FullRenderable& r, RenderEnvironment& env) override {

    //    if(!r.mesh()) throw GraphicsException("FullRenderable: trying to render without bound mesh.");

    //    if(!r.meshdata_on_gpu_) r.transfer_vertexdata_to_gpu(); // TODO MUSTFIX: Store on-gpu status in buffers
    //                                                            // So that FullRenderables may instantiate
    //                                                            // meshes in stead of transferring them to gpu.

    //    // TODO: Should here be transfer_texture_data_to_gpu

    //    auto active = make_active(r.program_);
    //    active.bind_vertex_input(r.device_buffers_.buffers_);

    //    program_params_from_env(active, env);
    //    program_params_from_env(active, r.material_);

    //    active.draw();
    //}

    virtual void GraphicsManagerInt::render(FullRenderable& r, RenderEnvironment& material, RenderEnvironment& env) override {
        render(r, *r.program_, material, env);
    }

    // TODO: Merge with render function above.
    virtual void render(FullRenderable& r, ProgramHandle& program, RenderEnvironment& material, RenderEnvironment& env) override {

        if(!r.mesh()) throw GraphicsException("FullRenderable: trying to render without bound mesh.");

        if(!r.meshdata_on_gpu_) r.transfer_vertexdata_to_gpu();
                                                               
        auto active = make_active(&program);
        active.bind_vertex_input(r.device_buffers_.buffers_);
        
        program_params_from_env(active, env);
        program_params_from_env(active, material);

        active.draw();
    }

     virtual void remove_from_gpu(Texture* t) override {
         bool on_gpu;
         GLuint handle;
         std::tie(on_gpu, handle) = t->gpu_status();
         // TODO: Add a state mapping consistency check here.
         if(on_gpu){
             free_texture_object(handle);
             t->gpu_status(false, 0);
         }
     }

    virtual DefaultMesh* create_mesh() override {
        meshes_.emplace_back(DefaultMeshPtr(new glh::DefaultMesh()));
        return meshes_.rbegin()->get();
    }

    virtual FullRenderable* create_renderable() override {
        renderables_.emplace_back(FullRenderablePtr(new glh::FullRenderable()));
        return renderables_.rbegin()->get();
    }

};

GraphicsManager* make_graphics_manager()
{
    GraphicsManagerInt* manager(new GraphicsManagerInt());
    return manager;
}

static void activate_texture_unit(int texture_unit){
    glActiveTexture(GL_TEXTURE0 + texture_unit);
}

static void upload_texture_image(GraphicsManagerInt& manager, Texture& t){

    GLuint texture_object;
    texture_object = manager.gen_texture_object();
    t.upload_image_data(texture_object);
}

void graphics_manager_use_texture(GraphicsManagerInt& manager, int texture_unit, Texture& t)
{
    if(t.image_ == 0) throw GraphicsException("graphics_manager_use_texture: Texture does not have image attached.");

    activate_texture_unit(texture_unit);

    bool on_gpu;
    GLuint dummy;

    std::tie(on_gpu, dummy) = t.gpu_status();

    if(!on_gpu){
        upload_texture_image(manager, t);
    }
    else if(t.dirty_) {
        manager.remove_from_gpu(&t);
        upload_texture_image(manager, t);
    }
    else {
        t.bind();
    }

    t.upload_sampler_parameters();
}

///////////// RenderPassSettings ///////////////

// TODO: writemask and glStencilMask, glDepthMask and glColorMask

RenderPassSettings::RenderPassSettings():
    clear_color_set(false), clear_depth_set(false), blend_set(false), clear_mask(0),
    depth_mask_set(false), color_mask_set(false) {
}

RenderPassSettings::RenderPassSettings(const GLuint clear_mask, const vec4& color, const GLclampd depth)
:clear_mask(clear_mask), clear_color(color), clear_depth(depth),
clear_color_set(true), clear_depth_set(true), blend_set(false), depth_mask_set(false), color_mask_set(false)
{}

RenderPassSettings::RenderPassSettings(BlendSettings& blend_settings){
    *this = RenderPassSettings::empty();
    blend = blend_settings;
    blend_set = true;
}

RenderPassSettings::RenderPassSettings(DepthMask& depth_mask_param){
    *this = RenderPassSettings::empty();
    depth_mask = depth_mask_param;
    depth_mask_set = true;
}

RenderPassSettings::RenderPassSettings(ColorMask& color_mask_param){
    *this = RenderPassSettings::empty();
    color_mask = color_mask_param;
    color_mask_set = true;
}

void RenderPassSettings::set_buffer_clear(Buffer buffer)
{
    switch(buffer)
    {
        case Color:   clear_mask |= GL_COLOR_BUFFER_BIT; break;
        case Depth:   clear_mask |= GL_DEPTH_BUFFER_BIT; break;
        case Stencil: clear_mask |= GL_STENCIL_BUFFER_BIT; break;
        default: assert(0);
    }
}

void apply(const RenderPassSettings& pass)
{
    const vec4& cc(pass.clear_color); 

    if(pass.clear_color_set) glClearColor(cc[0], cc[1], cc[2], cc[3]);
    if(pass.clear_depth_set) glClearDepth(pass.clear_depth);
    if(pass.clear_mask) glClear(pass.clear_mask);
    if(pass.blend_set) pass.blend.apply();
    if(pass.depth_mask_set) pass.depth_mask.apply();
    if(pass.color_mask_set) pass.color_mask.apply();
}

RenderPassSettings RenderPassSettings::empty()
{
    RenderPassSettings s;
    return s;
}

} // namespace glh
