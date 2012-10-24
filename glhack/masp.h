#ifndef MASP_H
#define MASP_H


#include "annotated_result.h"

#include<list>

namespace masp{

/** Script environment. */
class Masp
{
public:

#define MASP_VERSION 0.01

    enum Type{NUMBER, STRING, SYMBOL, CLOSURE, VECTOR, LIST, MAP, OBJECT};

    struct ListRef{void* list;};
    struct MapRef{void* map;};
    struct ClosureRef{void* fun;};
    struct ObjRef{void* obj;};
    struct VecRef{void* vec;};

    struct Number{
        enum Type{INT, DOUBLE};
        union
        {
            int    intvalue;
            double floatvalue;
        }value;
        Type type;

        Number& set(const int i){type = INT; value.intvalue = i; return *this;}
        Number& set(const double d){type = DOUBLE; value.floatvalue = d; return *this;}
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
            ClosureRef  fun;
            ObjRef      obj;
        } value;

        Value();
        ~Value();
        Value(const Value& v);
        Value(Value&& v);
        Value& operator=(const Value& v);
        Value& operator=(Value&& v);
        void alloc_str(const char* str);
        void alloc_str(const char* str, const char* end);

    };

    class Atom
    {
    public:
        /* Note: Legal types out of parser: NUMBER, STRING, SYMBOL, LIST.*/
        Type type;
        union
        {
            Number             number;
            const char*        string;
            std::list<Atom>*   list;
        } value;

        Atom();
        ~Atom();
        Atom(const Atom& atom);
        Atom(Atom&& atom);
        Atom& operator=(const Atom& a);
        Atom& operator=(Atom&& a);
        void alloc_str(const char* str);
        void alloc_str(const char* str, const char* end);
    };

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

typedef glh::AnnotatedResult<Masp::Atom> parser_result;

/** Parse string to atom data structure.*/
parser_result string_to_atom(Masp& m, const char* str);
/** Return string representation of atom. */
const std::string atom_to_string(const Masp::Atom& atom);

/** Evaluate the datastructure held within the atom in the context of the Masp env. Return result as atom.*/
Masp::Atom eval(Masp& m, const Masp::Atom& atom);

}//Namespace masp

#endif
