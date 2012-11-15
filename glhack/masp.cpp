#include "masp.h"
#include "persistent_containers.h"

#include<stack>
#include<cstring>
#include<algorithm>
#include<cstdlib>
#include<cctype>
#include<regex>
#include<sstream>
#include<functional>
#include<numeric>
#include<tuple>
#include<utility>

namespace masp{



////// Opaque value wrappers. ///////

// define hash function for value

class Value;

class ValuesAreEqual { public:
    static bool compare(const Value& k1, const Value& k2);
};

class ValueHash { public:
    static uint32_t hash(const Value& h);
};


typedef glh::PMapPool<Value, Value, ValuesAreEqual, ValueHash> MapPool;
typedef MapPool::Map   Map;

typedef glh::PMapPool<std::string, Value>      env_map_pool;
typedef glh::PMapPool<std::string, Value>::Map env_map;

typedef glh::PListPool<Value>              ListPool;
typedef glh::PListPool<Value>::List        List;

typedef std::vector<Value>        VRefContainer;

typedef VRefContainer::const_iterator VRefIterator;
typedef std::function<Value(Masp& m, VRefIterator arg_start, VRefIterator arg_end)> ValueFunction;

typedef std::vector<Value> Vector;
typedef std::vector<Number> NumberArray;


bool all_are_float(NumberArray& arr)
{
    bool result = true;
    for(auto n = arr.begin(); n != arr.end() && result; ++n) result = n->type == Number::FLOAT;
    return result;
}

bool all_are_int(NumberArray& arr)
{
    bool result = true;
    for(auto n = arr.begin(); n != arr.end() && result; ++n) result = n->type == Number::INT;
    return result;
}

void convert_to_float(NumberArray& arr)
{
    for(auto n = arr.begin(); n != arr.end(); ++n){double d = n->to_float(); n->set(d);}
}

void convert_to_int(NumberArray& arr)
{
    for(auto n = arr.begin(); n != arr.end(); ++n){int i = n->to_int(); n->set(i);}
}


typedef int Object; // TODO
typedef int Lambda; // TODO


struct Closure{ // TODO
    Lambda lambda; 
    env_map env;
    Closure(const env_map& env_in):env(env_in){}
};

struct Function{ValueFunction fun;};

class Value
{
public:
    Type type;

    union
    {
        Number       number;
        std::string* string; //> Data for string | symbol
        List*        list;
        Map*         map;
        Closure*     closure;
        Object*      object;
        Vector*      vector;
        Function*    function;
        NumberArray* number_array;
        Lambda*      lambda;
    } value;

    Value();
    ~Value();
    Value(const Value& v);
    Value(Value&& v);
    Value& operator=(const Value& v);
    Value& operator=(Value&& v);
    void alloc_str(const char* str);
    void alloc_str(const std::string& str);
    void alloc_str(const char* str, const char* end);

