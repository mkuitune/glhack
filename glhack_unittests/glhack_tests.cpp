/** Unit tests */


#include "glhack.h"
#include "persistent_containers.h"

#include<list>
#include<map>
#include<functional>
#include<iostream> 

/// Minimal unit test framework

// Output stream mapping.
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

// TODO: Test framework and tests to separate files.

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


typedef glh::PersistentMapPool<std::string, int> SIMapPool;
typedef glh::PersistentMapPool<std::string, int>::Map SIMap;
typedef std::map<std::string, int> StlSIMap;

/* Map tests:
Use cases: Insert, Access, Remove, Gc, Print (IARGP).


*/

void test_overwrite(SIMap map)
{
    auto map1 = map.add("One", 1);
    print_pmap(map);
    auto map1_b = map.add("One", 11);
    print_pmap(map);
    auto map1_c = map1_b.add("One", 111);
    print_pmap(map);
}

void write_random_elements(const int n, glh::Random<int> rand, glh::cstring& prefix, StlSIMap& map)
{
    int i = 0;
    while(i++ < n)
    {
        int r = rand.rand();
        glh::cstring str = prefix + glh::to_string(r);
        map[str] = r;
    }
}


// Map test: write, and gc test TODO

// Map test: write, remove and gc test TODO

// Map test: write, create and destroy recursively n maps, check root values, gc, check root values TODO 


// Map tests: create and gc tests:
//
// WE: fill map by writing element  by element
// WM: fill map by instantiating it from an stl-map.
// Instantiation elements: e1 e2
//                                    Test matrix:
//                                    1   2  3  4      5,6,7,8
// create_gc_test 1: write map        WE WE WM WM  e1   -||-  e1
//                2: write map2       WE WM WM WE  e2   -||-  e1
//                3: verify map     
//                3: delete map2, gc
//                4: verify map

SIMap map_insert_elements(const StlSIMap& map, SIMapPool& pool, bool gc_at_each)
{
    SIMap out_map = pool.new_map();
    if(gc_at_each)
    {
        for(auto i = map.begin(); i != map.end(); ++i)
        {
            out_map = out_map.add(i->first, i->second);
            out_map.gc();
        }
    }
    else
    {
        for(auto i = map.begin(); i != map.end(); ++i)
        {
            out_map = out_map.add(i->first, i->second);
        }
    }
    return out_map;
}

typedef SIMap (*create_map_inserter)(const StlSIMap&, SIMapPool&, bool gc_at_each);

SIMap map_direct_instantiation(const StlSIMap& map, SIMapPool& pool,bool gc_at_each)
{
    SIMap out_map = pool.new_map(map);
    if(gc_at_each) pool.gc();
    return out_map;
}

SIMap map_remove_elements(const StlSIMap& map, SIMap pmap)
{
    for(auto i = map.begin(); i != map.end(); ++i)
    {
        pmap = pmap.remove(i->first);
    }
    return pmap;
}

/** Return true if all elements in reference are found in map.*/
bool verify_map_elements(const StlSIMap& reference, SIMap& map)
{
    bool found = true;

    // Verify elements both ways - from map to reference and from reference to map.

    // From reference to map - check are elements exist.
    for(auto i = reference.begin(); i != reference.end(); ++i)
    {
        auto v = map.try_get_value(i->first);
        if(! v.is_valid())
        {
            GLH_TEST_LOG("Persistent map did not contain all elements.");
            return false;
        }
    }

    //  from map to reference - check there are no extra elements.
    for(auto i = map.begin(); i != map.end(); ++i)
    {
        if(reference.count(i->first) < 1)
        {
            GLH_TEST_LOG("Persistent map contained unexpected elements.");
            return false;
        }
    }

    return found;
}

