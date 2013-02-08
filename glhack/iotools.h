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

class InputFile{
public:
    InputFile(const char* path);
    ~InputFile();
    std::ifstream& file();
    bool is_open();
    std::tuple<std::string, bool> contents_to_string();

    std::ifstream file_;
};

class OutputFile{
public:
    std::ofstream& file(){}

    std::ofstream file_;
};

