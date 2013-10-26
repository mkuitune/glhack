/**\file gltexture.h 
    \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/
#pragma once

#include "glh_image.h"
#include "glsystem.h"

#include <memory>


namespace glh{

class Texture;

struct TextureUnitData{const uint8_t* bound_image; Texture* bound_texture; size_t stored_size;};

class Sampler{
public:


};

class TextureType {
public:
    enum Pixel{UnsignedByte, Float};

    enum InternalFormat{R8, RGB8, RGBA8}; // TODO: add rest as needed (srgb, rgb32f etc.)

    enum Channels{R, RGB, RGBA};

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

/** On-device buffers: textures, rendertargets etc. 
// TODO: need offload from GPU command at some point.
*/
class Texture {
private:
    GLuint handle_;

    bool   on_gpu_;
    GLuint bound_texture_object_;

public:

    TextureType type;
    GLsizei width;
    GLsizei height;

    bool   dirty_;

    const Image8* image_;


    Texture();

    void get_params_from_image(const Image8& image);

    void attach_image(const Image8& image);

    std::tuple<bool, GLuint> gpu_status() const {return std::make_tuple(on_gpu_, bound_texture_object_);}
    void gpu_status(bool ongpu, GLuint texture_object){on_gpu_ = ongpu; bound_texture_object_ = texture_object;}

    void bind();
    void upload_image_data(GLuint texture_object);

    void upload_sampler_parameters();
};

typedef std::shared_ptr<Texture> TexturePtr;


}
