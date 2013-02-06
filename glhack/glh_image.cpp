/**\file glh_image.cpp
    \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/

#include "glh_image.h"
#include "glsystem.h"
#include "stb_image.c"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include<vector>


namespace glh{

namespace {
    void      free_uint8tarray(uint8_t* ptr){if(ptr) delete [] ptr;}
    uint8_t*  alloc_uint8t_array(int size){return new uint8_t[size];}
    void      stb_free_data(uint8_t* ptr){stbi_image_free(ptr);}
} // End anonymous namespace

Image8::Image8():width_(0), height_(0), channels_(0), data_(0), dealloc_(free_uint8tarray){}

Image8::Image8(const int w, const int h, const int chan):
    width_(w), height_(h), channels_(chan), dealloc_(free_uint8tarray)
{
    int datasize = size();
    data_ = alloc_uint8t_array(datasize);
}

/** Takes ownership of data pointer passed to it. */
Image8::Image8(const int w, const int h, const int chan, uint8_t* data, Deallocator dealloc):
     width_(w), height_(h), channels_(chan), data_(data), dealloc_(dealloc){}

Image8::Image8(Image8&& rhs):width_(rhs.width_), height_(rhs.height_), 
    channels_(rhs.channels_), data_(rhs.data_), dealloc_(rhs.dealloc_){
        rhs.data_ = 0;
}

Image8::~Image8(){if(data_) dealloc_(data_);}

Image8& Image8::operator=(Image8&& rhs){
    if(&rhs != this)
    {

        if(data_) dealloc_(data_);

        width_    = rhs.width_;
        height_   = rhs.height_;
        channels_ = rhs.channels_;
        data_     = rhs.data_;
        dealloc_  = rhs.dealloc_;

        rhs.data_ = 0;
    }
    return *this;
}

Image8 load_image(const char* path)
{
    int width     = 0;
    int height    = 0;
    int channels  = 0;

    Image8::Deallocator dealloc = stb_free_data;

    uint8_t* data = stbi_load(path, &width, &height, &channels, 0);

    if(data == 0){
        std::string msg = "Could not open image:" + std::string(path);
        throw GraphicsException(msg);
    }

    return Image8(width, height, channels, data, dealloc);
}

bool write_image_png(const Image8& image, const char* path)
{
    int stride = image.stride();
    int success = stbi_write_png(path, image.width_, image.height_, image.channels_, image.data_, stride);
    return (success == 0);
}

void   flip_vertical(Image8& img)
{
    if(img.empty()) return;
    int stride = img.stride();
    std::vector<uint8_t> tmp_buffer(stride);
    uint8_t* high_ptr;
    uint8_t* low_ptr;
    uint8_t* tmp_ptr(&tmp_buffer[0]);

    for(int h = 0; h < img.height_/2; ++h)
    {
        high_ptr = img.data_ + (h * stride);
        low_ptr =   img.data_ + ((img.height_ - h - 1) * stride);
        memcpy(tmp_ptr, high_ptr, stride);
        memcpy(high_ptr, low_ptr, stride);
        memcpy(low_ptr, tmp_ptr, stride);
    }
}

}
