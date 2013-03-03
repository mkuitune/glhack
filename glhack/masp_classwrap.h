/** \file masp_classwrap.h. Wrapper for classes.
    \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/
#pragma once

#include "masp.h"
#include <typeinfo>
namespace masp{


/*
class IObject{
public:
    virtual ~IObject(){}
    virtual std::string to_string() = 0;
    virtual IObject* copy() = 0;
};

object := to_string, copy

return
(list value:obj value:map [funs] )


add functions to call wrapped object member funs

    Value <wrapper-for-somefun>(Masp& m, VecIterator arg_start, VecIterator arg_end, Map& env)
    {
        obj = art_start->value.object
        unwrap_n(arg_start, arg_end, a,b,c,...)
        instance = dynamic_cast<InstanceType>(object)
        instance->somefun(get_first_param_type(a), get_second_param_type(b), ...)


    }


objval = make_value_obj
mapval = make_value_map
    mapval add objval->to_string (?)
              {foreach  fun map = map->add(make_value_symbol(name), make_value_function(f))}


(. fun obj params) :=  (((fnext obj) fun) (first obj) params)
                           map       sym
                              function

*/

// object_factory = return list (value:IObject, value:map{name, objfun, name, objfun})
// objfun : void foo(Masp& m, VecIterator arg_start, VecIterator arg_end, Map& env)
// {obj = value_object(&*arg_start); if(obj){inst = dynamic_cast<inst_type>(obj)}}
// TOOD: add shorthand (. fun obj params) :=  (((fnext obj) fun) (first obj) params) = 
//                           

template<class T> T value_to(const Value& v);

template<class T>
class WrappedObject : public IObject { public:

    virtual ~WrappedObject(){}

    WrappedObject(T* ptr):t_(ptr){}

    WrappedObject(){}

    WrappedObject(std::shared_ptr<T> t):t_(t){}

    template<class C0> 
    WrappedObject(C0&& c0):t_(new T(std::forward<C0>(c0))){}
    //template<class C0, class C1> WrappedObject(const C0&& c0, const C1&& c1):t_(new T(c0, c1)){}

    virtual WrappedObject* copy() override {
        WrappedObject* obj = new WrappedObject(t_);
        return obj;
    }

    virtual std::string to_string() override {
        return typeid(T).name();
    }

    std::shared_ptr<T> t_;
};

template<class T>
auto wrapped_type()->WrappedObject<T>{return WrappedObject<T>();}

template<class T>
T* get_wrapped(IObject* iobj){
    typedef typename decltype(wrapped_type<T>()) wtype;
    wtype* wrapped = dynamic_cast<wtype*>(iobj);
    return wrapped->t_.get();
}

template<class T>
masp::Value to_value(masp::Masp& m, const T& val);

template<>
inline masp::Value to_value<bool>(masp::Masp& m, const bool& val){return masp::make_value_boolean(val);}

template<>
inline masp::Value to_value<std::string>(masp::Masp& m, const std::string& str){return masp::make_value_string(str);}

template<class P0, class P1>
masp::Value to_value(masp::Masp& m, const std::tuple<P0, P1>& input)
{
    using namespace masp;
    Value vlist = make_value_list(m);
    List* lst = value_list(vlist);
    *lst = lst->add(to_value(m, std::get<1>(input)));
    *lst = lst->add(to_value(m, std::get<0>(input)));
    return vlist;
}

template<class T>
T value_to_type(const Value& val);

#define TO_TYPE(type_param) template<> inline type_param value_to_type<type_param>(const Value& val)

TO_TYPE(Number){
    return value_number(val);
}

TO_TYPE(int){
    Number num = value_number(val);
    return num.to_int();
}

TO_TYPE(double){
    Number num = value_number(val);
    return num.to_float();
}

TO_TYPE(bool){
    return value_boolean(val);
}

TO_TYPE(std::string){
    const char* str = value_string(val);
    if(!str) throw EvaluationException("Cannot convert type to string");
    return std::string(str);
}

TO_TYPE(const char*){
    const char* str = value_string(val);
    if(!str) throw EvaluationException("Cannot convert type to string");
    return str;
}

