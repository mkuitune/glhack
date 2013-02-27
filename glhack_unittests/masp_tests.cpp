/** Unit tests */


#include "glhack.h"
#include "persistent_containers.h"
#include "masp.h"
#include "masp_classwrap.h"
#include <string>
#include <functional>
using namespace std::placeholders;
#include "unittester.h"



int i;

template<class T>
bool expect_value(const masp::ValuePtr p, std::function<T(const masp::Value& v)> get, const T& comp, masp::Type expect_type)
{
    bool result = false;

    if(p->type == expect_type)
    {
        T tvalue = get(*p.get());
        if(comp == tvalue) result = true;
        else {
            std::ostringstream ostr;
            ostr << "expect_value: value mismatch. Expected:" << comp << " but got:" << *p.get() << std::endl;
            GLH_TEST_LOG(ostr.str());
        }
    }
    else
    {
        GLH_TEST_LOG(std::string("expect_value:type mismatch."));
    }

    return result;
}

template<class T>
bool compare_parsing(masp::Masp& m, const char* str, std::function<T(const masp::Value& v)> get, const T& comp, masp::Type expect_type)
{
    bool result = false;

    masp::masp_result r = masp::read_eval(m, str);

    if(r.valid()){

        result = expect_value(*r.as_value(), get, comp, expect_type);
    }
    else
    {
        GLH_TEST_LOG(r.message());
    }

    return result;
}
#define FAKE_CONTENTS "Fake!"
class FakeInputFile{
public:
    FakeInputFile(const std::string& path):path_(path){}
    ~FakeInputFile(){}
    std::istream& file(){}
    bool is_open(){
        return true;
    }
    void close(){}
    std::tuple<std::string, bool> contents_to_string(){
        return std::make_tuple(std::string(FAKE_CONTENTS), true);
    }

    std::string path_;

    class Printer{public: static std::string to_string(const FakeInputFile& f){return f.path_;}};
};

typedef masp::WrappedObject<FakeInputFile> WrappedInput;


masp::Value make_FakeInputFile(masp::Masp& m,
                         masp::VecIterator arg_start,
                         masp::VecIterator arg_end, masp::Map& env){

    std::string path;
    masp::ArgWrap(arg_start, arg_end).wrap(&path);

    masp::Value obj = masp::make_value_object(new WrappedInput(path));

    auto is_open = masp::wrap_member(&FakeInputFile::is_open);
    auto close = masp::wrap_member(&FakeInputFile::close);
    auto contents_to_string = masp::wrap_member(&FakeInputFile::contents_to_string);

    masp::FunMap fmap(m);
    fmap.add("is_open", is_open);
    fmap.add("close", close);
    fmap.add("contents_to_string", contents_to_string);

    masp::Value listv = masp::make_value_list(m);
    masp::List* l = masp::value_list(listv);

    *l = l->add(fmap.map());
    *l = l->add(obj);

    return listv;
}

UTEST(masp, object_interface_fake_input)
{
    using namespace glh;

    masp::Masp m;

    /*
         builder: return value, tahtn
    */

    masp::add_fun(m, "FakeInputFile", make_FakeInputFile);

    const char* src = "(def m (FakeInputFile \"input.txt\" ))"
                      "(def res (. 'is_open m))"
                      "(def contlist (. 'contents_to_string m))"
                      "(def contstring (first contlist))"
                      ;

    masp::masp_result evalresult = masp::read_eval(m, src);

    ASSERT_TRUE(evalresult.valid(), "Unsuccesfull parsing");

    auto res = masp::get_value(m, "res");

    ASSERT_TRUE(masp::value_type(res) == masp::BOOLEAN, "Type is not boolean");
    ASSERT_TRUE(masp::value_boolean(*res), "Result was not true.");


    auto contstring = masp::get_value(m, "contstring");
    auto contlist = masp::get_value(m, "contlist");

    ASSERT_TRUE(masp::value_type(contstring) == masp::STRING, "Type is not string");
    ASSERT_TRUE(strcmp(masp::value_string(*contstring),FAKE_CONTENTS) == 0
        , "Result did not match expected.");

}


UTEST(masp, get_value)
{
    using namespace glh;

    masp::Masp m;

    const char* def1 = "";

    ASSERT_TRUE(compare_parsing<masp::Number>(m, "1", masp::value_number, masp::Number::make(1), masp::NUMBER), "value mismatch");
    ASSERT_TRUE(compare_parsing<std::string>(m, "\"foo\"", masp::value_string, std::string("foo"), masp::STRING), "value mismatch");
    ASSERT_TRUE(compare_parsing<std::string>(m, "(def fooname \"foo\") fooname", masp::value_string, std::string("foo"), masp::STRING), "value mismatch");
}

UTEST(masp, simple_evaluations)
{
    using namespace glh;

    masp::Masp m;
    ASSERT_TRUE(compare_parsing<masp::Number>(m, "1", masp::value_number, masp::Number::make(1), masp::NUMBER), "value mismatch");
    ASSERT_TRUE(compare_parsing<std::string>(m, "\"foo\"", masp::value_string, std::string("foo"), masp::STRING), "value mismatch");
    ASSERT_TRUE(compare_parsing<std::string>(m, "(def fooname \"foo\") fooname", masp::value_string, std::string("foo"), masp::STRING), "value mismatch");

}

UTEST(masp, simple_parsing)
{
    using namespace glh;

    masp::Masp m;

    auto parsestr = [&m](const char* str)
    {
        masp::masp_result a = masp::string_to_value(m, str);

        if(a.valid())
        {
            const masp::Value* ptr = (*a).get();
            std::string value_str = masp::value_to_typed_string(ptr);
            GLH_TEST_LOG(value_str);
        }
    };

    parsestr("1");
    parsestr("0xf");
    parsestr("0b101");

    parsestr("\"foo\"");
    parsestr("(\"foo\")");
    parsestr("'(\"foo\" \"bar\")");
    parsestr("(+ 1 2)");
}


#if 0
class WrappedInStream{ public:
    virtual ~WrappedInStream(){}
    virtual std::string readline() = 0;
    virtual std::string readall() = 0;};

template<class T> class StreamIn : public WrappedInStream { public:
    T& stream_; StreamIn(T& stream):stream_(stream){} 
    virtual std::string readline() override {
        std::ostringstream os;
        stream_ 
    };
    virtual std::string readall() override {}
};

class WrappedOutStream{ public:
    virtual ~WrappedOutStream(){}
    virtual void write(const char* str) = 0;};

template<class T> class StreamOut : public WrappedOutStream{ public:
    T& stream_; StreamOut(T& stream):stream_(stream){}
    virtual void write(const char* str) override{stream_ << str;}
};
#endif