    bool operator==(const Value& v) const;
    uint32_t get_hash() const;

private:
    void dealloc();
    void copy(const Value& v);
    void movefrom(Value& v);

};

// ValuesAreEqual and ValueHash member implementations
bool ValuesAreEqual::compare(const Value& k1, const Value& k2){return k1 == k2;} 
uint32_t ValueHash::hash(const Value& h){return h.get_hash();}

Value::Value()
{
    type = LIST;
    value.list = 0;
}

void Value::dealloc()
{
    if((type == STRING || type == SYMBOL) && value.string)
    {
        delete value.string;
    }
    else if(type == LIST && value.list)      { delete value.list;}
    else if(type == MAP && value.map)        { delete value.map;}
    else if(type == CLOSURE && value.closure){ delete value.closure;}
    else if(type == OBJECT && value.object)  { delete value.object;}
    else if(type == VECTOR && value.vector)  { delete value.vector;}
    else if(type == LAMBDA  && value.lambda)  { delete value.lambda;}
    else if(type == FUNCTION && value.function)  { delete value.function;}
    else if(type == NUMBER_ARRAY && value.number_array)  { delete value.number_array;}
}

Value::~Value()
{
    dealloc();
}

template<class V>
V* copy_new(const V* v)
{
    V* result = 0;
    if(v)
    {
        result = new V(*v);
    }
    return result;
}

Value::Value(const Value& v)
{
    copy(v);
}

void Value::copy(const Value& v)
{
    type = v.type;

#define COPY_PARAM_V(param_name) value.##param_name = copy_new(v.value.##param_name)
    if(type == NUMBER) value.number.set(v.value.number);
    else if(type == SYMBOL || type == STRING) COPY_PARAM_V(string);
    else if(type == LIST)    COPY_PARAM_V(list);
    else if(type == MAP)     COPY_PARAM_V(map);
    else if(type == CLOSURE) COPY_PARAM_V(closure);
    else if(type == OBJECT)  COPY_PARAM_V(object);
    else if(type == VECTOR)  COPY_PARAM_V(vector);
    else if(type == LAMBDA)  COPY_PARAM_V(lambda);
    else if(type == FUNCTION) COPY_PARAM_V(function);
    else if(type == NUMBER_ARRAY) COPY_PARAM_V(number_array);
    else {assert("Faulty param type.");}
#undef COPY_PARAM_V
}

void Value::movefrom(Value& v)
{
    type = v.type;
    void* that_value_ptr = reinterpret_cast<void*>(&v.value); 
    size_t value_size = sizeof(value);
    memcpy(reinterpret_cast<void*>(&value), that_value_ptr, sizeof(value));  
    memset(that_value_ptr, 0, value_size);
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
    if(&v != this)
    {
        movefrom(v);
    }

    return *this; 
}

void Value::alloc_str(const std::string& str)
{
    value.string = new std::string(str);
}

void Value::alloc_str(const char* str)
{
    value.string = new std::string(str);
}

void Value::alloc_str(const char* str, const char* str_end)
{
    value.string = new std::string(str, str_end);
}

bool Value::operator==(const Value& v) const
{
    if(v.type != type) return false;
    bool result = false;

    if(type == NUMBER)            result = value.number == v.value.number;
    else if(type == NUMBER_ARRAY) result = (*value.number_array) == (*v.value.number_array);
    else if(type == STRING || type == SYMBOL) result = (*value.string) == (*v.value.string);
    else if(type == CLOSURE)
    {
    }
    else if(type == VECTOR) result = (*value.vector) == *(v.value.vector);
    else if(type == LIST) result = (*(value.list) ==  *(v.value.list));
    else if(type == MAP) result = (*(value.map) == *(v.value.map));
    else if(type == OBJECT)
    {
        // TODO
    }
    else if(type == FUNCTION)
    {
        // TODO
    }
    else if(type == LAMBDA)
    {
        // TODO
    }

    return result;
}

uint32_t hash_of_number(const Number& n){return *((uint32_t*) &n);}
uint32_t accum_number_hash(const uint32_t& p, const Number& n){return p * hash_of_number(n);}
uint32_t accum_value_hash(const uint32_t& p, const Value& v){return p * v.get_hash();}

uint32_t Value::get_hash() const
{
    // For collections produce a product of the hashes of contained elements
    uint32_t h = 0;

    if(type == NUMBER)  h = hash_of_number(value.number);
    else if(type == NUMBER_ARRAY){
        uint32_t orig = 0;
        h = glh::fold_left<uint32_t, NumberArray>(orig, accum_number_hash, *value.number_array);
    }
    else if(type == STRING || type == SYMBOL) h = hash32(*value.string);
    else if(type == CLOSURE)
    {
        // TODO
    }
    else if(type == VECTOR)
    {
        uint32_t orig = 0;
        h = glh::fold_left<uint32_t, Vector>(orig, accum_value_hash, *value.vector);
    }
    else if(type == LIST) 
    {
        uint32_t orig = 0;
        h = glh::fold_left<uint32_t, List>(orig, accum_value_hash, *value.list);
    }
    else if(type == MAP)
    {
        uint32_t accum = 0;
        Map::iterator i = value.map->begin();
        Map::iterator end = value.map->end();
        while(i != end)
        {
            accum = accum_value_hash(accum_value_hash(accum, i->first), i->second);
            ++i;
        }
        h = accum;
    }
    else if(type == OBJECT)
    {
        // TODO
    }
    else if(type == FUNCTION)
    {
        // TODO
    }
    else if(type == LAMBDA)
    {
        // TODO
    }

    return h;
}


void free_value(Value* v)
{
    if(v) delete v;
}

Value* new_value()
{
    return new Value();
}

inline List* value_list(Value& v){return v.value.list;}

void append_to_value_stl_list(std::list<Value>& value_list, const Value& v)
{
    value_list.push_back(v);
}

void append_to_value_list(Value& container, const Value& v)
{
    assert((container.type == LIST) && "append_to_value_list error: container not list.");

    List* list = value_list(container);
    *list = list->add(v);
}

///// Masp::Env //////

class Masp::Env
{
public:
    Masp::Env():env_pool_()
    {
        env_.reset(new env_map(env_pool_.new_map()));
        load_default_env();
    }

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
        map_pool_.gc();
        list_pool_.gc();
        env_pool_.gc();
    }

    void add_fun(const char* name, ValueFunction f)
    {
        //*env_ = env_->add(); // TODO
    }

    void load_default_env();

    // Locals
    MapPool             map_pool_;
    env_map_pool        env_pool_;
    ListPool            list_pool_;

    std::unique_ptr<env_map> env_;
};


