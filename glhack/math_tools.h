/** \file math_tools.h Usefull wrappers and tools for numbers.
*/
#pragma once

#include <Eigen/Dense>

#include <stdint.h>

namespace glh{

typedef Eigen::Vector4f vec4;
typedef Eigen::Vector3f vec3;
typedef Eigen::Vector2i vec2i;


/////////////// Bit operations //////////////


/** Count bits in field */
inline const uint32_t count_bits(uint32_t field)
{
    uint32_t count = 0;
    for(;field;count++) field &= field - 1;
    return count;
}

/** Return index of lowest unset bit*/
inline const uint32_t lowest_unset_bit(uint32_t field)
{
    if(field == 0xffffffff) return 32; //> not found
    uint32_t index = 0;
    for(;(field&0x1); field >>= 1 , index++){}

    return index;
}

/** Set nth bit in field. Return result. */
inline uint32_t set_bit_on(const uint32_t field, uint32_t index)
{
    return index < 32 ? (field | 1 << index) : field;
}

/** Unset nth bit in field. Return result. */
inline uint32_t set_bit_off(uint32_t field, uint32_t index)
{
    return index < 32 ? (field & ~(1 << index)) : field;
}

/** Return true if bit is on */
inline bool bit_is_on(const uint32_t field, const uint32_t bit)
{
    return  (field & 1 << bit) != 0;
}

/////////////// Hash functions //////////////

uint32_t hash32(const char* data, int len);

} // namespace glh