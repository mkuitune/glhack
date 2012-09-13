#ifndef MASP_H
#define MASP_H

/** Script environment. */
class Masp
{
public:

    enum Type{NUMBER, LIST, MAP, STRING, FUNCTION, OBJECT};

    struct ListRef{void* list;};
    struct MapRef{void* map;};
    struct FunRef{void* fun;};
    struct ObjRef{void* obj;};

    struct Number{
        enum Type{INT,DOUBLE};
        union
        {
            int intvalue;
            double floatvalue;
        }value;
    };

    class Value
    {
        Type type;
        union
        {
            Number      number;
            const char* string;
            ListRef     list;
            MapRef      map;
            FunRef      fun;
            ObjRef      obj;
        } value;
    };

    class iterator
    {
        public:
            iterator();
            bool operator++();
            bool operator!=(const iterator& i){return value_ != i.value_;}
            const Value& operator*(){return *value_;}
            const Value& operator->(){return *value_;};

            Value* value_;
    };

    Masp();
    ~Masp();

    class Env;

    Env* env_;

};



#endif