void map_create_and_gc_test_body(const StlSIMap& first_elements,
                                 const StlSIMap& second_elements,
                                 create_map_inserter first_insert,
                                 create_map_inserter second_insert, 
                                 bool first_gc,
                                 bool gc_at_each,
                                 bool remove_before_delete,
                                 bool& result)
{
    using namespace glh;

    result = false;

    SIMapPool pool;

    SIMap first_map = first_insert(first_elements, pool, gc_at_each);

    ASSERT_TRUE(verify_map_elements(first_elements, first_map), "First map invalid.");

    if(first_gc)
    { 
        pool.gc();
        ASSERT_TRUE(verify_map_elements(first_elements, first_map), "First map invalid.");
    }

    SIMap* second_map = new SIMap(second_insert(second_elements, pool, gc_at_each));

    ASSERT_TRUE(verify_map_elements(second_elements, *second_map), "Second map invalid.");
    ASSERT_TRUE(verify_map_elements(first_elements, first_map), "First map invalid.");

    pool.gc();

    ASSERT_TRUE(verify_map_elements(second_elements, *second_map), "Second map invalid.");
    ASSERT_TRUE(verify_map_elements(first_elements, first_map), "First map invalid.");

    delete(second_map);

    ASSERT_TRUE(verify_map_elements(first_elements, first_map), "First map invalid.");

    pool.gc();

    ASSERT_TRUE(verify_map_elements(first_elements, first_map), "First map invalid.");

    // Remove half of elements from first map, insert them to another map and back again and
    // verify the results.
    if(first_elements.size() > 1)
    {
        auto maps = split_container(first_elements, 2);
        const int removed = 1;
        const int kept = 0;

        for(auto i = maps[removed].begin(); i != maps[removed].end(); ++i)
        {
            first_map = first_map.remove(i->first);
        }

        if(gc_at_each) first_map.gc();

        ASSERT_TRUE(verify_map_elements( maps[kept], first_map), "Persistent map erase half verify failed.");
    }
    result = true;
}

bool persistent_map_create_and_gc_body(const StlSIMap& first_elements, const StlSIMap& second_elements,
                                       bool gc_first,bool gc_at_each, bool remove_before_delete)
{
    using namespace glh;

    bool result = false;

    map_create_and_gc_test_body(first_elements, second_elements, map_insert_elements, map_insert_elements,
                                gc_first, gc_at_each, remove_before_delete, result);
    if(!result) return false;
    map_create_and_gc_test_body(first_elements, second_elements, map_insert_elements, map_direct_instantiation,
                                gc_first, gc_at_each,remove_before_delete,result);
    if(!result) return false;
    map_create_and_gc_test_body(first_elements, second_elements, map_direct_instantiation, map_insert_elements, 
                                gc_first, gc_at_each,remove_before_delete,result);
    if(!result) return false;
    map_create_and_gc_test_body(first_elements, second_elements, map_direct_instantiation, map_direct_instantiation,
                                gc_first, gc_at_each,remove_before_delete,result);
    if(!result) return false;
    map_create_and_gc_test_body(first_elements, first_elements, map_insert_elements, map_insert_elements, 
                                gc_first, gc_at_each,remove_before_delete, result);
    if(!result) return false;
    map_create_and_gc_test_body(first_elements, first_elements, map_insert_elements, map_direct_instantiation,
                                gc_first, gc_at_each,remove_before_delete, result);
    if(!result) return false;
    map_create_and_gc_test_body(first_elements, first_elements, map_direct_instantiation, map_insert_elements,
                                gc_first, gc_at_each,remove_before_delete, result);
    if(!result) return false;
    map_create_and_gc_test_body(first_elements, first_elements, map_direct_instantiation, map_direct_instantiation,
                                gc_first, gc_at_each,remove_before_delete, result);
    return result;
}

GLHTEST(collections, PersistentMap_combinations)
{
    using namespace glh;
    std::list<int> sizes;
    add(sizes, 1)(2)(3)(4)(5)(6)(7)(11)(13)(17)(19)(23)(29)(31)(32)(33)(67)(135)(271)(543);
    auto size_pairs = all_pairs(sizes);

    Random<int> rand;

    // First run all tests with different maps.
    for(auto p = size_pairs.begin(); p != size_pairs.end(); ++p)
    {
        StlSIMap first_elements;
        StlSIMap second_elements;

        write_random_elements(p->first, rand, std::string(""), first_elements);
        write_random_elements(p->second, rand, std::string(""), second_elements);

        // Go through each six combinations of the three bools insertable
        for(uint32_t bools = 0; bools < 6; ++bools)
        {
            bool gc_first             = bit_is_on(bools, 0);
            bool gc_at_each           = bit_is_on(bools, 1);
            bool remove_before_delete = bit_is_on(bools, 2);

            bool result = persistent_map_create_and_gc_body(first_elements, second_elements, gc_first, gc_at_each, 
                                              remove_before_delete);
            ASSERT_TRUE(result, "Persistent_map_create_and_gc_body failed");

            // Run only with values from one map
            result = persistent_map_create_and_gc_body(first_elements, first_elements, gc_first, gc_at_each, 
                                              remove_before_delete);
            ASSERT_TRUE(result, "Persistent_map_create_and_gc_body failed");
        }
    }
}

GLHTEST(collections, PersistentMap_test)
{
    using namespace glh;


    auto visit_scope = [](SIMap& map, cstring& str, int i){
        auto map2 = map.add(str, i);
        print_pmap(map2);
    };

    PersistentMapPool<std::string, int> pool;
    SIMap map = pool.new_map();

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
