#include "masp.h"
#include "persistent_containers.h"

#include<stack>
#include<cstring>
#include<algorithm>
#include<cstdlib>
#include<cctype>
#include<regex>
#include<sstream>

namespace masp{

////// Opaque value wrappers. ///////

typedef glh::PListPool<Value>       ListPool;
typedef glh::PListPool<Value>::List List;

struct Map{int i;};
struct Closure{int i;};
struct Object{int i;};
struct Vector{int i;};


List* reveal(const ListRef& ref){return reinterpret_cast<List*>(ref.data);}
Map* reveal(const MapRef& ref){return reinterpret_cast<Map*>(ref.data);}
Closure* reveal(const ClosureRef& ref){return reinterpret_cast<Closure*>(ref.data);}
Object* reveal(const ObjectRef& ref){return reinterpret_cast<Object*>(ref.data);}
Vector* reveal(const VectorRef& ref){return reinterpret_cast<Vector*>(ref.data);}

ListRef hide(List* ref){ListRef r = {ref};return r;}
MapRef hide(Map* ref){MapRef r = {ref};return r;}
ClosureRef hide(Closure* ref){ClosureRef r = {ref};return r;}
ObjectRef hide(Object* ref){ObjectRef r = {ref};return r;}
VectorRef hide(Vector* ref){VectorRef r = {ref};return r;}


List* reveal_list(Value& v) {return reveal(v.value.list);}


///// Masp::Env //////

class Masp::Env
{
public:

    typedef glh::PMapPool<std::string, Value>      value_map_pool;
    typedef glh::PMapPool<std::string, Value>::Map value_map;

    Masp::Env():env_pool_()
    {
        env_stack_.push(env_pool_.new_map());
    }

    List new_list(){return list_pool_.new_list();}

    List* new_list_alloc()
    {
        return new List(list_pool_.new_list());
    }

    ListPool& list_pool(){return list_pool_;}

    size_t reserved_size_bytes()
    {
        return list_pool_.reserved_size_bytes();
    }

    size_t live_size_bytes()
    {
        return list_pool_.live_size_bytes();
    }

    void gc()
    {
        env_pool_.gc();
        list_pool_.gc();
    }

    value_map_pool        env_pool_;
    ListPool              list_pool_;

