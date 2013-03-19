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

////////////////////// Image8 ////////////////////////

namespace {
    void      free_uint8tarray(uint8_t* ptr){if(ptr) delete [] ptr;}
    uint8_t*  alloc_uint8t_array(int size){return new uint8_t[size];}
    void      stb_free_data(uint8_t* ptr){stbi_image_free(ptr);}
} // End anonymous namespace

Image8::Image8():width_(0), height_(0), channels_(0), data_(0), stride_(0), dealloc_(free_uint8tarray){}

Image8::Image8(const int w, const int h, const int chan):
    width_(w), height_(h), channels_(chan),  stride_(width_ * channels_), dealloc_(free_uint8tarray)
{
    int datasize = size();
    data_ = alloc_uint8t_array(datasize);
}

/** Takes ownership of data pointer passed to it. */
Image8::Image8(const int w, const int h, const int chan, uint8_t* data, Deallocator dealloc):
     width_(w), height_(h), channels_(chan), data_(data), stride_(width_ * channels_), dealloc_(dealloc){}

Image8::Image8(Image8&& rhs):width_(rhs.width_), height_(rhs.height_), 
    channels_(rhs.channels_), data_(rhs.data_), stride_(rhs.stride_), dealloc_(rhs.dealloc_){
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
        stride_   = rhs.stride_;
        dealloc_  = rhs.dealloc_;

        rhs.data_ = 0;
    }
    return *this;
}

void Image8::set(uint8_t* data, int channels, uint8_t* sample){
    switch(channels){
        case 1: data[0] = sample[0];return;
        case 2: data[0] = sample[0]; data[1] = sample[1]; return;
        case 3: data[0] = sample[0]; data[1] = sample[1]; data[2] = sample[2]; return;
        case 4: data[0] = sample[0]; data[1] = sample[1]; data[2] = sample[2];  data[3] = sample[3];return;
        default: return;
    }
}

////////////////////// Image32 ///////////////////////////

namespace {
    void      free_floatarray(float* ptr){if(ptr) delete [] ptr;}
    float*  alloc_float_array(int size){return new float[size];}
} // End anonymous namespace

Image32::Image32():width_(0), height_(0), channels_(0), data_(0), stride_(0),dealloc_(free_floatarray){}

Image32::Image32(const int w, const int h, const int chan):
    width_(w), height_(h), channels_(chan), stride_(width_ * height_ ),dealloc_(free_floatarray)
{
    int datasize = size();
    data_ = alloc_float_array(datasize);
}

/** Takes ownership of data pointer passed to it. */
Image32::Image32(const int w, const int h, const int chan, float* data, Deallocator dealloc):
     width_(w), height_(h), channels_(chan), data_(data), stride_(width_ * height_ ), dealloc_(dealloc){}

Image32::Image32(Image32&& rhs):width_(rhs.width_), height_(rhs.height_), 
    channels_(rhs.channels_), data_(rhs.data_), stride_(rhs.stride_),dealloc_(rhs.dealloc_){
        rhs.data_ = 0;
}

Image32::~Image32(){if(data_) dealloc_(data_);}

Image32& Image32::operator=(Image32&& rhs){
    if(&rhs != this)
    {

        if(data_) dealloc_(data_);

        width_    = rhs.width_;
        height_   = rhs.height_;
        channels_ = rhs.channels_;
        data_     = rhs.data_;
        dealloc_  = rhs.dealloc_;
        stride_   = rhs.stride_;

        rhs.data_ = 0;
    }
    return *this;
}


////////////////////// Utilities //////////////////////////

Image8 convert(const Image32& img)
{
    Image8 out(img.width_, img.height_, img.channels_);
    const int width = img.width_;
    const int chan = img.channels_;
    const int stride = img.stride_;

    std::vector<float> err(chan, 0.f);

    for(int i = 0; i < img.height_; ++i){
        // Dither by just moving the error to next pixel.
        // Avoid artifacts by moving error forwards and backwards from a random initial pixel on line.

        const float* imgrow = img.cat(0, i);
        const float* end    = imgrow + stride;
        uint8_t*     outrow = out.at(0, i);

        int splitpos = (minrand(i) % width) * chan;

        uint8_t res = 0;

        uint8_t* o = outrow + splitpos - 1;

        for(const float* p = imgrow + splitpos - 1; p >= imgrow;){
            for(int c = 0; c < chan; c++){
                std::tie(res, err[c]) = float_to_ubyte_err(*(p--) + err[c]);
                *o-- = res;
            }
        }

        o = outrow + splitpos;
        for(const float* p = imgrow + splitpos; p < end;){
            for(int c = 0; c < chan; c++){
                std::tie(res, err[c]) = float_to_ubyte_err(*(p++) + err[c]);
                *o++ = res;
            }
        }
    }

    return out;
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
