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
#include "glh_uicontext.h"

namespace glh{

/** Container for the services initializable only after GL context exist. */
class AppServices{
public:

    App*            app_;

    FontManagerPtr  fontmanager_;
    AssetManagerPtr manager_;

    DynamicGraph graph_;

    std::shared_ptr<SceneAssets>    assets_;

    std::shared_ptr<UiContext> ui_context_;

    glh::RenderPickerPtr render_picker_;

    // TODO: RenderQueue
private:
    void init_font_manager();

public:
    void init(App* app, const char* config_file);

    void update(){
        assets_->update();
    }

    void finalize(){
        assets_.reset();
        manager_.reset();
        fontmanager_.reset();
    }

    FontManager* fontmanager(){ return fontmanager_.get(); }

    DynamicGraph& graph(){ return graph_; }
    UiContext&    ui_context(){ return *ui_context_; }
    SceneAssets& assets(){ return *assets_; }


};

}
