/**\file glh_names.h Common program variables
    \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/
#pragma once

namespace glh{

//////////////// Reserved shader and dynamic graph names. ////////////////
// Anything to do with these
// has probably some hidden un-explicit side effects attached
// to them. TODO: Try to make all side-effects explicit.

#define UICTX_SELECT_NAME "SelectionColor"

#define GLH_CHANNEL_ROTATION        "GLH_CHANNEL_ROTATION"
#define GLH_CHANNEL_POSITION        "GLH_CHANNEL_POSITION"
#define GLH_CHANNEL_SCALE           "GLH_CHANNEL_SCALE"
                                    
#define GLH_CHANNEL_TIME            "GLH_CHANNEL_TIME"
#define GLH_PROPERTY_TIME_DELTA     "GLH_PROPERTY_TIME_DELTA"

#define GLH_PROPERTY_POSITION_DELTA "GLH_PROPERTY_POSITION_DELTA"
#define GLH_PROPERTY_COLOR          "GLH_PROPERTY_COLOR"

#define GLH_PROPERTY_1              "GLH_PROPERTY_1"
#define GLH_PROPERTY_2              "GLH_PROPERTY_2"
                                    
#define GLH_PROPERTY_BIAS           "GLH_PROPERTY_BIAS"
#define GLH_PROPERTY_SCALE          "GLH_PROPERTY_SCALE"
#define GLH_PROPERTY_INTERPOLANT    "GLH_PROPERTY_INTERPOLANT"
#define GLH_PROPERTY_DELTA          "GLH_PROPERTY_DELTA"

#define GLH_COLOR_DELTA             "GLH_COLOR_DELTA"
#define GLH_PRIMARY_COLOR           "GLH_PRIMARY_COLOR"
#define GLH_SECONDARY_COLOR         "GLH_SECONDARY_COLOR"

//////////////// Program names. ////////////////

#define GLH_COLOR_PICKER_PROGRAM             "GLHColorPickerProgram"
#define GLH_CONSTANT_ALBEDO_PROGRAM          "GLHConstantAlbedoProgram"
#define GLH_TEXTURED_FONT_PROGRAM_SOLID_FILL "GLHTexturedFontProgramSolidFill"

//////////////// Shader parameters. ////////////////

#define GLH_LOCAL_TO_WORLD   "LocalToWorld"
#define GLH_WORLD_TO_CAMERA  "WorldToCamera"
#define GLH_CAMERA_TO_SCREEN "CameraToScreen"
#define GLH_WORLD_TO_SCREEN  "WorldToScreen"

#define GLH_COLOR_ALBEDO     "ColorAlbedo"


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
