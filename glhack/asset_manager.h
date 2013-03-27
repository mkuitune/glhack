/**\file asset_manager.h 
    \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/
#pragma once

#include "glh_image.h"
#include "shims_and_types.h"
#include <memory>

namespace glh{


/** Manage external assets. */
DeclInterface(AssetManager,
        virtual bool        exists(const char* path) = 0;
        virtual Image8      load_image_gl(const char* path)  = 0;
        virtual std::string load_text(const char* path) = 0;
        virtual std::string fontpath() = 0;
);

typedef std::shared_ptr<AssetManager> AssetManagerPtr;
AssetManagerPtr make_asset_manager(const char* config_file);

}
