#ifndef MASP_H
#define MASP_H


#include "annotated_result.h"

#include<list>

namespace masp{

enum Type{NUMBER, STRING, SYMBOL, CLOSURE, VECTOR, LIST, MAP, OBJECT};

struct ListRef{void* data;};
struct MapRef{void* data;};
struct ClosureRef{void* data;};
struct ObjectRef{void* data;};
struct VectorRef{void* data;};

struct Number{
    enum Type{INT, FLOAT};
    union
    {
        int    intvalue;
        double floatvalue;
    }value;
    Type type;

    Number& set(const int i){type = INT; value.intvalue = i; return *this;}
    Number& set(const double d){type = FLOAT; value.floatvalue = d; return *this;}
    Number& set(const Number& n){
        type = n.type;
        if(type == INT) value.intvalue = n.value.intvalue;
        else            value.floatvalue = n.value.floatvalue;
        return *this;
    }

    int to_int() const
    {
        if(type == INT) return value.intvalue;
        else return (int) value.floatvalue;
    }
    
    double to_float() const
    {
        if(type == INT) return (double) value.intvalue;
        else return value.floatvalue;
    }
};

class Value
{
public:
    Type type;
    union
    {
        Number      number;
        const char* string; //> Data for string | symbol
        ListRef     list;
        MapRef      map;
        ClosureRef  closure;
        ObjectRef   object;
        VectorRef   vector;
    } value;

    Value();
    ~Value();
    Value(const Value& v);
    Value(Value&& v);
    Value& operator=(const Value& v);
    Value& operator=(Value&& v);
    void alloc_str(const char* str);
    void alloc_str(const char* str, const char* end);

private:
    void dealloc();
    void copy(const Value& v);
    void movefrom(Value& v);

};



/** Script environment. */
class Masp
{
public:

#define MASP_VERSION 0.01



    Masp();
    ~Masp();


    // TODO: replace std::List with plist
    // TODO: replace Atom with Value
    //TODO: Eval : place value of atom to applied list
    //TODO: Apply: fun -> list -> value

    // Environment
    class Env;

    Env* env_;



};

typedef glh::AnnotatedResult<Value> parser_result;

/** Parse string to value data structure.*/
parser_result string_to_value(Masp& m, const char* str);
/** Return string representation of value. */
const std::string value_to_string(const Value& v);
/** Return string representation of value annotated with type. */
const std::string value_to_typed_string(const Value& v);
/** Return type of value as string.*/
const char* value_type_to_string(const Value& v);

/** Evaluate the datastructure held within the atom in the context of the Masp env. Return result as atom.*/
Value eval(Masp& m, const Value& v);

}//Namespace masp

#endif
