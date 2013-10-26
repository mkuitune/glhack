/**\file glh_default_assets.h
    \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/

#include "glh_default_assets.h"

#include <memory>

namespace glh{

std::shared_ptr<DefaultAssets> g_default_assets;

DefaultAssets* get_default_assets(){
    if(!g_default_assets.get()){
        g_default_assets.reset(new DefaultAssets);
    }

    return g_default_assets.get();
}

void finalize_default_assets(){
    g_default_assets.reset();
}


}
