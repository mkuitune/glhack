/**\file glh_names.h Common program variables
    \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/
#pragma once

namespace glh{

//////////////// Reserved shader names. ////////////////
// Anything to do with these
// has probably some hidden un-explicit side effects attached
// to them. TODO: Try to make all side-effects explicit.

#define UICTX_SELECT_NAME "SelectionColor"

#define GLH_LOCAL_TO_WORLD  "LocalToWorld"

#define GLH_CHANNEL_ROTATION "rotation"
#define GLH_CHANNEL_POSITION "position"
#define GLH_CHANNEL_SCALE    "scale"
//////////////// Color constants. ////////////////
// Can be used as eg. 
//    float red[] = {COLOR_RED};
//    or
//    vec4 red = vec4(COLOR_RED);

#define COLOR_RED    1.f, 0.f, 0.f, 1.f
#define COLOR_GREEN  0.f, 1.f, 0.f, 1.f
#define COLOR_BLUE   0.f, 0.f, 1.f, 1.f
#define COLOR_YELLOW 1.f, 1.f, 0.f, 1.f
#define COLOR_PURPLE 1.f, 0.f, 1.f, 1.f
#define COLOR_CYAN   0.f, 1.f, 1.f, 1.f
#define COLOR_WHITE  1.f, 1.f, 1.f, 1.f
#define COLOR_BLACK  0.f, 0.f, 0.f, 1.f


} // namespace glh
