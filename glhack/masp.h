#ifndef MASP_H
#define MASP_H


#include<string>
#include<list>

/** Script environment. */
class Masp
{
public:

    enum Type{NUMBER, LIST, MAP, STRING, FUNCTION, OBJECT, SYMBOL};

    struct ListRef{void* list;};
    struct MapRef{void* map;};
    struct FunRef{void* fun;};
    struct ObjRef{void* obj;};

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


    class Atom
    {
    public:
        /** Legal types: NUMBER, STRING, SYMBOL, LIST.*/
        Type type;
        union
        {
            Number           number;
            const char*      string;
            std::list<Atom>* list;
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

    class Value
    {
    public:
        Type type;
        union
        {
            Number      number;
            const char* string;
            const char* symbol;
            ListRef     list;
            MapRef      map;
            FunRef      fun;
            ObjRef      obj;
        } value;
    };


    Masp();
    ~Masp();

    // Environment
    class Env;
    Env* env_;

};


/** Parse string to data structure. If err is not empty then parsing failed.*/
Masp::Atom string_to_atom(const char* str, std::string& err);
const std::string atom_to_string(const Masp::Atom& atom);

#endif