    std::stack<value_map> env_stack_;
};

// Value

void append_to_value_stl_list(std::list<Value>& value_list, const Value& v)
{
    value_list.push_back(v);
}


void append_to_value_list(Value& container, const Value& v)
{
    if(container.type != LIST) assert("append_to_value_list error: container not list.");

    List* list = reveal_list(container);
    *list = list->add(v);
}


Value make_value_number(const Number& num)
{
    Value a;
    a.value.number.set(num);
    a.type = NUMBER;
    return a;
}

Value make_value_number(int i)
{
    Value a;
    a.value.number.set(i);
    a.type = NUMBER;
    return a;
}

Value make_value_number(double d)
{
    Value a;
    a.value.number.set(d);
    a.type = NUMBER;
    return a;
}

Value make_value_string(const char* str)
{
    Value a;
    a.type = STRING;
    a.alloc_str(str);
    return a;
}

Value make_value_string(const char* str, const char* str_end)
{
    Value a;
    a.type = STRING;
    a.alloc_str(str, str_end);
    return a;
}


Value make_value_symbol(const char* str)
{
    Value a;
    a.type = SYMBOL;
    a.alloc_str(str);
    return a;
}

Value make_value_symbol(const char* str, const char* str_end)
{
    Value a;
    a.type = SYMBOL;
    a.alloc_str(str, str_end);
    return a;
}

Value make_value_list(Masp& m)
{
    Value a;
    a.type = LIST;
    List* list_ptr = m.env()->new_list_alloc();
    a.value.list = hide(list_ptr);
    return a;
}

Value make_value_map()
{
    Value a;
    a.type = MAP;
    a.value.map = hide(new Map);
    return a;
}

Value make_value_closure()
{
    Value a;
    a.type = CLOSURE;
    a.value.closure = hide(new Closure);
    return a;
}

Value make_value_object()
{
    Value a;
    a.type = OBJECT;
    a.value.object = hide(new Object);
    return a;
}

Value make_value_vector()
{
    Value a;
    a.type = VECTOR;
    a.value.vector = hide(new Vector);
    return a;
}

Value::Value()
{
    type = LIST;
    value.list.data = 0;
}


void Value::dealloc()
{
    if((type == STRING || type == SYMBOL) && value.string)
    {
        free(((void*)value.string));
    }
    else if(type == LIST && value.list.data){ delete reveal(value.list);}
    else if(type == MAP && value.map.data){ delete reveal(value.map);}
    else if(type == CLOSURE && value.closure.data){ delete reveal(value.closure);}
    else if(type == OBJECT && value.object.data){ delete reveal(value.object);}
    else if(type == VECTOR && value.vector.data){ delete reveal(value.vector);}
}

Value::~Value()
{
    dealloc();
}

template<class T, class V>
V copy_t(const V& v)
{
    T* t = 0;
    if(v.data) t = new T(*reveal(v));
    return hide(t);
}

Value::Value(const Value& v)
{
    copy(v);
}

void Value::copy(const Value& v)
{
    type = v.type;
    switch(type)
    {
        case NUMBER:
        {
            value.number.set(v.value.number);
            break;
        }
        case SYMBOL:
        case STRING:
        {
            alloc_str(v.value.string);
            break;
        }
        case LIST:
        {
            value.list = copy_t<List, ListRef>(v.value.list);
            break;
        }
        case MAP:
        {
            value.map = copy_t<Map, MapRef>(v.value.map);
            break;
        }
        case CLOSURE:
        {
            value.closure = copy_t<Closure, ClosureRef>(v.value.closure);
            break;
        }
        case OBJECT:
        {
            value.object = copy_t<Object, ObjectRef>(v.value.object);
            break;
        }
        case VECTOR:
        {
            value.vector = copy_t<Vector, VectorRef>(v.value.vector);
            break;
        }
        default:
            assert(!"Illegal Value type");
    }
}

void Value::movefrom(Value& v)
{
    type = v.type;
    
    switch(type)
    {
        case NUMBER:
        {
            value.number.set(v.value.number);
            break;
        }
        case SYMBOL:
        case STRING:
        {
            value.string = v.value.string;
            v.value.string = 0;
            break;
        }
        case LIST:
        {
            value.list = v.value.list;
            v.value.list.data = 0;
            break;
        }
        case MAP:
        {
            value.map = v.value.map;
            v.value.map.data = 0;
            break;
        }
        case CLOSURE:
        {
            value.closure = v.value.closure;
            v.value.closure.data = 0;
            break;
        }
        case OBJECT:
        {
            value.object = v.value.object;
            v.value.object.data= 0;
            break;
        }
        case VECTOR:
        {
            value.vector = v.value.vector;
            v.value.object.data= 0;
            break;
        }
        default:
            assert(!"Illegal Value type");
    }
}

Value::Value(Value&& v)
{
    movefrom(v);
}

Value& Value::operator=(const Value& a)
{
    if(&a != this)
    {
        copy(a);
    }
    
    return *this;
}

Value& Value::operator=(Value&& v)
{
    if(&v == this)
    {
        movefrom(v);
    }

    return *this; 
}

void Value::alloc_str(const char* str)
{
    size_t size = strlen(str) + 1;
    value.string = (const char*) malloc(size * sizeof(char));
    strcpy(((char*)value.string), str);
}

void Value::alloc_str(const char* str, const char* str_end)
{
    size_t size = str_end - str + 1;
    value.string = (const char*) malloc(size * sizeof(char));
    ((char*)value.string)[0] = '\0';
    strncat(((char*)value.string), str, size - 1);

    ((char*)value.string)[size -1] = '\0';
}

inline List* value_list(Value& v){return reveal(v.value.list);}

///// Utility functions /////

static inline bool is_digit(char c){return isdigit(c) != 0;}
static inline bool is_space(char c){return isspace(c) != 0;}

const char* g_delimiters = "(){}[];'";

enum DelimEnum{
    LEFT_PAREN    = 0,
    RIGHT_PAREN   = 1,
    LEFT_BRACE    = 2,
    RIGHT_BRACE   = 3,
    LEFT_BRACKET  = 4,
    RIGHT_BRACKET = 5,
    SEMICOLON     = 6,
    QUOTE         = 7
};

std::regex g_regfloat("^([-+]?[0-9]+(\\.[0-9]*)?|\\.[0-9]+)([eE][-+]?[0-9]+)?$");
std::regex g_regint("^(?:([-+]?[1-9][0-9]*)|(0[xX][0-9A-Fa-f]+)|0[bB]([01]+))$");

typedef enum ParseResult_t{PARSE_NIL, PARSE_INT, PARSE_FLOAT} ParseResult;

ParseResult parsenum(const char* num, const char* numend, int& intvalue, double& doublevalue)
{
    std::cmatch res; 
    ParseResult result = PARSE_NIL;

    if(std::regex_search(num, numend, res, g_regint))
    {
        std::string is = res[0].str();

        if(is[0] == '0')
        {
            if(is[1] == 'x' ||is[1] == 'X') intvalue = strtol(is.c_str(), 0, 16);
            else if(is[1] == 'b' || is[1] == 'B') intvalue = strtol(res[3].str().c_str(), 0, 2); 
        }
        else
        {
            intvalue = atoi(is.c_str());
        }
        result = PARSE_INT;
    }
    else if(std::regex_search(num, numend, res, g_regfloat))
    {
        std::string sf = res[0].str();
        doublevalue = atof(sf.c_str());
        result = PARSE_FLOAT;
    }
    return result;

}

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

/** Look for the character in the given null terminated string. If value found return index of character in string. */
static int pos_in_string(const char c, const char* str)
{
    const char* ptr = str;
    while(c != *ptr && *(++ptr)){}
    if(*ptr) return ptr - str;
    else return - 1;
}

static bool is_in_string(const char c, const char* str)
{
    return pos_in_string(c, str) >= 0;
}

static bool is_delimiter(const char c)
{
    return is_in_string(c, g_delimiters);
}


/** Verify that given scope delimiters balance out.*/
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

class ParseException
{
public:

    ParseException(const char* msg):msg_(msg){}
    ~ParseException(){}

    std::string get_message(){return msg_;}

    std::string msg_;
};

/** Parse string to atom. Use one instance of AtomParser per string/atom pair. */
class ValueParser
{
public:

    Masp& masp_;

    ValueParser(Masp& masp):masp_(masp)
    {
        reading_string = false;
    }

    bool reading_string;

    // TODO: Write explanation of the parsing sequence
    //
    // a. Unless reading string symbols are broken on ', whitespace, (, ) and ;;
    // b. if reading string string is broken only when reading matching quote as on start
    //    \- will escape the following quote

    typedef const char* charptr;

    charptr c_;
    charptr end_;

    bool is(char ch){return *c_ == ch;}

    bool at_end(){return c_ >= end_;}

    void init(const char* start, const char* end){c_ = start; end_ = end;}

    const char* next(){return c_ + 1;}

    void move_forward(){c_ = c_ + 1;}

    void set(const char* chp){c_ = chp;}

    void to_newline()
    {
        while(c_ != end_ && *c_ != '\n'){c_++;}
    }

    const char* parse_string()
    {
        return last_quote_of_string(next(),end_);
    }

    bool parse_number(Number& out)
    {
        bool result = false;

        const char* c = c_;
        bool is_prefix = (*c == '+' || *c == '-');
        bool next_is_num = (c_ != end_ && (c_ + 1) != end_) && is_digit(c[1]);

        const char* begin = c_;
        const char* end;

        if(is_digit(*c) || (is_prefix && next_is_num))
        {
            int intvalue;
            double floatvalue;

            end = c_ + 1;
            while(!is_space(*end) && (! is_delimiter(*end)) && *end) end++;

            ParseResult r = parsenum(begin, end, intvalue, floatvalue);
            result = r != PARSE_NIL;

            if(r == PARSE_INT) out.set(intvalue);
            else if(r == PARSE_FLOAT) out.set(floatvalue);
        }

        if(result) c_ = end - 1;

        return result;
    }

    bool parse_symbol(const char** begin, const char** end)
    {
        bool result = false;
        *begin = c_;
        const char* last = c_ + 1;
        while(!is_space(*last) && (! is_delimiter(*last)) && *last) last++;
        *end = last;
        result = true;
        return result;
    }

    Value get_value()
    {
        Number tmp_number;
        const char* tmp_string_begin;
        const char* tmp_string_end;

        if(is('"')) //> string
        {
            const charptr first = c_ + 1;
            const charptr last = parse_string();
            set(last + 1);
            return make_value_string(first, last);
        }
        else if(is('(')) // Enter list
        {
            Value result = make_value_list(masp_);
            move_forward();
            recursive_parse(result);
            return result;
        }
        else if(parse_number(tmp_number))
        {
            move_forward();
            return make_value_number(tmp_number);
        }
        else if(parse_symbol(&tmp_string_begin, &tmp_string_end))
        {
            set(tmp_string_end);
            return make_value_symbol(tmp_string_begin, tmp_string_end);
        }
        else
        {
            throw ParseException("get_value: Masp parse error.");
            return Value();
        }
    }

