#include<iostream>
#include<cstring>
#include "masp.h"

void print_help()
{
    std::cout << "Welcome to Masp parser version " << MASP_VERSION << "\n";
}

int main(int argc, char* argv[])
{
    using namespace masp;

    Masp M;

    std::cout << "Masp repl\n" << std::endl;

    bool live = true;
    const int line_size = 1024;
    char line[line_size];
    while(live)
    {
        std::cout << ">";
        std::cin.getline(line, line_size);
    
        if(strcmp(line,"quit") == 0)
        {
            live = false;
        }
        else if(strcmp(line, "help") == 0)
        {
            print_help();
        }
        else
        {
            parser_result result = string_to_atom(M, line);
            if(result.valid())
            {
                // Masp::Atom eval_result = eval(M, *result);
                std::string outline = atom_to_string(*result);
                // std::string outline = atom_to_string(eval_result);
                std::cout << outline << std::endl;
            }
            else
            {
                std::cout << "Parse error:" << result.message() << std::endl;
            }
        }
    }

    return 0;
}

