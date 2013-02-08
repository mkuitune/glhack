/** \file iotools.cpp
    \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/
#include "iotools.h"

std::tuple<std::string, bool> file_to_string(const char* path)
{
    InputFile file(path);
    return file.contents_to_string();
}

InputFile::InputFile(const char* path):file_(path, std::ios::in|std::ios::binary){}

InputFile::~InputFile(){if(file_) file_.close();}

std::ifstream& InputFile::file(){return file_;}

bool InputFile::is_open(){if(file_) return true; else return false;}

std::tuple<std::string, bool> InputFile::contents_to_string()
{
    std::string contents;
    bool success = false;
    if(file_)
    {
        file_.seekg(0, std::ios::end);
        const size_t filesize = (size_t) file_.tellg();
        contents.resize(filesize);
        file_.seekg(0, std::ios::beg);
        file_.read(&contents[0], contents.size());
        success = true;
    }

    return std::make_tuple(contents, success);
}