TO_TYPE(IObject*){
    IObject* obj = value_object(val);
    if(!obj) throw EvaluationException("Cannot convert type to IObject.");
    return obj;
}


#undef TO_TYPE


//////// Member function wrapping ////////


class FunBase{public:
    virtual ~FunBase(){}
    virtual Value operator()(Masp& m, VecIterator arg_start, VecIterator arg_end, Map& env) = 0;
};

typedef std::shared_ptr<FunBase> FunBasePtr;

/** Parameter extraction. */
class ArgWrap{
public:

    VecIterator i_; VecIterator end_;

    ArgWrap(VecIterator i, VecIterator end):i_(i), end_(end){}

    template<typename T>
    void wrap(T* t){
        if(i_ == end_) throw EvaluationException("Trying to bind arguments from empty range");
        *t = value_to_type<T>(*i_);
    }
    
    template<typename T, typename... R>
    void wrap(T* t, R... rest){
        if(i_ == end_) throw EvaluationException("Trying to bind arguments from empty range");
        *t = value_to_type<T>(*i_);
    
        ++i_;

        wrap(rest...);
    }
    
    /** Return number of elements in wrapped range*/
    size_t size() const{
        VecIterator i(i_);
        size_t count = 0;
        while(i != end_){count++; ++i;}
        return count;
    }

};


class FunWrap0_0 : public FunBase{public:
    typedef std::function<void(void)> funt;
    funt fun_;

    FunWrap0_0(funt fun):fun_(fun){}

    Value operator()(Masp& m, VecIterator arg_start, VecIterator arg_end, Map& env) override {
        fun_();
        return Value();
    }
};

template<class P0>
class FunWrap0_1 : public FunBase{public:
    typedef std::function<void(P0)> funt;
    funt fun_;

    FunWrap0_1(funt fun):fun_(fun){}

    Value operator()(Masp& m, VecIterator arg_start, VecIterator arg_end, Map& env) override {
        P0 p0;
        ArgWrap(arg_start, arg_end).wrap(&p0);
        fun_(p0);
        return Value();
    }
};

template<class R>
class FunWrap1_0 : public FunBase{public:
    typedef std::function<R(void)> funt;
    funt fun_;

    FunWrap1_0(funt fun):fun_(fun){}

    Value operator()(Masp& m, VecIterator arg_start, VecIterator arg_end, Map& env) override {
        return to_value(m, fun_());
    }
};

template<class R, class P0>
class FunWrap1_1 : public FunBase{public:
    typedef std::function<R(P0)> funt;
    funt fun_;

    FunWrap1_1(funt fun):fun_(fun){}

    Value operator()(Masp& m, VecIterator arg_start, VecIterator arg_end, Map& env) override {
        P0 p0;
        ArgWrap(arg_start, arg_end).wrap(&p0);
        return to_value(m, fun_(p0));
    }
};

template<class R, class P0, class P1>
class FunWrap1_2 : public FunBase{public:
    typedef std::function<R(P0, P1)> funt;
    funt fun_;

    FunWrap1_2(funt fun):fun_(fun){}

    Value operator()(Masp& m, VecIterator arg_start, VecIterator arg_end, Map& env) override {
        P0 p0;
        P1 p1;
        ArgWrap(arg_start, arg_end).wrap(&p0, &p1);
        return to_value(m, fun_(p0, p1));
    }
};

inline PrimitiveFunction wrap_function(void (*mmbr)(void)){return FunWrap0_0(mmbr);}

template<class P0>
PrimitiveFunction wrap_function(void (*mmbr)(P0)){return FunWrap0_1<P0>(mmbr);}

template<class R, class P0>
PrimitiveFunction wrap_function(R (*mmbr)(P0)){return FunWrap1_1<R, P0>(mmbr);}

template<class R>
PrimitiveFunction wrap_function(R (*mmbr)(void)){return FunWrap1_0<R>(mmbr);}

template<class R, class P0, class P1>
PrimitiveFunction wrap_function(R (*mmbr)(P0, P1)){return FunWrap1_2<R, P0, P1>(mmbr);}


