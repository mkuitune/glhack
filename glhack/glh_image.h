/**\file glh_image.h 
    \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/
#pragma once

#include <functional>
#include "glh_typedefs.h"

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
    typedef std::function<void(uint8_t*)> Deallocator;

    int      width_;
    int      height_;
    int      channels_;
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
};

Image8 load_image(const char* path);
bool   write_image_png(const Image8& image, const char* path);
void   flip_vertical(Image8& img);

void image_fill(Image8& img);

}

