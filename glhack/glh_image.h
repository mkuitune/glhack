/**\file glh_image.h 
    \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/
#pragma once


#include "glh_typedefs.h"
#include "math_tools.h"
#include <functional>
#include <tuple>

namespace glh{

/** class Image8. Container for 8-bit per pixel images.

    channels_ = 1 -> grey
    channels_ = 2 -> grey, alpha (interleaved)
    channels_ = 3 -> red, green, blue (interleaved)
    channels_ = 4 -> red, green, blue, alpha (interleaved)

    Remember to always throw exception instead of returning an
    invalid image object.
*/

class Image8{
public:
    typedef uint8_t pixel_t;
    typedef std::function<void(uint8_t*)> Deallocator;

    int      width_;
    int      height_;
    int      channels_;
    int      stride_;
    uint8_t* data_;

    Deallocator dealloc_;

    Image8();
    Image8(const int w, const int h, const int chan);

    /** Takes ownership of data pointer passed to it. Deallocator is used to finalize the
    *   data pointer passed in (free/delete[]/reduce reference count/whatever).*/
    Image8(const int w, const int h, const int chan, uint8_t* data, Deallocator dealloc);

    Image8(Image8&& rhs);

    ~Image8();

    Image8& operator=(Image8&& rhs);

    int size() const {return width_ * height_ * channels_;}

    /** Return size of horizontal row in bytes. */
    int stride() const {return width_ * channels_;}
    bool empty() const {return data_ == 0;}
    
    uint8_t*       data(){return data_;}
    const uint8_t* data() const {return data_;}
    uint8_t*       at(int x, int y){return data_ + y * stride_ + x * channels_;}
    const uint8_t* cat(int x, int y) const {return data_ + y * stride_ + x * channels_;}

    static void set(uint8_t* data, int channels, uint8_t* sample);
};


/** class Image32. Container for 32-bit per pixel images.

    channels_ = 1 -> grey
    channels_ = 2 -> grey, alpha (interleaved)
    channels_ = 3 -> red, green, blue (interleaved)
    channels_ = 4 -> red, green, blue, alpha (interleaved)

    Remember to always throw exception instead of returning an
    invalid image object.
*/
class Image32 {
public:
    typedef float pixel_t;
    typedef std::function<void(float*)> Deallocator;

    int      width_;
    int      height_;
    int      channels_;
    int      stride_;
    float*   data_;

    Deallocator dealloc_;

    Image32();
    Image32(const int w, const int h, const int chan);

    /** Takes ownership of data pointer passed to it. Deallocator is used to finalize the
    *   data pointer passed in (free/delete[]/reduce reference count/whatever).*/
    Image32(const int w, const int h, const int chan, float* data, Deallocator dealloc);

    Image32(Image32&& rhs);

    ~Image32();

    Image32& operator=(Image32&& rhs);

    int size() const {return width_ * height_ * channels_;}

    /** Return size of horizontal row in bytes. */
    int stride() const {return width_ * channels_;}
    bool empty() const {return data_ == 0;}

    float* at(int x, int y){return data_ + y * stride_ + x * channels_;}
    const float* cat(int x, int y) const {return data_ + y * stride_ + x * channels_;}
};


/* Utility functions */

template<class IMG> void for_pixels(IMG& img, std::function<void(typename IMG::pixel_t*, int x, int y)> fun)
{
    for(int i = 0; i < img.height_; ++i)
        for(int j = 0; j < img.width_; ++j) fun(img.at(j,i), j, i);
}

inline uint8_t float_to_ubyte(float in){
    return (in < 0.f) ? 0 : ((in > 1.0f) ? 255 : to_ubyte(255.f * in)); 
}

inline std::tuple<uint8_t, float> float_to_ubyte_err(float in){
    uint8_t sample = (in < 0.f) ? 0 : ((in > 1.0f) ? 255 : to_ubyte(255.f * in));
    float err = in - to_float(sample);
    return std::make_tuple(sample, err); 
}

Image8 convert(const Image32& img);

Image8 load_image(const char* path);

bool   write_image_png(const Image8& image, const char* path);

void   flip_vertical(Image8& img);
void image_fill(Image8& img);



}

