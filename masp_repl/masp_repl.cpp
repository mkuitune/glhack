#include<iostream>
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

    std::string line;
    bool live = true;
    while(live)
    {
        std::cout << ">";
        std::cin >> line;
    
        if(line.compare("quit") == 0)
        {
            live = false;
        }
        else if(line.compare("help") == 0)
        {
            print_help();
        }
        else
        {
            parser_result result = string_to_atom(M, line.c_str());
            if(result.valid())
            {
                // Masp::Atom eval_result = eval(M, *result);
                // std::string outline = atom_to_string(eval_result);
                // std::cout << outline << std::endl;
            }
            else
            {
                std::cout << "Parse error:" << result.message() << std::endl;
            }
        }
    }

    return 0;
}

