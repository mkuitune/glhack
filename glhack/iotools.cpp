/** \file iotools.cpp
    \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/
#include "iotools.h"


#ifdef WIN32
#include "win32_dirent.h"
#else
#include <cdirent>
#endif


namespace {
std::string fix_path_to_win32_path(const std::string& path)
{
    std::string out(path);
    for(auto& c : out) if(c == '/') c = '\\';
    return out;
}
std::string fix_path_to_posix_path(const std::string& path)
{
    std::string out(path);
    for(auto& c : out) if(c == '\\') c = '/';
    return out;
}
} // end anonymous namespace

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

std::tuple<std::vector<uint8_t>, bool> file_to_bytes(const char* path)
{
    InputFile file(path);
    return file.contents_to_bytes();
}

bool string_to_file(const char* path, const char* string)
{
    bool result = false;
    OutputFile o(path);
    return o.write(string);
}

std::string path_join(const std::string& head, const std::string& tail)
{
    char separator = platform_separator();

    std::string normalized_head = path_to_platform_string(head);
    std::string normalized_tail = path_to_platform_string(tail);

    if(head.size() > 0){
        if(*head.rbegin() != separator) return normalized_head + separator + normalized_tail;
        else                          return normalized_head + normalized_tail;
    }
    else return normalized_tail;
}

std::list<std::string> path_split(const std::string& path)
{
    std::list<std::string> segments;
    segments.push_back(std::string("TODO IMPLEMENT"));
    return segments;
}

char platform_separator(){
#ifdef WIN32
    return '\\';
#else
    return return '/';
#endif
}

std::string path_to_platform_string(const std::string& path)
{
#ifdef WIN32
    return fix_path_to_win32_path(path);
#else
    return fix_path_to_posix_path(path);
#endif
}

//////////// InputFile //////////////////

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

std::tuple<std::vector<uint8_t>, bool> InputFile::contents_to_bytes()
{
    std::vector<uint8_t> contents;

    bool success = false;
    if(file_)
    {
        file_.seekg(0, std::ios::end);
        const size_t filesize = (size_t) file_.tellg();
        contents.resize(filesize);
        file_.seekg(0, std::ios::beg);
        file_.read(reinterpret_cast<char*>(&contents[0]), contents.size());
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
    file_.open(path, openflags);
}

OutputFile::~OutputFile(){if(file_) file_.close();}

bool OutputFile::is_open(){if(file_) return true; else return false;}

void OutputFile::close(){file_.close();}

bool OutputFile::write_str(std::string& str)
{
    return write(str.c_str());
}

bool OutputFile::write(const char* str)
{
    bool result = false;
    if(is_open()){
        file_ << str;
        result = true;
    }
    return result;
}

std::ofstream& OutputFile::file(){return file_;}
