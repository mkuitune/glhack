/** Unit tests */


#include "glhack.h"
#include "persistent_containers.h"
#include "glh_image.h"
#include "glh_scenemanagement.h"
#include "glh_timebased_signals.h"
#include "glh_dynamic_graph.h"


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

UTEST(containers, SortedArray_test)
{
    using namespace glh;
    SortedArray<int> set;
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

    virtual void render(FullRenderable& r, RenderEnvironment& material, RenderEnvironment& env)  override {}

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
    RenderQueue       queue;
    RenderEnvironment env;

    FullRenderable* f0 = manager.create_renderable();

    std::map<SceneTree::Node*, int> counts;

    auto add_counts = [&counts](SceneTree::Node* n){counts[n] = 0;};

    SceneTree::Node* root = scene.root();

    add_counts(root);

    SceneTree::Node* n0 = scene.add_node(root); add_counts(n0);
    SceneTree::Node* n1 = scene.add_node(n0);   add_counts(n1);
    SceneTree::Node* n2 = scene.add_node(n0);   add_counts(n2);
    SceneTree::Node* n3 = scene.add_node(n1);   add_counts(n3);
    SceneTree::Node* n4 = scene.add_node(n1);   add_counts(n4);
    SceneTree::Node* n5 = scene.add_node(root); add_counts(n5);
    SceneTree::Node* n6 = scene.add_node(n5); add_counts(n6);
    SceneTree::Node* n7 = scene.add_node(n5); add_counts(n7);

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

////////// Graph routines /////////////

namespace TestGraph {
using namespace glh;
#define TEST_INPUT_  "test_input"
#define TEST_OUTPUT_ "test_output"

class SinkNode : public DynamicGraph::DynamicNode { public:

    float val_;

    SinkNode():val_(-1.f){
        add_input(TEST_INPUT_, DynamicGraph::Value::Empty);}

    void eval() override {
        auto val = read_input(TEST_INPUT_);
        DynamicGraph::try_read(val, val_);
    }
    
    static DynamicGraph::dynamic_node_ptr_t make(){
        return DynamicGraph::dynamic_node_ptr_t(new SinkNode()); }

};

class SourceNode : public DynamicGraph::DynamicNode { public:

    float val_;

    SourceNode():val_(1.f){
        set_output(TEST_OUTPUT_, DynamicGraph::Value::Scalar, val_);}

    void eval() override {
        set_output(TEST_OUTPUT_, DynamicGraph::Value::Scalar, val_);}
    
    static DynamicGraph::dynamic_node_ptr_t make(){
        return DynamicGraph::dynamic_node_ptr_t(new SourceNode());}

};

class TransformNode : public DynamicGraph::DynamicNode { public:

    float val_;

    TransformNode():val_(2.f){
        add_input(TEST_INPUT_, DynamicGraph::Value::Scalar);
        set_output(TEST_OUTPUT_, DynamicGraph::Value::Scalar, val_);
    }

    void eval() override {
        auto val = read_input(TEST_INPUT_); 
        DynamicGraph::try_read(val, val_);
        val_ = val_ * 3.f;
        set_output(TEST_OUTPUT_, DynamicGraph::Value::Scalar, val_);
    }
    
    static DynamicGraph::dynamic_node_ptr_t make(){
        return DynamicGraph::dynamic_node_ptr_t(new TransformNode()); }
};

}// end namespace TestGRaph


UTEST(dynamic_graph, simple)
{
    using namespace glh;
    using namespace TestGraph;

    DynamicGraph g;

    auto source    = SourceNode::make();
    auto sink      = SinkNode::make();
    auto transform = TransformNode::make();
    auto SOURCE = "srcnode";
    auto SINK = "sinknode";
    auto TRANSFORM = "transform";

    g.add_node(SOURCE, source);
    g.add_node(SINK, sink);
    g.add_node(TRANSFORM, transform);

    g.add_link(SOURCE, TEST_OUTPUT_, TRANSFORM, TEST_INPUT_); // This should link the input ptr to the source node var
    g.add_link(TRANSFORM, TEST_OUTPUT_, SINK, TEST_INPUT_);

    g.solve_dependencies();
    g.execute();

    // Verify SINK has value of 3.f
    SinkNode* sink_ptr = dynamic_cast<SinkNode*>(sink.get());
    float sinkval = sink_ptr->val_;
    ASSERT_TRUE(are_near(sinkval, 3.f), "Graph compute chain failed.");

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



