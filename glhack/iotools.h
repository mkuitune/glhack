/** \file iotools.h
    \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/
#pragma once

#include<list>
#include<string>
#include<fstream>
#include<tuple>

bool file_exists(const char* path);
std::list<std::string> list_dir(const char* path);
bool is_file(const char* path);
bool is_directory(const char* path);

std::tuple<std::string, bool> file_to_string(const char* path);
bool string_to_file(const char* path, const char* string);

// All internal path operations expect '/' separator for paths

/** Join two file system paths together */
std::string path_join(const std::string& head, const std::string& tail);
/** Split path on '/ ' - to segments.*/
std::list<std::string> path_split(const std::string& path);

class InputFile{
private:
    InputFile(const InputFile& i){}
public:
    InputFile(const char* path);
    ~InputFile();
    std::ifstream& file();
    bool is_open();
    void close();
    std::tuple<std::string, bool> contents_to_string();

    std::ifstream file_;
};

class OutputFile{
private:
    OutputFile(const OutputFile& o){}
public:
    OutputFile(const char* path);
    OutputFile(const char* path, bool append);
    ~OutputFile();
    bool is_open();
    void close();
    bool write_str(std::string& str);
    bool write(const char* str);

    std::ofstream& file();

    std::ofstream file_;
};

