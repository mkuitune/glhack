/** \file glh_layout_tools.h Layout utils.
    \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/
#pragma once

#include "math_tools.h"

namespace glh{

struct Layout{
    vec2 origin_;
    vec2 size_;

    bool operator!=(const Layout& l){ return origin_ != l.origin_ && size_ != l.size_; }

    Box2f coverage() const { return Box2f(origin_, origin_ + size_); }
};

} // namespace glh

