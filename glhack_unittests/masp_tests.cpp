/** Unit tests */


#include "glhack.h"
#include "persistent_containers.h"
#include "masp.h"
#include<string>

#include "unittester.h"


ADD_GROUP(masp);

int i;


UTEST(masp, simple)
{
    using namespace glh;

    masp::Masp m;

    auto parsestr = [&m](const char* str)
    {
        masp::parser_result a = masp::string_to_value(m, str);

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
