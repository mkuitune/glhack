#include<iostream>
#include<cstring>
#include <sstream>
#include "masp.h"


void print_help()
{
    std::cout << "Welcome to Masp parser version " << MASP_VERSION << "\n" <<
                 "'help' Show this help.\n" <<
                 "'quit' Exit interpreter.\n" <<
                 "'memory' Display used memory (live/reserved).\n";
}

//TODO: gc

void printing_response(masp::Masp& M, masp::Value* v)
{
    std::string outline = masp::value_to_typed_string(v);
    std::cout << outline << std::endl;
}

void eval_response(masp::Masp& M, masp::Value* v)
{
    masp::masp_result result = masp::eval(M,v);
    if(result.valid())
    {
        printing_response(M, (*result).get());
    }
    else
    {
        std::cout << "Parse error:" << result.message() << std::endl;
    }

}

std::string memory_string(size_t b)
{
    std::ostringstream os;

    //if(b < 1024) os << b << " B";
    //else if(b > 1024 && b < 1024 * 1024) os << b/1024 << " kB";
    //else os << b/(1024*1024) << " MB";
    os << b << " B";
    return os.str();
}

void print_memory(std::ostream& os, const char* prefix, size_t live, size_t reserved)
{
    os << "(live/reserved): " << memory_string(live) << " / " << memory_string(reserved) << std::endl;
}

int main(int argc, char* argv[])
{
    using namespace masp;
    using std::cout;
    using std::cin;
    using std::endl;

    Masp M;

    std::cout << "Masp repl\n" << std::endl;

    bool live = true;
    const int line_size = 1024;
    char line[line_size];

    enum {EVAL, PRINT} mode = EVAL;
    
    while(live)
    {
        cout << ">";
        cin.getline(line, line_size);
    
        if(strcmp(line,"quit") == 0)
        {
            live = false;
        }
        else if(strcmp(line, "help") == 0)
        {
            print_help();
        }
        else if(strcmp(line, "memory") == 0)
        {
            size_t live_size = M.live_size_bytes();
            size_t reserved_size = M.reserved_size_bytes();
            print_memory(cout, "Memory used ",live_size, reserved_size);
        }
        else if(strcmp(line, "gc") == 0)
        {
            size_t live_size_before = M.live_size_bytes();
            size_t reserved_size_before = M.reserved_size_bytes();

            M.gc();

            size_t live_size = M.live_size_bytes();
            size_t reserved_size = M.reserved_size_bytes();

            cout << "Garbage collection done. Memory usage statistics:";
            print_memory(cout, "Before collection: ",live_size_before, reserved_size_before);
            print_memory(cout, "After collection: ", live_size, reserved_size);
        }
        else if(strcmp(line, "eval") == 0)
        {
            mode = EVAL;
        }
        else if(strcmp(line, "print") == 0)
        {
            mode = PRINT;
        }
        else
        {
            masp_result result = string_to_value(M, line);
            if(result.valid())
            {
                if(mode == PRINT)
                {
                    printing_response(M, (*result).get());
                }
                else if(mode == EVAL)
                {
                    eval_response(M, (*result).get());
                }
            }
            else
            {
                cout << "Parse error:" << result.message() << endl;
            }
        }
    }

    return 0;
}

