/**\file glh_default_assets.h
    \brief Shorcut to load a bunch of expected assets to make test programs shorter.
    \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/
#pragma once

namespace glh{

//#include "glh_scene_extensions.h"

class DefaultAssets
{
public:

    int i;
};

DefaultAssets* get_default_assets();

void finalize_default_assets();

}
