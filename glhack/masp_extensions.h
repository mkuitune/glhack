/** \file masp_extensions.h
    \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/
#pragma once

#include "masp.h"

namespace masp{
/* Add system callbacks to masp - file io. */
void load_masp_unsafe_extensions(masp::Masp& m);
}

