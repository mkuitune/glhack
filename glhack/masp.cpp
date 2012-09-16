#include "masp.h"
#include "persistent_containers.h"

#include<stack>
#include<cstring>
#include<algorithm>
#include<cstdlib>
#include<sstream>

////// Masp::Atom ///////

Masp::Atom make_atom_number(int i)
{
    Masp::Atom a;
    a.value.number.set(i);
    a.type = Masp::NUMBER;
    return a;
}

Masp::Atom make_atom_number(double d)
{
    Masp::Atom a;
    a.value.number.set(d);
    a.type = Masp::NUMBER;
    return a;
}

Masp::Atom make_atom_string(const char* str)
{
    Masp::Atom a;
    a.type = Masp::STRING;
    a.alloc_str(str);
    return a;
}

Masp::Atom make_atom_string(const char* str, const char* str_end)
{
    Masp::Atom a;
    a.type = Masp::STRING;
    a.alloc_str(str, str_end);
    return a;
}


Masp::Atom make_atom_symbol(const char* str)
{
    Masp::Atom a;
    a.type = Masp::SYMBOL;
    a.alloc_str(str);
    return a;
}

Masp::Atom make_atom_symbol(const char* str, const char* str_end)
{
    Masp::Atom a;
    a.type = Masp::SYMBOL;
    a.alloc_str(str, str_end);
    return a;
}

Masp::Atom make_atom_list()
{
    Masp::Atom a;
    a.type = Masp::LIST;
    a.value.list = new std::list<Masp::Atom>();
    return a;
}

Masp::Atom::Atom()
{
}

Masp::Atom::~Atom()
{
    if((type == Masp::STRING || type == Masp::SYMBOL) && value.string)
    {
        free(((void*)value.string));
    }
    else if(type == Masp::LIST && value.list)
    {
        delete value.list;
    }
}

Masp::Atom::Atom(const Atom& atom)
{
    type = atom.type;
    switch(atom.type)
    {
        case Masp::NUMBER:
        {
            value.number.set(atom.value.number);
            break;
        }
        case Masp::SYMBOL:
        case Masp::STRING:
        {
            alloc_str(atom.value.string);
            break;
        }
        case Masp::LIST:
        {

            value.list = new std::list<Masp::Atom>(atom.value.list->begin(), atom.value.list->end());
            break;
        }
        default:
            assert(!"Illegal Atom type");
    }
}

Masp::Atom::Atom(Atom&& atom)
{
    type = atom.type;
    switch(atom.type)
    {
        case Masp::NUMBER:
        {
            value.number.set(atom.value.number);
            break;
        }
        case Masp::SYMBOL:
        case Masp::STRING:
        {
            value.string = atom.value.string;
            atom.value.string = 0;
            break;
        }
        case Masp::LIST:
        {
            value.list = atom.value.list;
            atom.value.list = 0;
            break;
        }
        default:
            assert(!"Illegal Atom type");
    }
}

Masp::Atom& Masp::Atom::operator=(const Atom& a)
{
    if(&a == this) return *this;
    else
    {
        type = a.type;
        switch(type)
        {
            case Masp::NUMBER:
            {
                value.number.set(a.value.number);
                break;
            }
            case Masp::SYMBOL:
            case Masp::STRING:
            {
                alloc_str(a.value.string);
                break;
            }
            case Masp::LIST:
            {
                value.list = new std::list<Masp::Atom>(a.value.list->begin(), a.value.list->end());
                break;
            }
            default:
                assert(!"Illegal Atom type");
        }
    }
    return *this;
}

Masp::Atom& Masp::Atom::operator=(Atom&& atom)
{
    if(&atom == this) return *this;

    type = atom.type;
    switch(atom.type)
    {
        case Masp::NUMBER:
        {
            value.number.set(atom.value.number);
            break;
        }
        case Masp::SYMBOL:
        case Masp::STRING:
        {
            value.string = atom.value.string;
            atom.value.string = 0;
            break;
        }
        case Masp::LIST:
        {
            value.list = atom.value.list;
            atom.value.list = 0;
            break;
        }
        default:
            assert(!"Illegal Atom type");
    }
    return *this;
}

void Masp::Atom::alloc_str(const char* str)
{
    size_t size = strlen(str) + 1;
    value.string = (const char*) malloc(size * sizeof(char));
    strcpy(((char*)value.string), str);
}

