#ifndef MASP_H
#define MASP_H


#include "annotated_result.h"

#include<list>
#include<memory>


namespace masp{

enum Type{NUMBER, STRING, SYMBOL, CLOSURE, VECTOR, LIST, MAP, OBJECT, FUNCTION, LAMBDA};

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

class Value;
void free_value(Value* v);

class ValueRef
{
    public:
    ValueRef():v(0){}
    ~ValueRef(){if(v) free_value(v);}

    Value* get() const {return v;}

    Value* v;
};

typedef std::shared_ptr<ValueRef> ValueRefPtr;

/** Script environment. */
class Masp
{
public:

#define MASP_VERSION 0.01


    Masp();
    ~Masp();

    // TODO: replace std::List with plist
    // TODO: Eval : place value of atom to applied list
    // TODO: Apply: fun -> list -> value

    // Environment
    class Env;

    /** Return pointer to the environment instance. */
    Env* env();

    /** Garbage collect the used data structures.*/
    void gc();

    /** Number of bytes used by the state.*/
    size_t reserved_size_bytes();

    /** Number of bytes marked used.*/
    size_t live_size_bytes();

private:

    Env* env_;
};

typedef glh::AnnotatedResult<ValueRefPtr> parser_result;

/** Parse string to value data structure.*/
parser_result string_to_value(Masp& m, const char* str);

/** Return string representation of value. */
const std::string value_to_string(const Value* v);

/** Return string representation of value annotated with type. */
const std::string value_to_typed_string(const Value* v);

/** Return type of value as string.*/
const char* value_type_to_string(const Value* v);

/** Evaluate the datastructure held within the atom in the context of the Masp env. Return result as atom.*/
ValueRefPtr eval(Masp& m, const Value* v);

}//Namespace masp

#endif
