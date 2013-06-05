/** Unit tests */


#include "glhack.h"
#include "persistent_containers.h"
#include "glh_image.h"
#include "glh_scenemanagement.h"

#include<string>
#include "unittester.h"



//ADD_GROUP(collections_pmap);

/////////// Tests here //////////////

UTEST(basic, test_test)
{
    ut_test_out() << "Test test" << std::endl;
    ASSERT_TRUE(true, "Statement was not true!");
    ASSERT_FALSE(false, "Statement was not false!");
}

/////////// Image ops /////////////

UTEST(image_ops, float_to_byte_min)
{
    using namespace glh;
    ASSERT_TRUE(float_to_ubyte(0.f) == 0x0, "float_to_byte_linear failed");
}

UTEST(image_ops, float_to_byte_max)
{
    using namespace glh;
    ASSERT_TRUE(float_to_ubyte(1.f) == 0xff, "float_to_byte_linear failed");
}

/////////// Math ops /////////////

UTEST(math_ops, bit_ops_test)
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
/////////// Containers ////////////////

UTEST(containers, pool_test)
{
    using namespace glh;
    Pool<int> intpool;
}

UTEST(containers, pooled_list_test)
{
    using namespace glh;
    PooledList<int>::ListPool pool;
    PooledList<int> list1(pool);
    PooledList<int> list2(pool);

    add(list1, 1);
    add(list2, 2);

    ut_test_out() << list1 << std::endl;
    ut_test_out() << list2 << std::endl;
}

UTEST(containers, arrayset_test)
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

UTEST(containers, bimap_test)
{
    using namespace glh;
    BiMap<std::string, int> map;
    
    struct pr{std::string first; int second;} pairs[] = {{"one", 1}, {"two", 2}, {"three", 3}};
    const int pair_count = static_array_size(pairs);

    insert_all_pairs(map, pairs, pairs + pair_count - 1);

    ut_test_out() << map.map() << std::endl;
    ut_test_out() << map.inverse_map() << std::endl;
}

UTEST(containers, aligned_array_test)
{
    using namespace glh;
    size_t size = 500;
    AlignedArray<int> iarray(size, 17);
    auto is_aligned = [](int* t)->bool{return (((unsigned long) t) & 15) == 0;};

    ASSERT_TRUE( is_aligned(iarray.data()), "Data is not aligned");

#if 0 // Check with this that compilation fails for non-aligned types
    struct UnAligned
    {
        int i,j;
        char c;
    };

    AlignedArray<UnAligned> failme_please;

#endif
}

/////////// Scene management ////////////////

namespace glh{

class DummyManager: public GraphicsManager {
public:
    virtual ProgramHandle* create_program(cstring& name, cstring& geometry, cstring& vertex, cstring& fragment) override {return 0;}

    virtual ProgramHandle* program(cstring& name)  override { return 0;}

    virtual void render(FullRenderable& r, RenderEnvironment& env)  override {}

    virtual void render(FullRenderable& r, ProgramHandle& program, RenderEnvironment& material, RenderEnvironment& env)  override {}

    virtual Texture* create_texture()  override {return 0;}

    virtual void remove_from_gpu(Texture* t) override {}

    virtual DefaultMesh*    create_mesh() override {return 0;}

    virtual FullRenderable* create_renderable() override {
        renderables_.push_back(FullRenderable());
        return & renderables_.back();
    }

    std::vector<FullRenderable> renderables_;
};
}

UTEST(scene, scene_rai_test)
{
    using namespace glh;

    SceneTree         scene;
    DummyManager      manager;
    RenderQueue       queue(&manager);
    RenderEnvironment env;

    FullRenderable* f0 = manager.create_renderable();

    std::map<SceneTree::Node*, int> counts;

    auto add = [&counts](SceneTree::Node* n){counts[n] = 0;};

    SceneTree::Node* root = scene.root();

    add(root);

    SceneTree::Node* n0 = scene.make_node(root); add(n0);
    SceneTree::Node* n1 = scene.make_node(n0);   add(n1);
    SceneTree::Node* n2 = scene.make_node(n0);   add(n2);
    SceneTree::Node* n3 = scene.make_node(n1);   add(n3);

    scene.update();

    for(auto n:scene){
        if(counts.find(n) != counts.end()){
            counts[n]++;
        }
    }

    for(auto& p:counts){
        ASSERT_TRUE(p.second > 0, "Scene did not contain all elements.");
    }

    //queue.add(scene);
    //queue.render(env);

}

////////// Shader utilities etc. ///////////
#if 0
UTEST(shader_utilities, shader_parse_test)
{
    using namespace glh;

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
    
    ut_test_out() << std::endl;
    for(auto i = vars.begin(); i != vars.end();i++) ut_test_out() << "  " << *i << std::endl;
}

#endif