template<class CLS>
class MemFunWrap0_0 : public FunBase{public:
    typedef std::function<void(CLS)> funt;
    funt fun_;

    MemFunWrap0_0(funt fun):fun_(fun){}

    Value operator()(Masp& m, VecIterator arg_start, VecIterator arg_end, Map& env) override {
        IObject* obj;
        ArgWrap(arg_start, arg_end).wrap(&obj);
        CLS& cls = *get_wrapped<CLS>(obj);
        fun_(cls);
        return Value();
    }
};

template<class CLS, class P0>
class MemFunWrap0_1 : public FunBase{public:
    typedef std::function<void(CLS, P0)> funt;
    funt fun_;

    MemFunWrap0_1(funt fun):fun_(fun){}

    Value operator()(Masp& m, VecIterator arg_start, VecIterator arg_end, Map& env) override {
        IObject* obj;
        P0 p0;
        ArgWrap(arg_start, arg_end).wrap(&obj, &p0);
        CLS& cls = *get_wrapped<CLS>(obj);
        fun_(cls, p0);
        return Value();
    }
};

template<class CLS, class R>
class MemFunWrap1_0 : public FunBase{public:
    typedef std::function<R(CLS)> funt;
    funt fun_;

    MemFunWrap1_0(funt fun):fun_(fun){}

    Value operator()(Masp& m, VecIterator arg_start, VecIterator arg_end, Map& env) override {
        IObject* obj;
        ArgWrap(arg_start, arg_end).wrap(&obj);
        CLS& cls = *get_wrapped<CLS>(obj);
        return to_value(m, fun_(cls));
    }
};

template<class CLS, class R, class P0>
class MemFunWrap1_1 : public FunBase{public:
    typedef std::function<R(CLS, P0)> funt;
    funt fun_;

    MemFunWrap1_1(funt fun):fun_(fun){}

    Value operator()(Masp& m, VecIterator arg_start, VecIterator arg_end, Map& env) override {
        IObject* obj;
        P0 p0;
        ArgWrap(arg_start, arg_end).wrap(&obj, &p0);
        CLS& cls = *get_wrapped<CLS>(obj);
        return to_value(m, fun_(cls, p0));
    }
};

template<class CLS, class R, class P0, class P1>
class MemFunWrap1_2 : public FunBase{public:
    typedef std::function<R(CLS, P0, P1)> funt;
    funt fun_;

    MemFunWrap1_2(funt fun):fun_(fun){}

    Value operator()(Masp& m, VecIterator arg_start, VecIterator arg_end, Map& env) override {
        IObject* obj;
        P0 p0;
        P1 p1;
        ArgWrap(arg_start, arg_end).wrap(&obj, &p0, &p1);
        CLS& cls = *get_wrapped<CLS>(obj);
        return to_value(m, fun_(cls, p0, p1));
    }
};

template<class T>
PrimitiveFunction wrap_member(void (T::*mmbr)(void)){return MemFunWrap0_0<T>(mmbr);}

template<class T, class P0>
PrimitiveFunction wrap_member(void (T::*mmbr)(P0)){return MemFunWrap0_1<T, P0>(mmbr);}

template<class T, class R, class P0>
PrimitiveFunction wrap_member(R (T::*mmbr)(P0)){return MemFunWrap1_1<T, R, P0>(mmbr);}

template<class T, class R>
PrimitiveFunction wrap_member(R (T::*mmbr)(void)){return MemFunWrap1_0<T, R>(mmbr);}

template<class T, class R, class P0, class P1>
PrimitiveFunction wrap_member(R (T::*mmbr)(P0, P1)){return MemFunWrap1_2<T, R, P0, P1>(mmbr);}

class FunMap{public:
    Value mapv_;
    FunMap(Masp& m){
        mapv_ = make_value_map(m);
    }
    void add(const char* name, PrimitiveFunction fun){
        Map* map = value_map(mapv_);
        *map = map->add(make_value_symbol(name), make_value_function(fun));
    }
    Value& map(){return mapv_;}
};

}//Namespace masp