inline List new_list(Masp& m){return m.env()->list_pool_.new_list();}

template<class Cont>
inline List new_list(Masp& m, const Cont& container){return m.env()->list_pool_.new_list(container);}

inline List* new_list_alloc(Masp& m)
{
    return new List(m.env()->list_pool_.new_list());
}

inline Map* new_map_alloc(Masp& m)
{
    return new Map(m.env()->map_pool_.new_map());
}

inline Map new_map(Masp& m)
{
    return m.env()->map_pool_.new_map();
}

inline env_map* new_env_map_alloc(Masp& m)
{
    return new env_map(m.env()->env_pool_.new_map());
}

inline env_map new_env_map(Masp& m)
{
    return m.env()->env_pool_.new_map();
}


// Value factories

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
    List* list_ptr = new_list_alloc(m);
    a.value.list = list_ptr;
    return a;
}

Value* make_value_list_alloc(Masp& m)
{
    Value* a = new Value();
    a->type = LIST;
    List* list_ptr = new_list_alloc(m);
    a->value.list = list_ptr;
    return a;
}

Value make_value_map(Masp& m)
{
    Value a;
    a.type = MAP;
    Map* map_ptr = new_map_alloc(m);
    a.value.map = map_ptr;
    return a;
}

Value make_value_closure(Masp& m)
{
    // TODO - pass lambda and current env as parameters ?
    Value a;
    a.type = CLOSURE;
    a.value.closure = new Closure(new_env_map(m)); // TODO - use current env? Eval env?
    return a;
}

Value make_value_function(ValueFunction f)
{
    Value v;
    v.type = FUNCTION;
    v.value.function = new Function();
    v.value.function->fun = f;
    return v;

}

Value make_value_object(Masp& m)
{
    // TODO
    Value a;
    a.type = OBJECT;
    a.value.object = new Object();
    return a;
}

Value make_value_vector()
{
    // TODO ?
    Value a;
    a.type = VECTOR;
    a.value.vector = new Vector;
    return a;
}

Value make_value_number_array()
{
    // TODO ?
    Value a;
    a.type = NUMBER_ARRAY;
    a.value.number_array = new NumberArray();
    return a;
}



//////////// Native operators ////////////
namespace {
    Value op_add(Masp& m, VRefIterator arg_start, VRefIterator arg_end)
    {
        Number n;
        n.set(11);
        return make_value_number(n);
    }

    Value op_define(Masp& m, VRefIterator arg_start, VRefIterator arg_end)
    {
        return Value();
    }
}

//////////// Load environment ////////////

void Masp::Env::load_default_env()
{
    add_fun("+", op_add);
}

///// Utility functions /////

static inline bool is_digit(char c){return isdigit(c) != 0;}
static inline bool is_space(char c){return (isspace(c) != 0) || (c == ',');}

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

struct ScopeError
{
    /* If scope left open, return character of scope opening and number of line where scope was opened.
       If scope scoped pre-emptively, return line of error and and the faulty closing symbol.
    */
    enum Result{SCOPE_LEFT_OPEN, FAULTY_SCOPE_CLOSING, OK};
    int line;
    char scope;
    Result result;
    ScopeError():result(OK){}
    ScopeError(Result res,char c, int l):result(res), line(l), scope(c){}

    bool success(){return result == OK;}

    std::string report()
    {
        if(result == OK) return std::string("Scope ok");
        else if(result == SCOPE_LEFT_OPEN)
        {
            std::ostringstream os;
            os << "Scope "  << scope << " at line " << line << " not closed.";
            return os.str();
        }
        else if(result == FAULTY_SCOPE_CLOSING)
        {
            std::ostringstream os;
            os << "Premature scope closing "  << scope << " at line " << line  << ".";
            return os.str();
        }

        return std::string("");
    }
};

/** Verify that given scope delimiters balance out.
 *  The scope start and scope end arrays must be of equal length and contain start and end characters symmetrical
 *  positions such that scope_start[i] and scope_end[i] contain a character pair for a particular scope.
 *  @param scope_start string of scope begin characters.
 *  @param scope_end   string of scope end characters.
 *  @return ScopeError containing the result of the check.
*/

