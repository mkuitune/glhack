/**\file gltexture.cpp
    \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/

#include <cassert>
#include "gltexture.h"

namespace glh{

///////////////////// TextureType ///////////////////////////

GLenum TextureType::gl_pixeltype() const {
    if(pixel == UnsignedByte) return GL_UNSIGNED_BYTE;
    else if(pixel == Float)   return GL_FLOAT;
    assert(!"Type not found");
    return 0;
}

GLint TextureType::gl_internal_format() const {
    if(format == R8)         return GL_R8;
    else if(format == RGB8)  return GL_RGB8;
    else if(format == RGBA8) return GL_RGBA8;
    assert(!"Type not found");
    return 0;
}

GLenum TextureType::gl_format() const {
    if(channels == R)         return GL_RED;
    else if(channels == RGB)  return GL_RGB;
    else if(channels == RGBA) return GL_RGBA;
    assert(!"Type not found");
    return 0;
}

GLenum TextureType::gl_target() const {
    if(target == Texture2D) return GL_TEXTURE_2D;
    else if(target == PosX) return GL_TEXTURE_CUBE_MAP_POSITIVE_X;
    else if(target == NegX) return GL_TEXTURE_CUBE_MAP_NEGATIVE_X;
    else if(target == PosY) return GL_TEXTURE_CUBE_MAP_POSITIVE_Y;
    else if(target == NegY) return GL_TEXTURE_CUBE_MAP_NEGATIVE_Y;
    else if(target == PosZ) return GL_TEXTURE_CUBE_MAP_POSITIVE_Z;
    else if(target == NegZ) return GL_TEXTURE_CUBE_MAP_NEGATIVE_Z;
    assert(!"Type not found");
    return 0;
}


///////////////////// Texture ///////////////////////////

Texture::Texture():on_gpu_(false), bound_texture_object_(0),image_(0), dirty_(true){
}

void Texture::get_params_from_image(const Image8& image)
{
    type.pixel = TextureType::UnsignedByte;

    if(image.channels_ == 1){
        type.format = TextureType::R8;
        type.channels = TextureType::R;
    }
    else if(image.channels_ == 3){
        type.format = TextureType::RGB8;
        type.channels = TextureType::RGB;
    }
    else if(image.channels_ == 4){
        type.format = TextureType::RGBA8;
        type.channels = TextureType::RGBA;
    }
    else assert(!"Unsupported number of channels");

    // TODO: cube textures might need to touch this logic.
    type.target = TextureType::Texture2D;

    width = image.width_;
    height = image.height_;

}

void Texture::attach_image(const Image8& image){
    image_ = &image;
    get_params_from_image(image);
    dirty_ = true;
}

void Texture::bind(){
    if(on_gpu_){
        GLenum textarget = type.gl_target();
        glBindTexture(textarget, bound_texture_object_);
    } else {
        throw GraphicsException("Texture::bind: No texture object attached.");
    }
}

void Texture::upload_image_data(GLuint texture_object){
    GLenum textarget          = type.gl_target();
    GLint  texinternal_format = type.gl_internal_format();
    GLenum texformat          = type.gl_format();
    GLenum textype            = type.gl_pixeltype();

    gpu_status(true, texture_object);

    bind();

    glTexImage2D(textarget, 0, texinternal_format, width, height, 0, texformat, textype, image_->data_);

    dirty_ = false;
}

void Texture::upload_sampler_parameters(){
    GLenum textarget = type.gl_target();

    glTexParameterf(textarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(textarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}

}
