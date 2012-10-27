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
    using namespace masp;

    Masp m;

    auto parsestr = [&m](const char* str)
    {
            parser_result a = string_to_value(m, str);

        if(a.valid())
        {
            GLH_TEST_LOG(value_to_string(*a));
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
