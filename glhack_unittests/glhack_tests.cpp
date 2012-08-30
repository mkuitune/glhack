/** Unit tests */


#include "glhack.h"
#include "persistent_containers.h"

#include<list>
#include<map>
#include<functional>
#include<iostream> 

/// Minimal unit test framework

std::ostream* g_outstream = &std::cout;

std::ostream& glh_test_out(){return *g_outstream;}

#define GLH_TEST_LOG(msg_param)do{ glh_test_out() << std::endl << "  " << msg_param << std::endl;}while(0)
#define ASSERT_TRUE(stmnt_param, msg_param)do{if(!(stmnt_param)){glh_test_out() << std::endl << "  " << msg_param << std::endl; g_exec_result = false;return;}}while(0)
#define ASSERT_FALSE(stmnt_param, msg_param)do{if(stmnt_param){glh_test_out() << std::endl << "  " << msg_param << std::endl; g_exec_result = false;return;}}while(0)


typedef std::function<void(void)> GLhTestFun;


struct TestCallback{
    GLhTestFun callback; 
    std::string group;
    std::string name;
    TestCallback(GLhTestFun f, const char* grp, const char* str):
        callback(f), group(grp), name(str){}
    TestCallback():callback(0), name(""){}
};

typedef std::map<std::string, TestCallback> TestGroup;
typedef std::map<std::string, TestGroup> TestSet;

TestSet g_tests;

class GlhTestAdd
{
public:
    GlhTestAdd(const TestCallback& callback)
    {
        if(g_tests.count(callback.group) == 0) g_tests[callback.group] = TestGroup();

        g_tests[callback.group][callback.name] = callback;
    }
};

#define GLHTEST(group_name, test_name)     \
class glhtestclass_##test_name {           \
    public: static void run();             \
};                                         \
GlhTestAdd test_name##__add(TestCallback(glhtestclass_##test_name::run, #group_name, #test_name)); \
void glhtestclass_##test_name::run()

// Use global boolean to signal result of each test. No need for return values, shorter test functions.
bool g_exec_result;


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
    glh_test_out() << range_to_string(begin, end) << std::endl;
}

/////////// Tests here //////////////

GLHTEST(basic, test_test)
{
    glh_test_out() << "Test test" << std::endl;
    ASSERT_TRUE(true, "Statement was not true!");
    ASSERT_FALSE(false, "Statement was not false!");
}

/////////// Math ops /////////////

GLHTEST(math_ops, bit_ops_test)
{
    using namespace glh;

    uint32_t field = 0;
    uint32_t index;
 
    index = lowest_unset_bit(field);
    field = set_bit_on(field, index);

    index = lowest_unset_bit(field);
    field = set_bit_on(field, index);
    ASSERT_TRUE(field == 0x3, "Bit set operations failed");
}

/////////// Collections ////////////

GLHTEST(collections, PersistentList_test)
{
    using namespace glh;

    PersistentListPool<int> pool;
    auto list_empty = pool.new_list();
    auto list_a = pool.new_list(list(1, 2, 3, 4));
    auto list_b = pool.new_list(list(5, 6, 7, 8));

    print_container(list_a);
    print_container(list_b);

    std::list<int> long_range = range_to_list(1, 1, 100);
    // test gc

    pool.gc();

    auto list_c = pool.new_list(list(9, 10, 11, 12));

    auto list_long = pool.new_list(long_range);

    print_container(list_a);
    print_container(list_b);
    print_container(list_c);
    print_container(list_long);
}

template<class M> void print_pmap(M& map)
{
    glh_test_out() << "Contents of persistent map:" << std::endl;
    for(auto i = map.begin(); i != map.end(); ++i)
    {
        glh_test_out() << i->first  << ":" << i->second << std::endl;
    }
}

typedef glh::PersistentMapPool<std::string, int>::Map SI_Map;

void test_overwrite(SI_Map map)
{
    auto map1 = map.add("One", 1);
    print_pmap(map);
    auto map1_b = map.add("One", 11);
    print_pmap(map);
    auto map1_c = map1_b.add("One", 111);
    print_pmap(map);
}


GLHTEST(collections, PersistentMap_write_gc_test)
{
    // Write n elements to map
    // Write m elements to map2
    // delete map2, gc
    // Check map is unchanged.
}

