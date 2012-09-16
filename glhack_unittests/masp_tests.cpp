/** Unit tests */


#include "glhack.h"
#include "persistent_containers.h"
#include "masp.h"
#include<string>

#include "unittester.h"


ADD_GROUP(masp);


UTEST(masp, simple)
{
    using namespace glh;

    Masp m;

    Masp::Atom a = m.compile_string("1");

    GLH_TEST_LOG(string_of_atom(a));

}