    void recursive_parse(Value& root)
    {
        if(root.type != LIST) throw ParseException("recursive_parse: root type is not LIST.");

        std::list<Value> build_list;

        while(! at_end())
        {
            if(is(';'))  //> Quote, go to newline
            {
                to_newline();
                if(c_ != end_) move_forward();
            }
            else if(is(')')) // Exit list
            {
                move_forward();
                break;
            }
            else if(is_space(*c_))
            {
                move_forward();
                // Skip whitespace
                // After this we know that the input is either number or symbol
            }
            else
            {
                // append to root  
                // push_to_value(root, get_value());
                Value v = get_value();
                append_to_value_stl_list(build_list, v);
            }

            if(at_end()) break;
        }

        List* list_ptr = value_list(root);
        *list_ptr = masp_.env()->list_pool().new_list(build_list);
    }

    parser_result parse(const char* str)
    {
        size_t size = strlen(str);
        init(str, str + size);

        bool scope_valid = check_scope(c_, end_, ";", '(', ')');
        if(!scope_valid)
            return parser_result("Could not parse, error in list scope - misplaced '(' or ')' ");
        
        scope_valid = check_scope(c_, end_, ";", '[', ']');
        if(!scope_valid)
            return parser_result("Could not parse, error in vector scope - misplaced '[' or ']' ");
        
        scope_valid = check_scope(c_, end_, ";", '{', '}');
        if(!scope_valid)
            return parser_result("Could not parse, error in map scope - misplaced '{' or '}' ");

        Value root = make_value_list(masp_);

        try{
            recursive_parse(root);
        }
        catch(ParseException& e){
            return parser_result(e.get_message());
        }

        return parser_result(root);
    }

};

parser_result string_to_value(Masp& m, const char* str)
{
    ValueParser parser(m);

    return parser.parse(str);
}

typedef const char* (*PrefixHelper)(const Value& v);

static void value_to_string_helper(std::ostream& os, const Value& v, PrefixHelper prfx)
{
    auto out = [&]()->std::ostream& {
        if(prfx) os << prfx(v) << " ";
        return os;
    };

    switch(v.type)
    {
        case NUMBER:
        {
            if(v.value.number.type == Number::INT)
            {
                out() << v.value.number.to_int() << " ";
            }
            else
            {
                out() << v.value.number.to_float() << " ";
            }
            break;
        }
        case SYMBOL:
        {
            out() << v.value.string << " ";
            break;
        }
        case STRING:
        {
            out() << "\"" << v.value.string << "\" ";
            break;
        }
        case LIST:
        {
            List* lst_ptr = reveal(v.value.list);
            out() << "(";
            for(auto i = lst_ptr->begin(); i != lst_ptr->end(); ++i)
            {
                value_to_string_helper(os, *i, prfx);
            }
            os << ")";
            break;
        }
        default:
        {
            assert(!"Implement output for type");
        }
    }
}

const std::string value_to_string(const Value& v)
{
    std::ostringstream stream;
    value_to_string_helper(stream, v, 0);
    return stream.str();
}

const std::string value_to_typed_string(const Value& v)
{
    std::ostringstream stream;
    value_to_string_helper(stream, v, value_type_to_string);
    return stream.str();
}


const char* value_type_to_string(const Value& v)
{
    switch(v.type)
    {
        case NUMBER:
        {
            if(v.value.number.type == Number::INT) return "NUMBER:INT";
            else                                   return "NUMBER:FLOAT";
        }
        case STRING: return "STRING";
        case SYMBOL: return "SYMBOL";
        case CLOSURE: return "CLOSURE";
        case VECTOR: return "VECTOR";
        case LIST: return "LIST";
        case MAP: return "MAP";
        case OBJECT: return "OBJECT";
    }
    return "";
}

///// Evaluation utilities /////



////// Masp ///////

Masp::Masp()
{
    env_ = new Env();
}

Masp::~Masp()
{
    delete env_;
}

Masp::Env* Masp::env(){return env_;}

void Masp::gc(){env_->gc();}

size_t Masp::reserved_size_bytes(){return env_->reserved_size_bytes();}

size_t Masp::live_size_bytes(){return env_->live_size_bytes();}

} // Namespace masp ends
