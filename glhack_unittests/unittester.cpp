// This file unittester.cpp is part of Tiny Unittesting Framework (TU).
// This code is in the public domain. There is no warranty implied; use this code at your own risk. 
//
// See unittester.h for instructions.
//
// Author: Mikko Kuitunen (mkuitune.kapsi.fi)
#include "unittester.h"

TestSet* g_tests = 0;
std::list<std::string>* g_active_groups = 0;

struct ActiveGroups
{
    ~ActiveGroups()
    {
        if(g_active_groups) delete g_active_groups;
        if(g_tests) delete g_tests;
    }
    void init(){
        if(!g_active_groups ) g_active_groups = new std::list<std::string>();
        if(!g_tests) g_tests = new TestSet();
    }
} g_ag;




UtTestAdd::UtTestAdd(const TestCallback& callback)
{
    g_ag.init();
    if((*g_tests).count(callback.group) == 0) (*g_tests)[callback.group] = TestGroup();

    (*g_tests)[callback.group][callback.name] = callback;
}



// Globals
bool g_exec_result;


void ut_test_err()
{
    ut_test_out() << "Error.";
    g_exec_result = false;
}

// Output stream mapping.
std::ostream* g_outstream = &std::cout;
std::ostream& ut_test_out(){return *g_outstream;}


AddGroup::AddGroup(const char* str)
{
    g_ag.init();
    g_active_groups->push_back(std::string(str));
}

/** Execute one test.*/
void run_test(const TestCallback& test)
{
    ut_test_out() << "Run " << test.name << " ";
    g_exec_result = true;
    test.callback();
    if(g_exec_result)
    {
        ut_test_out() << "\n    passed." << std::endl;
    }
    else
    {
        ut_test_out() << test.name << "\n    FAILED!" << std::endl; 
    }
}

/** Execute the test within one test group.*/
void run_group(TestSet::value_type& group)
{
    ut_test_out() << "Group:'" << group.first << "'" << std::endl;
    for(auto g = group.second.begin(); g != group.second.end(); ++g) run_test(g->second);
}

/** If the user has not defined any specific groups for testrun then this function executes all of the tests.*/
bool run_all_tests()
{
    bool result = true;
    for(auto t = g_tests->begin(); t != g_tests->end(); ++t) run_group(*t);
    return result;
}

/** If the user has defined exclusively runnable groups using ut_add_group then
 *  this function is used to execute them. */
bool run_tests(const std::list<std::string>& group_names)
{
    bool result = true;
    for(auto gname = group_names.begin(); gname != group_names.end(); ++gname)
    {
        auto test = g_tests->find(*gname);
        if(test == g_tests->end())
        {
            ut_test_out() << "Warning: Could not find group:" << *gname << std::endl;
        }
        {
            run_group(*test);
        }
    }

    return result;
}

int main(int argc, char* argv[])
{
    if(g_active_groups->size() > 0) run_tests(*g_active_groups);
    else run_all_tests();
    return 0;
}
