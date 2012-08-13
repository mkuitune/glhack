/** \file allocator.h Memory allocators.*/
#pragma once


#include <cstring>

/** Return memory aligned to 16 bytes. Must be freed using alignedFree */
unsigned char* aligned_alloc(size_t size);
void aligned_free(void* p);
