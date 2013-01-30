/** \file glsystem.cpp
 
 \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/
#pragma once

#include"glsystem.h"

#include<iostream>


bool          glh_logging_active(){return true;}
std::ostream* glh_get_log_ptr(){return &std::cout;}


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

bool check_gl_error(void)
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

