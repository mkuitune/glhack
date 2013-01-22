/** \file glhack.cpp OpenGL platform agnostic rendering and other utilities. 
    \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)*/

#include "glhack.h"
#include "shims_and_types.h"

#include <list>
#include <sstream>
#include <iostream>
#include <regex>
#include <memory>

bool          glh_logging_active(){return true;}
std::ostream* glh_get_log_ptr(){return &std::cout;}


namespace glh
{

///////////// OpenGL State management ////////////

bool check_gl_error(const char* msg)
{
    bool result = true;
    GLenum gl_error = glGetError();
    if (gl_error != GL_NO_ERROR)
    {
        result = false;
        GLH_LOG_EXPR("Gl error" << gluErrorString(gl_error) << " at:" << msg);
    }
    return result;
}


///////////// Shaders ///////////////


typedef BiMap<ShaderVar::Type, std::string> ShaderTypeTokens;
typedef BiMap<ShaderVar::Mapping, std::string> ShaderMappingTokens;

ShaderTypeTokens shader_type_tokens()
{
    ShaderTypeTokens map;
    add(map, ShaderVar::Vec3, "vec3")
            (ShaderVar::Vec4, "vec4")
            (ShaderVar::Mat4, "mat4");

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

const ShaderMappingTokens g_shader_mappings = shader_mapping_tokens();
const ShaderTypeTokens g_shader_types = shader_type_tokens();

std::ostream& operator<<(std::ostream& os, const ShaderVar& v)
{
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
    std::list<TextLine> lines = string_split(shader.c_str(), "\n");

    for(auto line = lines.begin(); line != lines.end(); line++)
    {
        std::cmatch match;
        if(std::regex_search(line->begin(), line->end(), match, var_regex))
        {
            auto        mapping = g_shader_mappings.inverse_map().find(match[1]);
            auto        type    = g_shader_types.inverse_map().find(match[2]);
            std::string name    = match[3];
            add(vars, ShaderVar(mapping->second, type->second, name));
        }
    }

    return vars;
}

bool release_program(ShaderProgram& program);
/** At this point an unholy mess containing all data relevant to a shader program management. '
    The lifetime of the wrapped program object is tied to the lifetime of this object: 
    do not allocate from stack (usually).
*/
class ShaderProgram 
{
public:
    typedef std::list<ShaderVar>                  varlist;
    typedef std::map<ShaderVar::Mapping, varlist> varmap;

    std::string name;

    varmap vars;

    // TODO: ShaderVar -> program handle map?

    std::string geometry_shader;
    std::string vertex_shader;
    std::string fragment_shader;

    GLuint program_handle;
    GLuint fragment_handle;
    GLuint vertex_handle;
    GLuint geometry_handle;

    ShaderProgram(cstring& program_name):name(program_name), program_handle(0),
        fragment_handle(0), vertex_handle(0), geometry_handle(0)
    {}

    ~ShaderProgram(){release_program(*this);}

    void reset_vars()
    {
        ShaderVar::for_Mapping([this](ShaderVar::Mapping m){this->vars[m] = varlist();});
    }
};

const char* program_name(ProgramHandle p)
{
    if(p) return p->name.c_str();
    else  return 0;
}

bool valid(ProgramHandle p)
{
    return p != 0;
}

namespace
{
    ///////////////// Shader program utilities //////////////////////
    enum Compilable {Shader = 0, Program = 1};

    void assign_varlist_locations(ShaderProgram::varlist& vars)
    {
        int i = 0;
        for(auto v = vars.begin(); v != vars.end(); v++) v->program_location = i++;
    }

    /** Parse variables from shaders to lists. */
    void init_var_lists(ShaderProgram& program)
    {
        auto var_list = join(parse_shader_vars(program.geometry_shader),
                             parse_shader_vars(program.vertex_shader),
                             parse_shader_vars(program.fragment_shader));

        program.reset_vars();

        for(auto v = var_list.begin(); v != var_list.end(); v++) program.vars[v->mapping].push_back(*v);

        // Init variable locations based on order of listing
        ShaderVar::for_Mapping([&program](ShaderVar::Mapping m){
            assign_varlist_locations(program.vars[m]);
        });
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
        const ShaderProgram::varlist& vars(program.vars[ShaderVar::StreamIn]);
        for(auto v = vars.begin(); v != vars.end(); ++v)
            glBindAttribLocation(program.program_handle, v->program_location, v->name.c_str());
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
                    log_program_error(program.name, program.program_handle);
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

    result = check_gl_error();

    if(!result)
    {
        GLH_LOG_EXPR("Shader program finalization failed");
    }

    program.program_handle = 0;

    return result;
}


ShaderProgram* create_shader_program(cstring& name, cstring& geometry_shader, cstring& vertex_shader, cstring& fragment_shader)
{
    ShaderProgram* program = new ShaderProgram(name);
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

    return program;
}


//////////// Graphics context /////////////

class GraphicsManagerInt : public GraphicsManager
{
public:

    typedef std::shared_ptr<ShaderProgram> ShaderProgramPtr;


    std::map<std::string, ShaderProgramPtr> programs_;


    ~GraphicsManagerInt()
    {
    }

    virtual ProgramHandle create_program(cstring& name, cstring& geometry, cstring& vertex, cstring& fragment) override
    {

        ShaderProgram* p = create_shader_program(name, geometry, vertex, fragment);

        if(p) programs_[name] = ShaderProgramPtr(p);

        ProgramHandle h(p);

        return h;
    }

     virtual ProgramHandle program(cstring& name) override
     {
         auto pi = programs_.find(name);
         ShaderProgram* p = 0;
         if(pi != programs_.end()) p = pi->second.get();
         return ProgramHandle(p);
     }

     virtual void use_program(ProgramHandle h) override
     {

     }

};


GraphicsManager* make_graphics_manager()
{
    GraphicsManagerInt* manager(new GraphicsManagerInt());
    return manager;
}


///////////// RenderPassSettings ///////////////

RenderPassSettings::RenderPassSettings(const GLuint mask, const vec4& color, const GLclampd depth)
:clear_mask(mask), clear_color(color), clear_depth(depth)
{}

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
    glClearColor(cc[0], cc[1], cc[2], cc[3]);
    glClearDepth(pass.clear_depth);
    if(pass.clear_mask)
    {
        glClear(pass.clear_mask);
    }
}

///////////// OpenGL Utilities /////////////

bool check_gl_error()
{
    bool result = true;
    GLenum ErrorCheckValue = glGetError();
    if (ErrorCheckValue != GL_NO_ERROR)
    {
        result = false;
        GLH_LOG_EXPR("GL error:" << gluErrorString(ErrorCheckValue));
    }
    return result;
}

} // namespace glh
