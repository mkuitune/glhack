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
#include<limits>


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

typedef glh::PListPool<Value>              ListPool;
typedef glh::PListPool<Value>::List        List;

typedef std::vector<Value>        VRefContainer;

//typedef VRefContainer::const_iterator VRefIterator;
typedef List::iterator VRefIterator;
typedef std::function<Value(Masp& m, VRefIterator arg_start, VRefIterator arg_end, Map& env)> ValueFunction;

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

typedef int Object; // TODO - is just tagged list?
typedef int Lambda; // TODO - is just tagged list?

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
        Object*      object;
        Vector*      vector;
        Function*    function;
        NumberArray* number_array;
        Lambda*      lambda;
        bool         boolean;
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
    bool is_nil() const;
    bool is(const Type t) const{return type == t;}
    bool operator==(const Value& v) const;
    uint32_t get_hash() const;

    void movefrom(Value& v);


private:
    void dealloc();
    void copy(const Value& v);

};

// ValuesAreEqual and ValueHash member implementations
bool ValuesAreEqual::compare(const Value& k1, const Value& k2){return k1 == k2;} 
uint32_t ValueHash::hash(const Value& h){return h.get_hash();}

Value::Value():type(NIL){}

void Value::dealloc()
{
    if((type == STRING || type == SYMBOL) && value.string)
    {
        delete value.string;
    }
    else if(type == LIST && value.list)      { delete value.list;}
    else if(type == MAP && value.map)        { delete value.map;}
    else if(type == OBJECT && value.object)  { delete value.object;}
    else if(type == VECTOR && value.vector)  { delete value.vector;}
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
    else if(type == OBJECT)  COPY_PARAM_V(object);
    else if(type == VECTOR)  COPY_PARAM_V(vector);
    else if(type == FUNCTION) COPY_PARAM_V(function);
    else if(type == NUMBER_ARRAY) COPY_PARAM_V(number_array);
    else if(type == BOOLEAN) value.boolean = v.value.boolean;
    else if(type != NIL)
        {assert("Faulty param type.");}
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

bool Value::is_nil() const {return type == NIL;}

bool Value::operator==(const Value& v) const
{
    if(v.type != type) return false;
    bool result = false;

    if(type == NUMBER)            result = value.number == v.value.number;
    else if(type == NUMBER_ARRAY) result = (*value.number_array) == (*v.value.number_array);
    else if(type == STRING || type == SYMBOL) result = (*value.string) == (*v.value.string);
    else if(type == VECTOR) result = (*value.vector) == *(v.value.vector);
    else if(type == LIST) result = (*(value.list) ==  *(v.value.list));
    else if(type == MAP) result = (*(value.map) == *(v.value.map));
    else if(type == OBJECT)
    {
        // TODO - what to do.
    }
    else if(type == FUNCTION)
    {
        // TODO - what to do
    }
    else if(type == BOOLEAN) result = value.boolean == v.value.boolean;
    else if(type == NIL) result = true;

    return result;
}

uint32_t hash_of_number(const Number& n){return *((uint32_t*) &n);}
uint32_t accum_number_hash(const uint32_t& p, const Number& n){return p * hash_of_number(n);}
uint32_t accum_value_hash(const uint32_t& p, const Value& v){return p * v.get_hash();}

uint32_t Value::get_hash() const
{
    // For collections produce a product of the hashes of contained elements
    uint32_t h = 0;

    if(type == BOOLEAN) h = (uint32_t) value.boolean;
    else if(type == NIL) h = std::numeric_limits<uint32_t>::max();
    else if(type == NUMBER)  h = hash_of_number(value.number);
    else if(type == NUMBER_ARRAY){
        uint32_t orig = 0;
        h = glh::fold_left<uint32_t, NumberArray>(orig, accum_number_hash, *value.number_array);
    }
    else if(type == STRING || type == SYMBOL) h = hash32(*value.string);
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

inline List* value_list(const Value& v){return v.type == LIST ? v.value.list : 0;}

inline const Value* value_list_first(const Value& v)
{
    const Value* result = 0;
    List* l = value_list(v);
    if(l && !l->empty()) result = l->begin().data_ptr();
    return result;
}

inline const Value* value_list_nth(const Value& v, size_t n)
{
    const Value* result = 0;
    List* l = value_list(v);

    if(l && !l->empty()){ 
        auto i = l->begin();
        auto e = l->end();
        while(i != e && n > 0) {++i; --n;}
        if(i != e) result = i.data_ptr();
    }
    return result;
}

inline const Value* value_list_second(const Value& v)
{
    return value_list_nth(v, 1);
}

inline const Value* value_list_third(const Value& v)
{
    return value_list_nth(v, 2);
}

inline Vector* value_vector(Value& v){return v.type == VECTOR ? v.value.vector : 0;}

inline NumberArray* value_number_array(Value& v){return v.type == NUMBER_ARRAY ? v.value.number_array : 0;}

inline Map* value_map(const Value& v){return v.type == MAP ? v.value.map : 0;}

void append_to_value_stl_list(std::list<Value>& ext_value_list, const Value& v)
{
    ext_value_list.push_back(v);
}

///// Masp::Env //////

class Masp::Env
{
public:
    Masp::Env()
    {
        env_.reset(new Map(map_pool_.new_map()));
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
        // TODO: fix gc. Now does not collect nested lists on first gc
        // i.e nested list that's alone ((1 2 3)) has it's inner list still referred to when 
        // collecting. Is this a problem or not...
        //
        map_pool_.gc();
        list_pool_.gc();
    }

    void add_fun(const char* name, ValueFunction f)
    {
        //*env_ = env_->add(); // TODO
    }

    void load_default_env();

    Map& get_env(){return *env_;}

    // Locals
    MapPool             map_pool_;
    ListPool            list_pool_;
    std::unique_ptr<Map> env_;
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

Value make_value_list(const List& oldlist)
{
    Value a;
    a.type = LIST;
    List* list_ptr = new List(oldlist);
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

Value make_value_map(Map& oldmap)
{
    Value a;
    a.type = MAP;
    Map* map_ptr = new Map(oldmap);
    a.value.map = map_ptr;
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

Value make_value_boolean(bool b)
{
    Value v;
    v.type = BOOLEAN;
    v.value.boolean = b;
    return v;
}

bool type_is(Value& v, Type t){return v.type == t;}

//////////// Native operators ////////////
namespace {
    Value op_add(Masp& m, VRefIterator arg_start, VRefIterator arg_end, Map& env)
    {
        Number n;
        n.set(11);
        return make_value_number(n);
    }

    Value op_sub(Masp& m, VRefIterator arg_start, VRefIterator arg_end, Map& env)
    {
        Number n;
        n.set(11);
        return make_value_number(n);
    }

    Value op_mul(Masp& m, VRefIterator arg_start, VRefIterator arg_end, Map& env)
    {
        Number n;
        n.set(11);
        return make_value_number(n);
    }

    Value op_div(Masp& m, VRefIterator arg_start, VRefIterator arg_end, Map& env)
    {
        Number n;
        n.set(11);
        return make_value_number(n);
    }

    // def'd values are immutable - fails if symbol has been already defined
    Value op_def(Masp& m, VRefIterator arg_start, VRefIterator arg_end, Map& env)
    {
        return Value();
    }
   
    // Sets a value. Defines it first if it has not been defined. 
    Value op_set(Masp& m, VRefIterator arg_start, VRefIterator arg_end, Map& env)
    {
        return Value();
    }

    // first, next, eq, cons, cond. 
    // TODO: first next ffirst fnext while < > + - * / dot cross
    // map filter range apply count zip

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

static bool equal(const char* str_1, const char* str_2)
{
    return (strcmp(str_1, str_2) == 0);
}

static bool string_value_is(const Value& v, const char* str)
{
    return (v.type == STRING) ? (strcmp(v.value.string->c_str(), str) == 0) : false;
}

static bool symbol_value_is(const Value& v, const char* str)
{
    return (v.type == SYMBOL) ? (strcmp(v.value.string->c_str(), str) == 0) : false;
}

static bool match_range(const char* begin, const char* end, const char*str)
{
    while(*str && begin != end && *str == *begin){str++;begin++;}
    if(*str || begin != end) return false;
    else return true;
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
        else if(is('[')) // Enter vector
        {
            Value result = make_value_vector();
            move_forward();
            recursive_parse(result);
            return result;
        }
        else if(is('{')) // Enter map
        {
            Value result = make_value_map(masp_);
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
            if(match_range(tmp_string_begin, tmp_string_end, "nil")) return Value(); 
            else if(match_range(tmp_string_begin, tmp_string_end, "true")) return make_value_boolean(true);
            else if(match_range(tmp_string_begin, tmp_string_end, "false")) return make_value_boolean(false);
            else return make_value_symbol(tmp_string_begin, tmp_string_end);
        }
        else
        {
            throw ParseException("get_value: Masp parse error.");
            return Value();
        }
    }

    void recursive_parse(Value& root)
    {
        if(glh::none_of(root.type ,LIST , VECTOR, MAP))
            throw ParseException("recursive_parse: root type is not container.");

        std::list<Value> build_list;

        bool next_is_quoted = false;;

        while(! at_end())
        {
            if(is(';'))  //> Comment, go to newline
            {
                to_newline();
                if(c_ != end_) move_forward();
            }
            else if(is(')') || is(']') || is('}')) // Exit container - scope was checked before this
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
            else if(is('\''))
            {
                next_is_quoted = true;
                move_forward();
            }
            else
            {
                // append to root  
                // push_to_value(root, get_value());
                Value v = get_value();
                if(next_is_quoted)
                {
                    Value quote_sym = make_value_symbol("quote");
                    Value outer = make_value_list(masp_);
                    *(outer.value.list) = outer.value.list->add(v);
                    *(outer.value.list) = outer.value.list->add(quote_sym);
                    append_to_value_stl_list(build_list, outer);
                    next_is_quoted = false;
                }
                else
                {
                    append_to_value_stl_list(build_list, v);
                }
            }

            if(at_end()) break;
        }

        if(next_is_quoted) // Quoted flag was not used.
        {
            throw ParseException("Quote cannot be empty.");
        }

        if(type_is(root, LIST))
        {
            List* list_ptr = value_list(root);
            *list_ptr = new_list(masp_, build_list);
        }
        else if(type_is(root, VECTOR))
        {
            Vector* vec_ptr = value_vector(root);
            size_t size = build_list.size();
            vec_ptr->resize(size);
            std::list<Value>::iterator li = build_list.begin();
            for(Vector::iterator i = vec_ptr->begin(); i != vec_ptr->end(); ++i)
                i->movefrom(*li++);
        } else if(type_is(root, MAP))
        {
            Map* map = value_map(root);
            std::list<Value>::iterator lend = build_list.end();
            for(std::list<Value>::iterator li = build_list.begin(); li != lend; ++li)
            {
                Value* key = &*li;
                ++li;
                if(li != lend)
                {
                    *map = map->add(*key, *li);
                }
                else
                {
                    *map = map->add(*key, make_value_list(masp_));
                    break;
                }
            }
        }
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
        case NIL:{out() << "nil"; break;} // TODO: Do we need to print nill?
        case BOOLEAN:
        {
            if(v.value.boolean) out() << "true";
            else out() << "false";
            break;
        }
        case NUMBER:
        {
            if(v.value.number.type == Number::INT)
            {
                out() << v.value.number.to_int();
            }
            else
            {
                out() << v.value.number.to_float();
            }
            break;
        }
        case SYMBOL:
        {
            out() << *(v.value.string);
            break;
        }
        case STRING:
        {
            out() << "\"" << *(v.value.string) << "\"";
            break;
        }
        case LIST:
        {
            out() << "(";
            List* lst_ptr = v.value.list;
            for(auto i = lst_ptr->begin(); i != lst_ptr->end(); ++i)
            {
                value_to_string_helper(os, *i, prfx);
                out() << " ";
            }
            os << ")";
            break;
        }
        case MAP:
        {
            Map* map_ptr = v.value.map;
            out() << "{";
            auto mend = map_ptr->end();
            for(auto m = map_ptr->begin(); m != mend; ++m)
            {
                value_to_string_helper(os, m->first, prfx);
                out() << " ";
                value_to_string_helper(os, m->second, prfx);
                out() << " ";
            }
            os << "}";
            break;
        }
        case VECTOR:
        {
            Vector* vec_ptr = v.value.vector;
            out() << "[";
            for(auto i = vec_ptr->begin(); i != vec_ptr->end(); ++i)
            {
                value_to_string_helper(os, *i, prfx);
                out() << " ";
            }
            os << "]";
            break;
        }
        // TODO: Number array, lambda, function, object
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
        case BOOLEAN: return std::string("BOOLEAN");
        case NIL: return std::string("");
        case SYMBOL: return std::string("SYMBOL");
        case VECTOR: return std::string("VECTOR");
        case LIST: return std::string("LIST");
        case MAP: return std::string("MAP");
        case OBJECT: return std::string("OBJECT");
    }
    return "";
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

// Evaluation utils

namespace {

    Value eval(const Value& v, Map& env, Masp& masp);
    Value apply(const Value& v, VRefIterator args_begin, VRefIterator args_end, Map& env, Masp& masp);

    bool is_self_evaluating(const Value& v)
    {
        return v.type == NUMBER || v.type == STRING || v.type == MAP ||
            v.type == NUMBER_ARRAY || v.type == VECTOR || v.type == FUNCTION;
    }

    bool is_tagged_list(const Value& v, const char* symname)
    {
        bool result = false;
        const Value* first = value_list_first(v);
        if(first) result = symbol_value_is(*first, symname);
        return result;
    }

    bool is_quoted(const Value& v){ return is_tagged_list(v,"quote");}
    bool is_assignment(const Value& v){ return is_tagged_list(v,"def");}
    bool is_if(const Value& v){return is_tagged_list(v, "if");} 
    bool is_lambda(const Value& v){return is_tagged_list(v, "lambda");} 
    bool is_begin(const Value& v){return is_tagged_list(v, "begin");} 
    bool is_cond(const Value& v){return is_tagged_list(v, "cond");} 
    bool is_else(const Value& v){return is_tagged_list(v, "else");}
    bool is_application(const Value& v){return v.is(LIST);}

    Value begin_actions(const Value& v)
    {
        List* vlist = value_list(v);
        if(vlist)
        {
            return make_value_list(vlist->rest());  
        }
        else
        {
            return Value();
        }
    }

    bool is_true(const Value& v)
    {
        bool v_is_false = v.type == NIL || (v.type == BOOLEAN && (!v.value.boolean));
        return !v_is_false;
    }

    const Value* assignment_var(const Value& v){return value_list_second(v);}
    const Value* assignment_value(const Value& v){return value_list_third(v);}
    
    Value eval_sequence(const List& expressions, Map& env, Masp& masp)
    {
        if(expressions.empty())
            throw EvaluationException(std::string("Trying to evaluate empty sequence"));

        if(!expressions.has_rest()) 
        {
            return eval(*expressions.first(), env, masp);
        }
        else
        {
            eval(*expressions.first(), env, masp);
            return eval_sequence(expressions.rest(), env, masp);
        }
    }

    Value sequence_exp(const List& action)
    {
        if(action.empty())
            return make_value_list(action);
        else if(!action.has_rest())
            return *action.first();
        else // make begin
            return make_value_list(action.add(make_value_symbol("begin")));
    }

    // cond: expand-clauses
    Value expand_clauses(const List& clauses, Masp& masp)
    {
        if(clauses.empty()) return make_value_symbol("false");
        else
        {
            const Value* first = clauses.first();
            List rest = clauses.rest();

            if(!first) throw EvaluationException(std::string("Error interpreting cond clause"));

            if(is_else(*first))
            {
                if(rest.empty())
                {
                    // Sequence-exp
                    List ca = value_list(*first)->rest(); // cond-actions
                    return sequence_exp(ca);
                }
                else
                {
                    throw EvaluationException(std::string("ELSE clause isn't last - COND->iF"));
                }
            }
            else
            {
                // make-if
                Value v_iflist = make_value_list(masp);

                List* iflist = value_list(v_iflist);

                const Value* pred = value_list_first(*first); 
                const List     ca = value_list(*first)->rest(); // cond-actions
                Value         seq = sequence_exp(ca);
                Value     clauses = expand_clauses(rest, masp);

                *iflist = iflist->add(clauses);
                *iflist = iflist->add(seq);
                *iflist = iflist->add(*pred);

                return v_iflist;
            }
        }
    }

    // cond: cond->if
    Value convert_cond_to_if(const Value& v, Masp& masp)
    {
        return expand_clauses(value_list(v)->rest(), masp); 
    }
    
    Value eval(const Value& v, Map& env, Masp& masp)
    {
        if(is_self_evaluating(v)) return v;
        else if(v.type == SYMBOL)
        {
            glh::ConstOption<Value> result = env.try_get_value(v);
            if(!result.is_valid()) throw EvaluationException(std::string("Symbol not found:") + *v.value.string);
            return *result;
        }
        else if(is_quoted(v))
        {
            const Value* ref_result = value_list_second(v);
            if(!ref_result) throw EvaluationException(std::string("Quote was not followed by an element"));
            return *ref_result;
        }
        else if(is_assignment(v))
        {
            const Value *asgn_var = assignment_var(v);
            const Value *asgn_val = assignment_value(v);
            if(asgn_var && asgn_val)
            {
                if(asgn_var->type != SYMBOL)
                    throw EvaluationException(std::string("Value to assign to was not symbol"));
                if(is_self_evaluating(*asgn_val))
                    env = env.add(*asgn_var, *asgn_val);
                else
                    env = env.add(*asgn_var, eval(*asgn_val, env, masp));
            }
            else
            {
                throw EvaluationException(std::string("Did not find anything to assign to."));
            }
            return Value();
        }
        else if(is_if(v))
        {
            const Value* if_predicate = value_list_second(v);
            if(if_predicate)
            {
                if(is_true(eval(*if_predicate, env, masp)))
                {
                    const Value* if_then = value_list_third(v);
                    if(if_then)
                    {
                        if(is_self_evaluating(*if_then)) return *if_then;
                        else return eval(*if_then, env, masp);
                    }
                    else throw EvaluationException(std::string("Did not find 'fst' in expected form (if pred fst snd)"));
                }
                else
                {
                    const Value* if_else = value_list_nth(v, 3);
                    if(if_else)
                    {
                        if(is_self_evaluating(*if_else))
                            return *if_else;
                        else
                            return eval(*if_else, env, masp);
                    }
                    else
                    {
                        return Value();
                    }
                }
            }
            else  throw EvaluationException(std::string("Did not find 'pred' in expected form (if pred fst snd)"));
        }
        else if(is_lambda(v))
        {
            const Value* lambda_parameters = value_list_second(v);
            const Value* lambda_body = value_list_third(v);

            if(lambda_parameters && lambda_body)
            {
                std::list<Value> lambda_list = glh::list(make_value_symbol("procedure"),
                        *lambda_parameters,
                        *lambda_body,
                        make_value_map(env));
                return make_value_list(new_list(masp, lambda_list));
            }
            else
            {
                throw EvaluationException(std::string("Could not find one or more of 'params' 'body' in (lambda params body) expression."));
            }
        }
        else if(is_begin(v))
        {
            return eval_sequence(value_list(v)->rest(), env, masp);
        }
        else if(is_cond(v))
        {
            return eval(convert_cond_to_if(v, masp), env, masp);
        }
        else if(is_application(v) && (!value_list(v)->empty())) // Is application
        {
            // Get operator
            const Value* first = value_list_first(v);
            Value op;

            if(!is_self_evaluating(*first)) 
                op = eval(*first, env, masp);
            else
                op = *first;

            List operands = value_list(*first)->rest();

            return apply(op, operands.begin(), operands.end(), env, masp);
        }

        throw EvaluationException(std::string("Could not find evaluable value."));

        return Value();
    }

    bool is_primitive_procedure(const Value& v){return v.type == FUNCTION;}
    bool is_compound_procedure(const Value& v){return is_tagged_list(v, "procedure");}

    ValueFunction value_function(const Value& v)
    {
        return v.value.function->fun;
    }

    Value apply(const Value& v, VRefIterator args_begin, VRefIterator args_end, Map& env, Masp& masp)
    {
        if(is_primitive_procedure(v))
        {
            return value_function(v)(masp, args_begin, args_end, env);
        }
        else if(is_compound_procedure(v))
        {
            // eval sequence
            const Value* proc_params = value_list_first(v);
            const Value* proc_body   = value_list_second(v);
            const Value* proc_env    =  value_list_third(v);

            List* params_list = value_list(*proc_params);
            List* body_list   = value_list(*proc_body);
            Map* proc_env_map     = value_map(*proc_env);

            if(proc_params->type != LIST || proc_body->type != LIST || proc_env->type != MAP)
            {
                throw EvaluationException(std::string("apply: Malformed compound procedure."));
            }

            if(!params_list) throw EvaluationException(std::string("apply: params_list is null."));
            if(!body_list) throw EvaluationException(std::string("apply: body_list is null."));
            if(!proc_env_map) throw EvaluationException(std::string("apply: env_map is null."));

            return eval_sequence(*value_list(*proc_body), proc_env_map->add(params_list->begin(), params_list->end(),
                        args_begin, args_end), masp);

        }
        return value_function(v)(masp, args_begin, args_end, env);
    }



} // empty namespace


/** More or less straightforward translation 
 *  of the Evaluator in 'Structure and Interpretation of Computer Programs' (Steele 1996)
 *  Section 4.1.1 'The Core of the Evaluator'*/
class Evaluator
{
public:

    Masp& masp_;

    Map env_;

    bool eval_ok;

    Evaluator(Masp& masp): masp_(masp), env_(masp_.env()->get_env())
    {
        eval_ok = true;
    }

    ~Evaluator()
    {
        if(eval_ok)
        {
            // Apply generated env back to masp_
            masp_.env()->get_env() = env_;
        }
    }
    
    Value eval_value(const Value& v)
    {
        return eval(v, env_, masp_);
    }
};

evaluation_result eval(Masp& m, const Value* v)
{
    Evaluator e(m);

    ValuePtr result(new Value(), ValueDeleter());

    //TODO: Return last result of an expression.
    //If is unquoted list

    try
    {
        *result = e.eval_value(*v);
    }catch(EvaluationException& e)
    {
        return evaluation_result(e.get_message());
    }

    // value - if not in place, add to value_stack, add reference to ref_stack
    // add value references to eval stack

    return evaluation_result(result);
}


} // Namespace masp ends
