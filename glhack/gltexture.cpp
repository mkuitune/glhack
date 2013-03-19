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
    assert("Type not found");
    return 0;
}

GLint TextureType::gl_internal_format() const {
    if(format == RGB8)       return GL_RGB8;
    else if(format == RGBA8) return GL_RGBA8;
    assert("Type not found");
    return 0;
}

GLenum TextureType::gl_format() const {
    if(channels == RGB)       return GL_RGB;
    else if(channels == RGBA) return GL_RGBA;
    assert("Type not found");
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
    assert("Type not found");
    return 0;
}


///////////////////// Texture ///////////////////////////

Texture::Texture():on_gpu_(false){
    glGenTextures(1, &handle_);
}

void Texture::get_params_from_image(const Image8& image)
{
    type.pixel = TextureType::UnsignedByte;

    if(image.channels_ == 3){
        type.format = TextureType::RGB8;
        type.channels = TextureType::RGB;
    }
    else if(image.channels_ == 4){
        type.format = TextureType::RGBA8;
        type.channels = TextureType::RGBA;
    }
    else assert("Unsupported number of channels");

    width = image.width_;
    height = image.height_;

}

void Texture::apply_settings(){
    // TODO: store sampler settings in texture.

}

static void activate_texture_unit(int texture_unit){
    glActiveTexture(GL_TEXTURE0 + texture_unit);
}

// TODO: Parametrize sampler settings.
void Texture::assign(const Image8& image, int texture_unit) {
    get_params_from_image(image);
    type.target = TextureType::Texture2D;
    texture_unit_ = texture_unit;

    activate_texture_unit(texture_unit_);

    GLenum textarget          = type.gl_target();
    GLint  texinternal_format = type.gl_internal_format(); 
    GLenum texformat          = type.gl_format();
    GLenum textype            = type.gl_pixeltype();

    glBindTexture(textarget, handle_);
    glTexImage2D(textarget, 0, texinternal_format, width, height, 0, texformat, textype, image.data_);
    glTexParameterf(textarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(textarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    on_gpu_ = true;
}

}
