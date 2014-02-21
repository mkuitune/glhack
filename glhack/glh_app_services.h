/**\file glh_app_services.h
    \brief Assets pending on existing OpenGL context.
    \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/
#pragma once

#include "glbase.h"
#include "asset_manager.h"
#include "glh_generators.h"
#include "glh_image.h"
#include "glh_typedefs.h"
#include "shims_and_types.h"
#include "glh_scene_extensions.h"
#include "iotools.h"


namespace glh{

/** Container for the services initializable only after GL context exist. */
class AppServices{
public:

    App*            app_;

    FontManagerPtr  fontmanager_;
    AssetManagerPtr manager_;

    std::shared_ptr<SceneAssets>  assets_;

    // TODO: RenderQueue

    void init_font_manager()
    {
        GraphicsManager* gm = app_->graphics_manager();

        std::string fontpath = manager_->fontpath();

        if(!directory_exists(fontpath.c_str())){
            throw GraphicsException(std::string("Font directory not found:") + fontpath);
        }

        fontmanager_.reset(new FontManager(gm, fontpath));
    }

    SceneAssets& assets(){ return *assets_; }

    void init(App* app, const char* config_file)
    {
        app_ = app;
        GraphicsManager* gm = app_->graphics_manager();

        manager_ = make_asset_manager(config_file);

        init_font_manager();

        // TODO: Use the 2d camera as the camera for the gui renderpass

        assets_ = SceneAssets::create(app_, gm, fontmanager_.get());

        if(!assets_.get()){ throw GraphicsException("AppServices: Could not init assets_"); }
    }

    void update(){
        assets_->update();
    }

    void finalize(){
        assets_.reset();
        manager_.reset();
        fontmanager_.reset();
    }

    FontManager* fontmanager(){ return fontmanager_.get(); }
};

}
