// This file unittester.h is part of Tiny Unittesting Framework (TU).
// This code is in the public domain. There is no warranty implied; use this code at your own risk. 
//
// Author: Mikko Kuitunen (mkuitune.kapsi.fi)
//
// A short guide
// =============
//
// Introduction
// ------------
// A Tiny Unittester Framework is a lightweight unit test suite for hobby projects and other exploratory purposes.
//
// The 'framework' is composed of only two files - unittester.h and unittester.cpp. The latter of which
// contains the main function. You build your unittestsuite by including unittester.h in all files that implement
// tests and then building them with unittester.cpp into an executable (of course, linking any libs you might want
// to use on the way).
//
//
// Usage
// -----
// You use the macro
//
// UTEST(<test_group>, <test_name>)
//
// to declare a test after which you write the test body wrapped in brackets.
//
// Use the macros
//
// ASSERT_TRUE(<statement_in_c++>, <message string>)
//
// and
//
// ASSERT_FALSE(<statement_in_c++>, <message string>)
//
// to catch erroneus states, seize the execution of a particular test and signal the test suite of failure in a particular test.
//
// Error in one test does not terminate the execution of the entire test suite.
//
// Example:
//
// UTEST(string_tests, catenate)
// {
//  std::string a("foo");
//  std::string b("bar");
//  std::string c = a + b;
//  ASSERT_TRUE(c == std::string("foobar"), "String catenation using operator '+' failed.");
// }
//
// The test group parameter is used for specifying individual groups for test execution. You can exclude the execution of all but
// explicitly added tests by using the following function:
//
// void ut_add_group(const std::string& str).
//
//
// Complete example:
// ----------------
//
// (in own_tests.cpp)
//
// UTEST(string_tests, constructor)
// {
//  std::string a("foo");
//  ASSERT_TRUE(strcmp("foo", a.c_str() == 0, "String constructor failed.");
// }
//
// UTEST(string_tests, find)
// {
//  std::string a("foo");
//  ASSERT_TRUE(a.find('o') == 1, "String find failed.");
// }
//
// UTEST(list_tests, begin)
// {
//  std::list<int> list;
//  list.push_back(12);
//  ASSERT_TRUE(*(list.begin()) == 12 , "Acquiring iterator to begin of string failed.");
// }
//
// ut_add_group("list_tests");
//
// // Now only the tests in list_tests group is run. Without the previous call all tests would execute.
//
//
#ifndef UNITTESTER_H
#define UNITTESTER_H


#include<list>
#include<map>
#include<functional>
#include<iostream> 
#include<string>
#include<memory>

/** Ouput stream for test logging. Usage: ut_test_out() << "Hello, you feisty tester's little helper!" */
std::ostream& ut_test_out();

void ut_test_err();

/** Use this macro to declare a test. */
#define UTEST(group_name, test_name)     \
class utestclass_##test_name {           \
    public: static void run();             \
};                                         \
UtTestAdd test_name##__add(TestCallback(utestclass_##test_name::run, #group_name, #test_name)); \
void utestclass_##test_name::run()

#define GLH_TEST_LOG(msg_param)do{ ut_test_out() << std::endl << "  " << msg_param << std::endl;}while(0)

// Asset that the stmnt_param evaluates to true, or signall failed test and log the message in msg_param. Test ends.
#define ASSERT_TRUE(stmnt_param, msg_param)do{if(!(stmnt_param)){ut_test_err(); ut_test_out() << std::endl << "  " << msg_param << std::endl;return;}}while(0)

// Asset that the stmnt_param evaluates to false, or signall failed test and log the message in msg_param. Test ends.
#define ASSERT_FALSE(stmnt_param, msg_param)do{if(stmnt_param){ut_test_err(); ut_test_out() << std::endl << "  " << msg_param << std::endl;return;}}while(0)

// Use this macro to define and add tests to the exclusively run test group
#define ADD_GROUP(str_param) AddGroup str_param##run_grp(#str_param)

/** Add groups for exclusive testruns. If no groups are added then all the tests are run.
 *  Otherwise only those groups that are exclusively added are run.*/
class AddGroup{public:AddGroup(const char* str);};


/////////// Test utilities ///////////

template<class T>
std::list<T> range_to_list(T start, T delta, T end)
{
    T value;
    std::list<T> result;

    for(value = start; value < end; value += delta) result.push_back(value);

    return result;
}

template<class T>
void print_container(T container)
{
    auto begin = container.begin();
    auto end = container.end();
    ut_test_out() << range_to_string(begin, end) << std::endl;
}


////////////// Implementation /////////////////

typedef std::function<void(void)> UtTestFun;

struct TestCallback{
    UtTestFun callback; 
    std::string group;
    std::string name;
    TestCallback(UtTestFun f, const char* grp, const char* str):
        callback(f), group(grp), name(str){}
    TestCallback():callback(0), name(""){}
};

typedef std::map<std::string, TestCallback> TestGroup;
typedef std::map<std::string, TestGroup> TestSet;


class UtTestAdd
{
public:
    UtTestAdd(const TestCallback& callback);
};


#endif
