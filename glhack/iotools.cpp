/** \file iotools.cpp
    \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/
#include "iotools.h"

#ifdef WIN32
#include "win32_dirent.h"
#else
#include <cdirent>
#endif

bool file_exists(const char* path)
{
    bool result = false;

    return result;
}

std::list<std::string> list_dir(const char* path)
{
    std::list<std::string> contents;

    return contents;
}

bool is_file(const char* path)
{
    bool result = false;

    return result;
}

bool is_directory(const char* path)
{
     bool result = false;

    return result;
}

std::tuple<std::string, bool> file_to_string(const char* path)
{
    InputFile file(path);
    return file.contents_to_string();
}

bool string_to_file(const char* path, const char* string)
{
    bool result = false;
    OutputFile o(path);
    if(o.is_open()){
        o.file() << string;
        result = true;
    }
    return result;
}

InputFile::InputFile(const char* path):file_(path, std::ios::in|std::ios::binary){}

InputFile::~InputFile(){if(file_) file_.close();}

std::ifstream& InputFile::file(){return file_;}

bool InputFile::is_open(){if(file_) return true; else return false;}

void InputFile::close(){file_.close();}

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

//// OutputFile ////

OutputFile::OutputFile(const char* path)
{
    file_.open(path, std::ios::out | std::ios::binary);
}

OutputFile::OutputFile(const char* path, bool append)
{
    auto openflags = append ? std::ios::out | std::ios::app | std::ios::binary : std::ios::out | std::ios::binary;
    file_.open(path, std::ios::out | std::ios::binary);
}

OutputFile::~OutputFile(){if(file_) file_.close();}

bool OutputFile::is_open(){if(file_) return true; else return false;}

void OutputFile::close(){file_.close();}

std::ofstream& OutputFile::file(){return file_;}