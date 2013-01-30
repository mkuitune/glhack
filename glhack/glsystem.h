/** \file glsystem.h OpenGL namespace import and lowlevel utilities.
 
 \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/
#pragma once

#ifdef WIN32
#define NOMINMAX
#include<windows.h>
#endif

#include<GL/glew.h>
#include <GL/glfw.h>

#include <string>
#include <cstdint>

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

class GraphicsException{
public:

    GraphicsException(const char* msg):msg_(msg){}
    ~GraphicsException(){}

    std::string get_message(){return msg_;}

    std::string msg_;
};

///////////// OpenGL State management ////////////

bool check_gl_error(const char* msg);

/** Check GL error. @return true if no error found. */
bool check_gl_error(void);

} // namespace glh
