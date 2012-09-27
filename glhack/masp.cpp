#include "masp.h"
#include "persistent_containers.h"

#include<stack>
#include<cstring>
#include<algorithm>
#include<cstdlib>
#include<cctype>
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

/** Return pointer either to the next newline ('\n') or to the end of the given range.
*/
const char* to_newline(const char* begin, const char* end)
{
    const char* c = begin;
    while(c != end && *c != '\n'){c++;}
    return c;
}

/** Return pointer to end of string. Returns the pointer to the last
 *  quote in strings delimited with " -characters.
 *
 *  @param begin Pointer to first element after the initial " in the string.
 *  @param end   Pointer to the null termination character for the string.
 *  @return      Pointer to the delimiting " in a string or to the end of the given range.
 */
static const char* last_quote_of_string(const char* begin, const char* end)
{
    const char* last = end;
    const char* c = begin;
    size_t i = 0;

    while(c != end)
    {
        if(*c == '\\')
        {
            c++;
            if(c == end) return c;
        }
        else if(*c == '"') 
        {
            return c;
        }

        c++;
    }

    return c;
}

/** Match the pattern to the beginning of the given range.
 * @param pattern        Pattern to match
 * @param pattern_length Length of pattern (equalent to to strlen).
 * @param begin          Beginning of the range to scan.
 * @param end            End of the range to scan.
 * @return               True if the pattern matched the beginning of the range, otherwise false.
*/
static bool match_string(const char* pattern, const size_t pattern_length, const char* begin, const char* end)
{
    bool result = false;
    size_t length = end - begin;
    if(length >= pattern_length)
    {
        size_t i = 0;
        const char* c = begin;
        result = true;
        while(i++ < pattern_length && result){result = *pattern++ == *c++;}
    }
    return result;
}

/** Verify that give scope delimiters balance out.*/
static bool check_scope(const char* begin, const char* end, const char* comment, char scope_start, char scope_end)
{
    bool result = true;
    const char* c = begin;
    int scope = 0;
    size_t comment_length = strlen(comment);
    while(c != end)
    {
        if(match_string(comment, comment_length, c, end))
        {
            c = to_newline(c, end);
        }
        if(*c == '"')
        {
            c = last_quote_of_string(c, end);
        }
        else if(*c == scope_start)
        {
            scope = scope + 1;
        }
        else if(*c == scope_end)
        {
            scope = scope - 1;
        }

        if(scope < 0)
        {
            break;
        }

        c++;
    }

    if(scope != 0) result = false;

    return result;
}



/** Parse string to atom. Use one instance of AtomParser per string/atom pair. */
class AtomParser
{
public:

    AtomParser()
    {
        reading_string = false;
    }

    // List stack.

    typedef std::list<Masp::Atom> atom_list;
    std::stack<atom_list* > stack;

    atom_list* push_list(atom_list* list)
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
    atom_list* pop_list()
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

    charptr c_;
    charptr end_;

    bool is(char ch){return *c_ == ch;}
    bool at_end(){return c_ == end_;}
    void init(const char* start, const char* end){c_ = start; end_ = end;}
    const char* next(){return c_ + 1;}
    void move_forward(){c_ = c_ + 1;}
    void set(const char * ch){c_ = ch;}
    void to_newline()
    {
        while(c_ != end_ && *c_ != '\n'){c_++;}
    }

    bool is_space(){return isspace(*c_) != 0;}

    const char* parse_string()
    {
        return last_quote_of_string(c_,end_);
    }

    bool parse_number()
    {
        return true;
    }

    bool parse_symbol()
    {
        return true;
    }

    Masp::parser_result parse(const char* str)
    {
        Masp::Atom root = make_atom_list();
        atom_list* list = push_list(root.value.list);

        size_t size = strlen(str);
        init(str, str + size);

        bool scope_valid = check_scope(c_, end_, ";", '(', ')');

        if(!scope_valid)
        {
            return Masp::parser_result("Could not parse, error in list scope - misplaced '(' or ')' ");
        }

        while(! at_end())
        {
            if(is('"')) //> string
            {
                const charptr last = parse_string();
                list->push_back(make_atom_string(next(), last));
                set(last);
            }
            else if(is(';'))  //> newline
            {
                to_newline();
            }
            else if(is('(')) // Enter list
            {
                list->push_back(make_atom_list());
                atom_list* new_list = list->back().value.list;
                list = push_list(new_list);
            }
            else if(is(')')) // Exit list
            {
                list = pop_list();
            }
            else if(is_space())
            {
                // After this we know that the input is either number or symbol
            }
            else if(parse_number())
            {
                // push back number on list make_atom_number
            }
            else if(parse_symbol())
            {
                // push back symbol on list make_atom_symbol
            }

            if(at_end()) break;
            move_forward();
        }

        return Masp::parser_result(root);
    }

};

Masp::parser_result string_to_atom(const char* str)
{
    AtomParser parser;

    return parser.parse(str);
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