static ScopeError check_scope(const char* begin, const char* end, const char* comment, const char* scope_start, const char* scope_end)
{
    using namespace std;

    bool result = true;
    const char* c = begin;
    int scope = 0;
    size_t comment_length = strlen(comment);
    int line_number = 0;

    std::stack<std::pair<int,int>> expected_closing; // Scope index; line number

    while(c < end)
    {

        if(match_string(comment, comment_length, c, end))
        {
            c = to_newline(c, end);
            line_number++;
        }

        int scope_start_index = pos_in_string(*c, scope_start);
        int scope_end_index = pos_in_string(*c, scope_end);

        if(*c == '"')
        {
            c = last_quote_of_string(c+1, end);
        }
        else if(scope_start_index >= 0)
        {
            expected_closing.push(std::make_pair(scope_start_index, line_number));
        }
        else if(scope_end_index >= 0)
        {
            if(expected_closing.size() > 0 && expected_closing.top().first == scope_end_index)
            {
                expected_closing.pop();
            }
            else return ScopeError(ScopeError::FAULTY_SCOPE_CLOSING, *c, line_number);
        }

        if(scope < 0)
        {
            break;
        }

        c++;
    }

    if(expected_closing.size() > 0)
    {
        int scp; int line;
        std::tie(scp, line) = expected_closing.top();
        return ScopeError(ScopeError::SCOPE_LEFT_OPEN, scope_start[scp],line);
    }

    return ScopeError();
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
        *list_ptr = new_list(masp_, build_list);
    }

    parser_result parse(const char* str)
    {
        size_t size = strlen(str);
        init(str, str + size);

        ScopeError scope_result = check_scope(c_, end_, ";", "({[", ")}]");

        if(!scope_result.success())
        {
            return parser_result(scope_result.report());
        }

        Value* root = make_value_list_alloc(masp_);

        try{
            recursive_parse(*root);
        }
        catch(ParseException& e){
            return parser_result(e.get_message());
        }

        return parser_result(ValuePtr(root, ValueDeleter()));
    }

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

Masp::Env* Masp::env(){return env_;}

void Masp::gc(){env_->gc();}

size_t Masp::reserved_size_bytes(){return env_->reserved_size_bytes();}

size_t Masp::live_size_bytes(){return env_->live_size_bytes();}



///// Evaluation utilities /////

parser_result string_to_value(Masp& m, const char* str)
{
    ValueParser parser(m);

    return parser.parse(str);
}

typedef std::string (*PrefixHelper)(const Value& v);

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
            out() << *(v.value.string) << " ";
            break;
        }
        case STRING:
        {
            out() << "\"" << *(v.value.string) << "\" ";
            break;
        }
        case LIST:
        {
            List* lst_ptr = v.value.list;
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

std::string value_type_to_string(const Value& v);

const std::string value_to_typed_string(const Value* v)
{
    std::ostringstream stream;
    value_to_string_helper(stream, *v, value_type_to_string);
    return stream.str();
}


std::string value_type_to_string(const Value& v)
{
    switch(v.type)
    {
        case NUMBER:
        {
            if(v.value.number.type == Number::INT) return std::string("NUMBER:INT");
            else                                   return std::string("NUMBER:FLOAT");
        }
        case STRING: return std::string("STRING");
        case SYMBOL: return std::string("SYMBOL");
        case CLOSURE: return std::string("CLOSURE");
        case VECTOR: return std::string("VECTOR");
        case LIST: return std::string("LIST");
        case MAP: return std::string("MAP");
        case OBJECT: return std::string("OBJECT");
    }
    return "";
}

const Value* lookup(Masp& masp, const std::string& name)
{
    glh::ConstOption<Value> res = masp.env()->env_->try_get_value(name);

    return res.get(); // TODO
}

class EvaluationException
{
public:

    EvaluationException(const char* msg):msg_(msg){}
    EvaluationException(const std::string& msg):msg_(msg){}
    ~EvaluationException(){}

    std::string get_message(){return msg_;}

    std::string msg_;
};


class Evaluator
{
public:

    Masp& masp_;

    std::vector<Value> value_stack;

    std::vector<Value> ref_stack;
    
    std::vector<Value> tmp_values;
    std::vector<Value> apply_list;


    Evaluator(Masp& masp): masp_(masp)
    {
    }

    Value eval(const Value& v)
    {
        if(v.type == NUMBER || v.type == STRING || v.type == MAP ||
           v.type == NUMBER_ARRAY || v.type == VECTOR || v.type == FUNCTION) return v;
        else if(v.type == SYMBOL)
        {
            const Value* r = lookup(masp_, *v.value.string);
            if(!r) throw EvaluationException(std::string("Symbol not found:") + *v.value.string);

            return *r;
        }
        else if(v.type == LIST)
        {
            // eval unless previous was quote?
        }
        else if(v.type == LAMBDA)
        {
        }
        else if(v.type == CLOSURE)
        {
        }
        return Value();
    }

    void apply(Value& v)
    {
    }

};

ValuePtr eval(Masp& m, const Value& v)
{
    Evaluator e(m);

    Value* vptr = new Value(e.eval(v));

    // value - if not in place, add to value_stack, add reference to ref_stack
    // add value references to eval stack
    return ValuePtr(vptr, ValueDeleter());
}

} // Namespace masp ends