void Masp::Atom::alloc_str(const char* str, const char* str_end)
{
    size_t size = str_end - str + 1;
    value.string = (const char*) malloc(size * sizeof(char));
    strncat(((char*)value.string), str, size - 1);
}

///// Utility functions /////

class AtomParser
{
public:

    AtomParser()
    {
        reading_string = false;

        LPAREN  ="(";
        RPAREN  =")";
        COMMENT =";;";
        QUOTE   ="'";
        NEWLINE = "\n";
    }

    // List stack.

    typedef std::list<Masp::Atom> atom_list;
    std::stack<atom_list* > stack;

    atom_list* push(atom_list* list)
    {
        atom_list* result = 0;
        if(list)
        {
            stack.push(list);
            result = list;
        }
        else
        {
            assert(!"AtomParser: Attempted to push null list to stack.");
        }
        return result;
    }

    /** Pop list stack, return popped list if not empty.*/
    atom_list* pop()
    {
        atom_list* result = 0;
        if(!stack.empty())
        {
            result = stack.top();
            stack.pop();
        }
        return result;
    }

    bool reading_string;

    // Parse order:
    // 1. check if ;; and skip to next line
    // 2. check if ( and push list
    // 3. check if ) and pop list
    // 4. check if is float and parse number
    // 5. check if is int and parse number
    // 6. check if is string and read string
    // 7. check if is symbol and read symbol
    //
    // a. Unless reading string symbols are broken on ', whitespace, (, ) and ;;
    // b. if reading string string is broken only when reading matching quote as on start
    //    \- will escape the following quote

    typedef const char* charptr;

    charptr LPAREN;
    charptr RPAREN;
    charptr COMMENT;
    charptr QUOTE;
    charptr NEWLINE;


    void parse_string(charptr c, charptr end, charptr* last)
    {
        size_t i = 0;
        while(c != end)
        {
            if(*c == '\\')
            {
                c++;
                if(c==end) return;
            }
            else if(*c == '"') 
            {
                *last = c;
                return;
            }

            c++;
        }
    }

    Masp::Atom parse(const char* str, std::string& err)
    {
        Masp::Atom root = make_atom_list();
        atom_list* list = push(root.value.list);

        size_t size = strlen(str);
        size_t i = 0;
        charptr end = str + size;



        while(i < size)
        {
            charptr c = str + i;

            // Parse string
            if(*c == '"') 
            {
                const char* last = 0;
                parse_string(c + 1, end, &last);
                if(last)
                {
                    i = last - str;
                    list->push_back(make_atom_string(c + 1, last));
                }
                else break;
                 
            }

            i++;
        }

        return root;
    }

};

Masp::Atom string_to_atom(const char* str, std::string& err)
{
    AtomParser parser;
    err = "";
    Masp::Atom a = parser.parse(str, err);
    return a;
}

static void atom_to_string_helper(std::ostream& os, const Masp::Atom& a)
{
    switch(a.type)
    {
        case Masp::NUMBER:
        {
            if(a.value.number.type == Masp::Number::INT)
            {
                os << a.value.number.to_int() << " ";
            }
            else
            {
                os << a.value.number.to_float() << " ";
            }
            break;
        }
        case Masp::SYMBOL:
        {
            os << a.value.string << " ";
            break;
        }
        case Masp::STRING:
        {
            os << "\"" << a.value.string << "\" ";
            break;
        }
        case Masp::LIST:
        {
            os << "(";
            for(auto i = a.value.list->begin(); i != a.value.list->end(); ++i)
            {
                atom_to_string_helper(os, *i);
            }
            os << ")";
            break;
        }
    }
}

const std::string atom_to_string(const Masp::Atom& atom)
{
    std::ostringstream stream;
    atom_to_string_helper(stream, atom);
    return stream.str();
}

///// Masp::Env //////
class Masp::Env
{
public:

    typedef glh::PMapPool<std::string, Masp::Value>      value_pool;
    typedef glh::PMapPool<std::string, Masp::Value>::Map value_map;

    Masp::Env():env_pool_()
    {
        env_stack_.push(env_pool_.new_map());
    }

    value_pool            env_pool_;
    std::stack<value_map> env_stack_;
};


////// Masp ///////

Masp::Masp()
{
    env_ = new Env();
}

Masp::~Masp()
{
    delete env_;
}

