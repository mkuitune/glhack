/**\file asset_manager.cpp 
    \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/

#include "glh_typedefs.h"
#include "glsystem.h"
#include "asset_manager.h"
#include "iotools.h"
#include "masp.h"

namespace glh{

const char* g_asset_path_name = "asset_path";


std::string fix_win32_path(const std::string& path)
{
    std::string out(path);
    for(auto& c : out) if(c == '/') c = '\\';
    return out;
}

class DefaultAssetManager : public AssetManager {
public:
    DefaultAssetManager(const char* config_file):config_file_path(config_file)
    {
        masp::Masp M;
        std::string config_file_lines;
        bool file_ok;

        std::tie(config_file_lines, file_ok) = file_to_string(config_file_path.c_str());

        if(file_ok)
        {
            // TODO: masp::readfile
            masp::parser_result parser_result = string_to_value(M, config_file_lines.c_str());
            if(parser_result.valid())
            {
                masp::evaluation_result eval_result = masp::eval(M, (*parser_result).get());
                if(eval_result.valid()){
                    const masp::Value* asset_path_val = masp::get_value(M, g_asset_path_name);
                    if(asset_path_val) asset_path = masp::get_value_string(asset_path_val);
                } else {
                    std::string msg = std::string("Config file evaluation error:") + parser_result.message() + std::string("(") + config_file_path + std::string(")");
                    throw GraphicsException(msg);
                }
            } else {
                std::string msg = std::string("Config file parse error:") + parser_result.message() + std::string("(") + config_file_path + std::string(")");
                throw GraphicsException(msg);
            }
        } else {
            std::string msg = std::string("Could not find config file:") + config_file_path;
            throw GraphicsException(msg);
        }
    }

    bool exists(const char* path) override {
        bool result = true;
        InputFile i(path);
        result = i.is_open();
        return result;
    }

    Image8 load_image_gl(const char* path) override {
        std::string fullpath = compose_full_path(path);
        Image8 image = load_image(fullpath.c_str());
        flip_vertical(image);
        return image;
    }

    std::string load_text(const char* path) override
    {
        std::string text;
        bool        success;

        std::string fullpath = compose_full_path(path);
        std::tie(text, success) = file_to_string(fullpath.c_str());
        if(success){
            return text;
        }else{
            std::string msg = std::string("Could not find file:") + fullpath;
            throw GraphicsException(msg);
        }

    }

// Class specific
    std::string compose_full_path(const char* path){
        std::string filepath;
        if(asset_path.size() > 0){
            // Internal paths all use '/' -as a path separator, for system interfacing this is replaced with '\' where appropriate (windows).
            char last = *asset_path.rbegin();
            bool terminates_on_pathsign = (last == '/' ||  last == '\\');
            filepath = terminates_on_pathsign ? asset_path + std::string(path) : asset_path + "/" + std::string(path);
        }else{
            filepath = std::string(path);
        }

#ifdef WIN32
        return fix_win32_path(filepath);
#else
        return filepath;
#endif
    }


    std::string asset_path;
    std::string config_file_path;
};


std::shared_ptr<AssetManager> make_asset_manager(const char* config_file)
{
    std::shared_ptr<AssetManager> manager(new DefaultAssetManager(config_file)); 
   
    return manager;
}

}// namespace glh
