/**\file gltexture.h 
    \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/
#pragma once

#include "glh_image.h"
#include "glsystem.h"

namespace glh{


class Sampler{
public:


};

class TextureType {
public:
    enum Pixel{UnsignedByte, Float};

    enum InternalFormat{RGB8, RGBA8}; // TODO: add rest as needed (srgb, rgb32f etc.)

    enum Channels{RGB, RGBA};

    enum Target{Texture2D,
                // Cube map targets
                PosX, NegX,
                PosY, NegY, 
                PosZ, NegZ};

    GLenum gl_pixeltype() const;
    GLint  gl_internal_format() const;
    GLenum gl_format() const;
    GLenum gl_target() const;

    Pixel          pixel;
    InternalFormat format;
    Channels       channels;
    Target         target;
};

/** On-device buffers: textures, rendertargets etc. */
class Texture {
public:

    TextureType type;
    GLsizei width;
    GLsizei height;

    GLuint handle_;
    bool   on_gpu_;
    int    texture_unit_;

    Texture();

    void apply_settings();

    void get_params_from_image(const Image8& image);

    // TODO: Init from shader program. On assignment verify image matches
    // with preconfigured values.
    // TODO: Parametrize sampler settings.
    void assign(const Image8& image, int texture_unit);

};



}