GLHTEST(collections, PersistentMap_test)
{
    using namespace glh;


    auto visit_scope = [](SI_Map& map, cstring& str, int i){
        auto map2 = map.add(str, i);
        print_pmap(map2);
    };

    PersistentMapPool<std::string, int> pool;
    SI_Map map = pool.new_map();

    glh_test_out() << "  #########" << std::endl;

    print_pmap(map);
    auto map1 = map.add("Foo", 300);
    print_pmap(map);
    print_pmap(map1);

    glh_test_out() << "  #########" << std::endl;

    test_overwrite(map);
    print_pmap(map);
    print_pmap(map1);
    
    glh_test_out() << "  #########" << std::endl;

    auto map2 = map1.add("Removethis", 6996);
    print_pmap(map1);
    print_pmap(map2);
    map2 = map2.remove("Removethis");
    print_pmap(map1);
    print_pmap(map2);
    map2.gc();
    print_pmap(map1);
    print_pmap(map2);
}

/////////// Containers ////////////////

GLHTEST(containers, pool_test)
{
    using namespace glh;
    Pool<int> intpool;
}

GLHTEST(containers, pooled_list_test)
{
    using namespace glh;
    PooledList<int>::ListPool pool;
    PooledList<int> list1(pool);
    PooledList<int> list2(pool);

    add(list1, 1);
    add(list2, 2);

    glh_test_out() << list1 << std::endl;
    glh_test_out() << list2 << std::endl;
}

GLHTEST(containers, arrayset_test)
{
    using namespace glh;
    ArraySet<int> set;
    int ints[] = {5,1,1,2,3,1,2,4};
    size_t intsize = static_array_size(ints);
    add_range(set, ints, ints + intsize);
    
    std::cout << set << std::endl;
}

namespace {

    template<class M, class P>
    glh::Inserter2<M> insert_all_pairs(M& map, P ptr, P last)
    {
        if(ptr != last)
            return insert_all_pairs(map, ptr + 1, last)(ptr->first, ptr->second);
        else
            return glh::add(map, ptr->first, ptr->second);
    }
}

GLHTEST(containers, bimap_test)
{
    using namespace glh;
    BiMap<std::string, int> map;
    
    struct pr{std::string first; int second;} pairs[] = {{"one", 1}, {"two", 2}, {"three", 3}};
    const int pair_count = static_array_size(pairs);

    insert_all_pairs(map, pairs, pairs + pair_count - 1);

    glh_test_out() << map.map() << std::endl;
    glh_test_out() << map.inverse_map() << std::endl;
}

GLHTEST(containers, aligned_array_test)
{
    using namespace glh;
    size_t size = 500;
    AlignedArray<int> iarray(size, 17);
    auto is_aligned = [](int* t)->bool{return (((unsigned long) t) & 15) == 0;};

    ASSERT_TRUE( is_aligned(iarray.data() + 155), "Data is not aligned");

#if 0 // Check with this that compilation fails for non-aligned types
    struct UnAligned
    {
        int i,j;
        char c;
    };

    AlignedArray<UnAligned> failme_please;

#endif
}


////////// Shader utilities etc. ///////////

GLHTEST(shader_utilities, shader_parse_test)
{
    using namespace glh;
    bool result = true;
    const char* shader_f = 
        "#version 150\n"
        "uniform mat4 view_matrix;\n"
        "in vec4 position;\n"
        "in vec3 color;\n"
        "out vec3 v_color;\n"
        "void main(){\n"
        "  v_color = color;\n"
        "  gl_Position = view_matrix * vec4(position.x, position.y, position.z, 1.0);\n"
        "}";

    auto vars = parse_shader_vars(shader_f);
    
    glh_test_out() << std::endl;
    for(auto i = vars.begin(); i != vars.end();i++) glh_test_out() << "  " << *i << std::endl;

}


//////////////// Test Runners. ////////////////////

void run_test(const TestCallback& test)
{
    glh_test_out() << "Run " << test.name << " ";
    g_exec_result = true;
    test.callback();
    if(g_exec_result)
    {
        glh_test_out() << "\n    passed." << std::endl;
    }
    else
    {
        glh_test_out() << test.name << "\n    FAILED!" << std::endl; 
    }
}

void run_group(TestSet::value_type& group)
{
    using namespace glh;
    glh_test_out() << "Group:'" << group.first << "'" << std::endl;
    foreach_value(group.second, run_test);
}

bool run_all_tests()
{
    using namespace glh;
    bool result = true;
    
    foreach(g_tests, run_group);

    return result;
}

bool run_tests(const std::list<std::string>& group_names)
{
    using namespace glh;
    bool result = true;
    
    foreach(group_names, [&](const std::string& str) {run_group(*g_tests.find(str));});

    return result;
}

std::list<std::string> active_groups = glh::list(std::string("PersistentMap_test")); 

/** Run the tests. */
bool glh_run_tests()
{
    bool result = true;

    if(active_groups.size() > 0) run_tests(active_groups);
    else run_all_tests();

    return result;
}

int main(int argc, char* argv[])
{
    glh_run_tests();
}
