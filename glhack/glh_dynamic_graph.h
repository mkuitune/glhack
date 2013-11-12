/** \file glh_dynamic_graph.h
    \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)

Todo: Compile dynamic graph to execution stack

node:inputs, outputs, operator

input:local, external (but, stack address is sufficient in both cases)


-- type of following data, number of elements to read
struct Header{
    enum t{EXEC_NAME,   --context name, ie. operator to interpret following data
                --EXEC_NAME defines the context which is used to interpret the data
               FLOAT,
               VEC4, MAT4};
};

struct Var{union{int int_; double double_;};};
struct StackData{union{Header::t name_; Var var_;};};
struct Stack{std::vector<StackData> stack_; operator[]...}


#define OPNAME_SYS           1025
#define OPNAME_NODERECIEVER  1026
#define OPNAME_MIX           1027

stack: array[h0,s0,s1,s2,...sn]

THe format is: OPERATOR_NAME, VALUE_INDEX_1, VALUE_INDEX_2, ... , ... VALUE, VALUE, VALUE

void foo_operand(double* in1, double* in2, double* out1, ...)

eval(Stack& stack, size_t i){

    -- globals in local context
    double sys_time;
    ...

    Header::t opname = stack[i];
    switch(opname){
    case FOO:
    int index1 = stack[i+1].var_.int_;
    int index2 = ...
    double* in1 = &stack[index1];
    double* in1 = &stack[index2];
    ...
    foo_operand(in1, in2, ...)


... not sure if this would be any more efficient than straight forward graph evaluation. 
(if there are lots and lots of string based map accesses in the evaluation, then yes,
there might be a noticeable difference at some point (as these would be linear accesses,
and, we should be able to make the pure stack based approach more cache friendlier)
    }

}

Node inputs and output: node inputs are compiled into global vars in the beginning.
Likewise, once the stack has been evaluated, node values are copied from the stack vars
through the addresses stored in the nodes. I.e

struct NodeReciever{
    
    input_addresses std::map<sdt::string, tuple<value,int -- address-->> ...
    
    void collect(Stack& stack){
        for(auto& i: input_adresses){
            auto type  = i.second.first;
            auto index = i.second.second;
            if(i.second.first == A_SOME_TYPE){
                double val = stack[index];
                
                --... rest of the val handling same as with 
                -- plain graph evaluation
            }
        }
    }
}

-- TODO: Should really convert all name strings to string key integers where symbolic
-- processing is done....

    */
#pragma once

#include "glh_typedefs.h"
#include "glh_timebased_signals.h"
#include "glh_scenemanagement.h"

#include<tuple>
#include<functional>
#include<memory>

namespace glh{

template<class T>
struct List {

    struct ListNode{
        T data_;
        ListNode* next_;

        ListNode(const T& data):data_(data), next_(0){}
    };

    struct NodeCreator {
        std::deque<ListNode> data_;

        ListNode* new_node(const T& v){
            data_.push_back(ListNode(v));
            return &*data_.rbegin();}
    };

    struct iterator{
        ListNode* current_;
        iterator(ListNode* current):current_(current){}
        void operator++(){current_ = current_->next_;}
        bool operator!=(const iterator& rhs){return current_ != rhs.current_;}
        T& operator*(){return current_->data_;}
        T& operator->(){return current_->data_;}
    };

    ListNode*    head_;
    NodeCreator* creator_;

    List():head_(0), creator_(0){}
    List(NodeCreator* node_creator):head_(0), creator_(node_creator){}
    List(const List& other):head_(other.head_), creator_(other.creator_){}

    void insert(const T& value){
        if(!head_){
            head_ = creator_->new_node(value);
        } else {
            ListNode* last = head_;
            while(last->next_){last = last->next_;}

            ListNode* new_node =  creator_->new_node(value);
            last->next_ = new_node;
        }
    }

    iterator begin() const {return iterator(head_);}
    iterator end() const {return iterator(0);}
};


// Animation: source, target.

/** Adjacency list based graph. */
template<class V>
class AdjacencyListGraph{
public:

    typedef int                                vertex_id;
    typedef List<vertex_id>                    vertex_list_t;
    typedef std::map<vertex_id, vertex_list_t> list_map_t;

    static const vertex_id npos = 4294967295;

    vertex_id add_vertex_nocheck(const V& v){
        const vertex_id id = vertex_data_.size();
        vertex_data_.push_back(v);
        heads_.emplace(std::make_pair(id, vertex_list_t(&node_creator_)));
        return id;
    }

    vertex_id instantitate_vertex(const V& v){
        const vertex_id vpos = has_vertex(v);
        if(vpos == npos) return add_vertex_nocheck(v);
        else             return vpos;
    }

    vertex_id has_vertex(const V& v){
        std::vector<V>::iterator iter = std::find(vertex_data_.begin(), vertex_data_.end(), v);
        if(iter != vertex_data_.end()){
            const auto begin = vertex_data_.begin();
            vertex_id delta = iter - begin;
            return delta;
        }
        else                           return npos;
    }

    void add_edge(const V& from, const V& to){
        vertex_id fromid = instantitate_vertex(from);
        vertex_id toid   = instantitate_vertex(to);
        heads_[fromid].insert(toid);
    }

    std::vector<V>                      vertex_data_;
    list_map_t                          heads_;
    typename vertex_list_t::NodeCreator node_creator_;

};

/** Implements depth-first search postorder sorting. Sorts the graph
 *  into a forest of depth-first search trees and returns the forest
 *  in a linear list of vertex datums where the trees are in random
 *  order and each tree is a postorder sorted component of the input graph.
 *  
 *  TODO: Verify algorithmically that the graph is acyclic.
 *
 *  */
template<class T>
class DfsForestSort{
public:
    typedef AdjacencyListGraph<T>           graph_t;
    typename typedef graph_t::vertex_list_t graph_list_t;
    typename typedef graph_t::vertex_id     id_t;

    DfsForestSort(const graph_t& graph):graph_(graph){
        for(auto& vi:graph.heads_){
            visited_[vi.first] = false;}
    }

    void dfs(const id_t vertex){
        visited_[vertex] = true; 

        auto iter = graph_.heads_.find(vertex);

        const graph_list_t& list(iter->second);

        for(auto neighbour :list){
            if(!visited_[neighbour]){
                dfs(neighbour);}
        }

        if(!last_is(sorted_nodes_, vertex)) sorted_nodes_.push_back(vertex);
    }

    std::vector<T> operator()(){
    // The visiting information is not stored explicitly in graph nodes.
    // For large graphs this probably results in quite suboptimal performance.
    // For our purpose it should be sufficient.
        for(auto& vi:graph_.heads_){
            const id_t vertex = vi.first;
            if(!visited_[vertex]) dfs(vertex);
        }

        std::vector<T> sorted_vertices;
        for(auto id:sorted_nodes_){
            sorted_vertices.push_back(graph_.vertex_data_[id]);
        }

        return sorted_vertices;
    }

    std::vector<id_t>    sorted_nodes_;
    std::map<id_t, bool> visited_;
    const graph_t&       graph_;
};

// TODO Remove doubles from output.
template<class T>
std::vector<T> postorder_sort_graph(const AdjacencyListGraph<T>& graph){
    return DfsForestSort<T>(graph)();
}

class DynamicGraph{
public:

    struct Value{
        enum t{Empty, Scalar, Vector2, Vector3, Vector4};

        Value():type_(Empty){}
        Value(t type):type_(type){}
        Value(t type, float value):type_(type){
            value_.fill(value);
        }
        Value(float x, float y):type_(Vector2), value_(x, y, 0.f, 0.f){}
        Value(float x, float y, float z):type_(Vector3), value_(x, y, z, 0.f){}
        Value(t type, const array4& value):type_(type), value_(value){}
        Value(t type, const vec4& value):type_(type), value_(value){}

        /** The Scalar type can be used as a bool
        *   in some cases. Think it as a driving voltage over a diode.
        *   In these instances 0.f is returned for false and 1.f for true.
        *   The value can then be used e.g. for mixing nodes and they are converted
        *   into gates. This simplifies handling various context based events etc. 
        *   Do not let this leak beyond the DynamicGraph.*/
        static bool is_truthy(float f){return f > 0.9999f;}

        void get(float& res) const {res = value_[0];}
        void get(vec3& res) const {res = value_.change_dim<3>();}
        void get(vec2& res) const {res = value_.change_dim<2>();}
        void get(vec4& res) const {res = value_.to_vec();}
        void get(quaternion& res) const {res = quaternion(value_.data_);}

        array4 value_;
        t type_;
    };

    template<class V>
    static bool try_read(const Value& val, V& res){
        bool result = false;
        if(val.type_ != Value::Empty){val.get(res); result = true;}
        return result;
    }
    
    typedef const Value   constvalue_t;
    typedef constvalue_t* constvalue_t_ptr;
    typedef std::string   node_id_t;
    typedef std::string   var_id_t;

    typedef std::tuple<node_id_t, var_id_t, constvalue_t_ptr> datalink_t;

    static const node_id_t& link_node_id(datalink_t& link){return std::get<0>(link);}
    static const var_id_t&  link_var_id(datalink_t& link){return std::get<1>(link);}
    static constvalue_t_ptr&  link_varptr(datalink_t& link){return std::get<2>(link);}

    // TODO: Design: do we want to store the node's name in the node itself.
    struct DynamicNode{

        typedef std::map<std::string, datalink_t> inputs_map_t;

        inputs_map_t                 inputs_;  //> sinkname, link to value to read
        std::map<std::string, Value> sinks_;   //> Available sinks - "prototypes". TODO: use only Value::Type!
        std::map<std::string, Value> sources_; //> Available sources.                    or not. This way we can have a default value inplace.

        std::string                  typename_;

        inputs_map_t& get_inputs(){return inputs_;}
        const inputs_map_t& get_inputs() const {return inputs_;}

        DynamicNode(const char* typestring):typename_(typestring){}

        /** Create a reference from sink var with the specified source var of the given node. */
        void add_link(const std::string& sinkname, const node_id_t& linked_node, const var_id_t& source_var){
            datalink_t link = std::make_tuple(linked_node, source_var, (constvalue_t_ptr) 0);
            inputs_[sinkname] = link;
        }

        constvalue_t_ptr try_get_sourcevar(const std::string& varname){
            constvalue_t_ptr result = 0;
            auto iter = sources_.find(varname);
            if(iter != sources_.end())
                result = &(iter->second);
            return result;
        }

        constvalue_t read_input(const std::string& input_name){
            constvalue_t_ptr res = 0;
            auto i = inputs_.find(input_name);
            if(i != inputs_.end()) res = link_varptr(i->second);

            if(!res) return Value(Value::Empty);
            else     return *res;
        }

        template<class T>
        T read_input(const std::string& input_name){
            constvalue_t val = read_input(input_name);
            T output;
            val.get(output);
            return output;
        }

        void set_output(const std::string& output_name, const Value& val){
            sources_[output_name] = val;}

        void set_output(const std::string& output_name, const Value::t& type){
            sources_[output_name] = Value(type);}

        template<class V>
        void set_output(const std::string& output_name, const Value::t& type, const V& val){
            sources_[output_name] = Value(type, val);}

        void set_output(const std::string& output_name, const float val){
            sources_[output_name] = Value(Value::Scalar, val);}

        void set_input_address(const std::string& input_name, constvalue_t_ptr ptr){
            link_varptr(inputs_[input_name]) = ptr;
        }

        /** This initializes the input field to the sink var. The sink var is a placeholder which can be 
        *   set manually. Once a connection is made between the input of a particular variable of this node
        *   and another node the pointer is redirected to the var in the outputs of the other node.
        *   add_link is used to link a var of this node with an output var of other node.*/
        void add_input(const std::string& varname, Value::t type){
            sinks_[varname] = Value(type);
            set_input_address(varname, &sinks_[varname]);}

        template<class V>
        void add_input(const std::string& varname, Value::t type, const V& val){
            sinks_[varname] = Value(type, val);
            set_input_address(varname, &sinks_[varname]);}

        void add_input(const std::string& varname, const float val){
            sinks_[varname] = Value(Value::Scalar, val);
            set_input_address(varname, &sinks_[varname]);}

        /** Call eval. Eval can read values using read_input and write values using set_output.*/
        virtual void eval() = 0;
    };

    typedef std::shared_ptr<DynamicNode>            dynamic_node_ptr_t;
    typedef std::map<node_id_t, dynamic_node_ptr_t> node_dictionary_t;

    DynamicNode* try_get_node(const node_id_t& node){
        auto iter = node_dictionary_.find(node);
        if(iter == node_dictionary_.end()) return 0;
        else return iter->second.get();}

    /** This links the target input data to the source node output data. */
    bool add_link(const node_id_t& sourcename, const var_id_t& sourcevar,
                  const node_id_t& targetname, const var_id_t& targetvar){
        bool link_succesfull = false;

        DynamicNode* source = try_get_node(sourcename);
        DynamicNode* target = try_get_node(targetname);

        if(source && target){
            if(source->try_get_sourcevar(sourcevar)){ 
                target->add_link(targetvar, sourcename, sourcevar);
                link_succesfull = true;}}
        return link_succesfull;}

    /** Might need some sanity checks later on, thus the constant return value. */
    bool add_node(const std::string& name, const dynamic_node_ptr_t& nodeptr){
        node_dictionary_.emplace(std::make_pair(name, dynamic_node_ptr_t(nodeptr)));
        return true;}

    void build_evaluation_queue(){
        typedef AdjacencyListGraph<DynamicNode*> dependency_graph_t;

        dependency_graph_t graph;

        std::set<std::pair<DynamicNode*, DynamicNode*>> edges;

        for(auto& id_node_i:node_dictionary_){
            DynamicNode* node = id_node_i.second.get(); 

            for(auto& i: node->inputs_){
                auto source_id = link_node_id(i.second);
                if(source_id.empty()) continue;
                DynamicNode* source = try_get_node(source_id);
                if(source){
                    edges.insert(std::make_pair(source, node));
                }else{
                    throw GraphicsException(std::string("Invalid link from node") 
                            + id_node_i.first + std::string(" to ") +  source_id);
                }
            }
        }

        for(auto& e:edges){
            graph.add_edge(e.second, e.first);
        }
        
        // Depth first sort graph
        evaluation_queue_ = postorder_sort_graph(graph);
    }

    //Call after each time the graph is reconfigured. No need to call every frame. 
    void solve_dependencies(){

        // Collect source var addresses
        for(auto& inode: node_dictionary_){
            DynamicNode* input_node = inode.second.get();
            for(auto& input: input_node->get_inputs()){

                auto srcnodename = link_node_id(input.second);

                if(srcnodename.empty()) continue; // TODO, do we need some graph computability checks here.

                DynamicNode* srcnode = try_get_node(srcnodename);

                if(srcnode){
                    auto srcvarname = link_var_id(input.second);
                    if(constvalue_t_ptr var = srcnode->try_get_sourcevar(srcvarname)){
                        link_varptr(input.second) = var;
                    } else {
                        throw GraphicsException(std::string("Invalid link from node") 
                                + inode.first + std::string(" to ") +  srcnodename + std::string("/") + srcvarname);
                    }
                } else {
                    throw GraphicsException(std::string("Invalid link from node") 
                            + inode.first + std::string(" to ") +  srcnodename);
                }
            }}
        
        build_evaluation_queue();
    }

    void execute(){
        // Go through the evaluation_queue_;
        for(auto& node:evaluation_queue_){
            node->eval();
        }
    }

    std::vector<DynamicNode*> evaluation_queue_;
    node_dictionary_t         node_dictionary_;

};

/* TODO! Add verification code that source and sink datatypes match! */

class NodeReciever : public DynamicGraph::DynamicNode{
public:
    SceneTree::Node* node_;

    NodeReciever(SceneTree::Node* node):node_(node), DynamicGraph::DynamicNode("NodeReciever"){
        if(!node) throw GraphicsException("Trying to init NodeReciever with empty node!");
        add_input(GLH_CHANNEL_ROTATION, DynamicGraph::Value::Empty);
        add_input(GLH_CHANNEL_POSITION, DynamicGraph::Value::Empty);
        add_input(GLH_CHANNEL_SCALE, DynamicGraph::Value::Empty);

        // TODO: Should be able to add paramaeters to all shader variables.

        //sinks_[GLH_CHANNEL_ROTATION] = DynamicGraph::Value();
        //sinks_[GLH_CHANNEL_POSITION] = DynamicGraph::Value();
        //sinks_[GLH_CHANNEL_SCALE]    = DynamicGraph::Value();
    }

    void add_var(const std::string& name){
        auto hasvar = node_->material_.has(name);

        if(hasvar.second){
            if(hasvar.first == ShaderVar::Scalar){
                add_input(name, DynamicGraph::Value::Scalar);
            }
            else if(hasvar.first == ShaderVar::Vec4){
                add_input(name, DynamicGraph::Value::Vector4);
            }
            else{
                throw GraphicsException(std::string("Trying to link un-supported type for var") + name + std::string("as input term to NodeReciever"));
            }
        }
        else{
            throw GraphicsException(std::string("Trying to link non-existing var") + name + std::string("as input term to NodeReciever"));
        }
    }

    void read_in_val( const std::string& name, DynamicGraph::constvalue_t_ptr val){
        if(val->type_ != DynamicGraph::Value::Empty){
                 if(name == GLH_CHANNEL_ROTATION){val->get(node_->transform_.rotation_);}
            else if(name == GLH_CHANNEL_POSITION){val->get(node_->transform_.position_);}
            else if(name == GLH_CHANNEL_SCALE)   {val->get(node_->transform_.scale_);}

            else{
                // Handle:
                //  * Environment vars
                if(val->type_ == DynamicGraph::Value::Scalar){
                    float scalar;
                    val->get(scalar);
                    node_->material_.set_scalar(name, scalar);
                }
                else if(val->type_ == DynamicGraph::Value::Vector4){
                    vec4 vec;
                    val->get(vec);
                    node_->material_.set_vec4(name, vec);
                }
            }
        }
    }

    void eval() override {
        for(auto& v:inputs_){
            read_in_val(v.first, DynamicGraph::link_varptr(v.second));
        }
    }
};

class NodeSource : public DynamicGraph::DynamicNode{
public:
    SceneTree::Node* node_;
    std::list<std::string> vars_;

    NodeSource(SceneTree::Node* node, std::list<std::string> vars):node_(node), vars_(vars), DynamicGraph::DynamicNode("NodeSource"){
        if(!node) throw GraphicsException("Trying to init NodeReciever with empty node!");

        for(auto& var:vars_){
            set_output(var, DynamicGraph::Value::Empty);}
    }

    void try_set_var(RenderEnvironment& env, const std::string& name){

        auto varresult = env.has(name);
        if(varresult.second){
            if(varresult.first == ShaderVar::Vec4){
                array4 val = env.get_vec4(name);
                set_output(name, DynamicGraph::Value(DynamicGraph::Value::Vector4, val));
            }
            else if(varresult.first == ShaderVar::Scalar){
                float val = env.get_scalar(name);
                set_output(name, DynamicGraph::Value(DynamicGraph::Value::Scalar, val));
            }
        }
    }

    void eval() override {
        for(auto& var:vars_) try_set_var(node_->material_, var);
    }
};

/** Mix inputs of vec4 (color). */
class MixNode : public DynamicGraph::DynamicNode{
public:

    MixNode():DynamicGraph::DynamicNode("MixNode"){
        vec4 color_out(COLOR_RED);
        set_output(GLH_PROPERTY_COLOR, DynamicGraph::Value::Vector4, color_out);

        vec4 color1(COLOR_WHITE);
        vec4 color2(COLOR_BLACK);

        add_input(GLH_PROPERTY_1, DynamicGraph::Value::Vector4, color1);
        add_input(GLH_PROPERTY_2, DynamicGraph::Value::Vector4, color2);

        add_input(GLH_PROPERTY_INTERPOLANT, DynamicGraph::Value::Scalar, 0.0f);
    }

    void eval() override {
        vec4 color_out;
        vec4 color1       = read_input<vec4>(GLH_PROPERTY_1);
        vec4 color2       = read_input<vec4>(GLH_PROPERTY_2);
        float interpolant = read_input<float>(GLH_PROPERTY_INTERPOLANT);
        interpolant = constrain(interpolant, 0.0f, 1.0f);
        color_out = lerp(interpolant, color1, color2);

        set_output(GLH_PROPERTY_COLOR, DynamicGraph::Value::Vector4, color_out);
    }
};

/** Mix inputs of scalar. */
class MixScalar : public DynamicGraph::DynamicNode{
public:

    MixScalar():DynamicGraph::DynamicNode("MixScalar"){
        set_output(GLH_PROPERTY_INTERPOLANT, DynamicGraph::Value::Scalar, 0.f);

        add_input(GLH_PROPERTY_1,  DynamicGraph::Value::Scalar, 0.f);
        add_input(GLH_PROPERTY_2,  DynamicGraph::Value::Scalar, 1.f);

        add_input(GLH_PROPERTY_INTERPOLANT, DynamicGraph::Value::Scalar, 0.0f);
    }

     MixScalar(float fst, float snd):DynamicGraph::DynamicNode("MixScalar"){
        set_output(GLH_PROPERTY_INTERPOLANT, DynamicGraph::Value::Scalar, 0.f);

        add_input(GLH_PROPERTY_1,  DynamicGraph::Value::Scalar, fst);
        add_input(GLH_PROPERTY_2,  DynamicGraph::Value::Scalar, snd);

        add_input(GLH_PROPERTY_INTERPOLANT, DynamicGraph::Value::Scalar, 0.0f);
    }

    void eval() override {
        float out;
        float color1       = read_input<float>(GLH_PROPERTY_1);
        float color2       = read_input<float>(GLH_PROPERTY_2);
        float interpolant = read_input<float>(GLH_PROPERTY_INTERPOLANT);
        interpolant = constrain(interpolant, 0.0f, 1.0f);
        out = lerp(interpolant, color1, color2);

        set_output(GLH_PROPERTY_INTERPOLANT, DynamicGraph::Value::Scalar, out);
    }
};

/** Offset: bias, scale */
class ScalarOffset : public DynamicGraph::DynamicNode{
public:
    ScalarOffset():DynamicGraph::DynamicNode("ScalarOffset"){
        add_input(GLH_PROPERTY_INTERPOLANT, DynamicGraph::Value::Scalar, 0.f);
        add_input(GLH_PROPERTY_SCALE, DynamicGraph::Value::Scalar, 1.f);
        add_input(GLH_PROPERTY_BIAS, DynamicGraph::Value::Scalar, 0.f);

        set_output(GLH_PROPERTY_INTERPOLANT, DynamicGraph::Value::Scalar, 0.f);
    }

    void eval() override {
        float output;
        float input = read_input<float>(GLH_PROPERTY_INTERPOLANT);
        float bias = read_input<float>(GLH_PROPERTY_BIAS);
        float scale = read_input<float>(GLH_PROPERTY_SCALE);
        
        output = input * scale + bias;
        set_output(GLH_PROPERTY_INTERPOLANT, output);
    }
};

/** Lerp, etc. Limited to range [0,1].*/
class ScalarRamp : public DynamicGraph::DynamicNode{
public:

    float lower_;
    float higher_;
    Animation::Easing::t easing_;

    ScalarRamp():lower_(0.0f), higher_(1.0f), easing_(Animation::Easing::Linear), DynamicGraph::DynamicNode("ScalarRamp"){
       add_input(GLH_PROPERTY_INTERPOLANT, 0.f); 
       set_output(GLH_PROPERTY_INTERPOLANT, 0.f);
    }

    ScalarRamp(float lower, float higher):lower_(lower), higher_(higher), easing_(Animation::Easing::Linear), DynamicGraph::DynamicNode("ScalarRamp"){
       add_input(GLH_PROPERTY_INTERPOLANT, 0.f); 
       set_output(GLH_PROPERTY_INTERPOLANT, 0.f);
    }

    void eval() override {
        float value = read_input<float>(GLH_PROPERTY_INTERPOLANT);

             if(value < lower_) set_output(GLH_PROPERTY_INTERPOLANT, lower_);
        else if(value > higher_) set_output(GLH_PROPERTY_INTERPOLANT, higher_);
        else set_output(GLH_PROPERTY_INTERPOLANT, Animation::easing(easing_, value));

    }
};

class LimitedIncrementalValue : public DynamicGraph::DynamicNode{
public:
    double value_;
    double minimum_;
    double maximum_;

    LimitedIncrementalValue():value_(0.0), minimum_(-1.e37), maximum_(1.e37), DynamicGraph::DynamicNode("LimitedIncrementalValue"){
        config_node();
    }

    LimitedIncrementalValue(double value, double min, double max):value_(value), minimum_(min), maximum_(max), DynamicGraph::DynamicNode("LimitedIncrementalValue"){
        config_node();
    }

    void config_node(){
       add_input(GLH_PROPERTY_DELTA, 0.f); 
       set_output(GLH_PROPERTY_INTERPOLANT, 0.f);
    }

    void eval() override {
        double delta = read_input<float>(GLH_PROPERTY_DELTA);
        if(delta < 0.0){
            if(value_ > minimum_) value_ = value_ + delta;
        } else {
            if(value_ < maximum_) value_ = value_ + delta;
        }
        set_output(GLH_PROPERTY_INTERPOLANT, (float) value_);
    }
};

class SystemInput : public DynamicGraph::DynamicNode{
public:
    App* app_;
    double previous_time_;

    SystemInput(App* app):app_(app), previous_time_(-1.0),DynamicGraph::DynamicNode("SystemInput"){
        set_output(GLH_CHANNEL_TIME, DynamicGraph::Value::Scalar, (float) app_->time());
        set_output(GLH_PROPERTY_TIME_DELTA, DynamicGraph::Value::Scalar, 0.f);
    }

    void eval() override {
        double time = app_->time();
        double delta = 0.0;
        if(previous_time_ > 0.0) delta = time - previous_time_;
        previous_time_ = time;

        set_output(GLH_CHANNEL_TIME, DynamicGraph::Value::Scalar, (float) time);
        set_output(GLH_PROPERTY_TIME_DELTA, DynamicGraph::Value::Scalar, (float) delta);
    }
};

/**  */
class NodeFocusState : public DynamicGraph::DynamicNode{
public:
    SceneTree::Node* node_;
    FocusContext&    context_;

    NodeFocusState(SceneTree::Node* node, FocusContext& context):node_(node), context_(context), DynamicGraph::DynamicNode("NodeFocusState"){
        set_output(GLH_PROPERTY_INTERPOLANT, DynamicGraph::Value::Scalar, 0.0f);
    }

    bool is_focused(){
        return context_.currently_focused_.contains(node_);}

    void eval() override {
        bool focused = is_focused();
        if(focused){set_output(GLH_PROPERTY_INTERPOLANT, DynamicGraph::Value::Scalar, 1.0f);}
        else{set_output(GLH_PROPERTY_INTERPOLANT, DynamicGraph::Value::Scalar, 0.0f);}
    }
};

#if 0 // TODO figure this out.
class TransformGadget : public DynamicGraph::DynamicNode{
public:

    mat4 input_to_target_;
    vec3 transformer_centroid_;

    TransformGadget(mat4 input_to_target, vec3 transformer_centroid_):input_to_target_(input_to_target){
        add_input(GLH_PROPERTY_POSITION_DELTA, DynamicGraph::Value::Vector2, 0.f);

        set_output(GLH_CHANNEL_ROTATION, DynamicGraph::Value::Vector3, 0.0f);
        set_output(GLH_CHANNEL_SCALE, DynamicGraph::Value::Vector3, 0.0f);
        set_output(GLH_CHANNEL_POSITION, DynamicGraph::Value::Vector3, 0.0f);
    }

    void eval() override {
        vec2 dp_in  = read_input<vec2>(GLH_PROPERTY_POSITION_DELTA);

        // ds: either distance dp to all or ... to specific dim
        // dr: dp dot rotator tangent
        vec3 dp;
        vec3 ds;
        vec3 dr;

        set_output(GLH_CHANNEL_ROTATION, DynamicGraph::Value::Vector3, dr);
        set_output(GLH_CHANNEL_SCALE, DynamicGraph::Value::Vector3, ds);
        set_output(GLH_CHANNEL_POSITION, DynamicGraph::Value::Vector3, dp);

    }
};
#endif

struct DynamicNodeRef{
    DynamicGraph::dynamic_node_ptr_t node_;
    std::string name_;

    DynamicNodeRef(DynamicGraph::DynamicNode* node, const std::string& name):node_(node), name_(name){}

    DynamicNodeRef(DynamicGraph::DynamicNode* node, const std::string& name, glh::DynamicGraph& graph)
        :node_(node), name_(name){
            add(graph);
        }

    void add(glh::DynamicGraph& graph){
        graph.add_node(name_, node_);
    }

    static std::function<DynamicNodeRef(DynamicGraph::DynamicNode* node)> factory(glh::DynamicGraph& graph){
        return [&graph](DynamicGraph::DynamicNode* node)->DynamicNodeRef{
            return DynamicNodeRef(node, node->typename_, graph);
        };
    }

    static std::function<DynamicNodeRef(DynamicGraph::DynamicNode* node)> factory_enumerate_names(glh::DynamicGraph& graph, glh::StringNumerator& string_numerator){
        return [&graph, &string_numerator](DynamicGraph::DynamicNode* node)->DynamicNodeRef{
            return DynamicNodeRef(node, string_numerator(node->typename_), graph);
        };
    }

    static std::function<void(DynamicNodeRef, const DynamicGraph::var_id_t&, DynamicNodeRef, const DynamicGraph::var_id_t&)> linker(glh::DynamicGraph& graph){
        return [&graph](DynamicNodeRef src, const DynamicGraph::var_id_t& srcvar, DynamicNodeRef dst, const DynamicGraph::var_id_t& dstvar){
            graph.add_link(src.name_, srcvar, dst.name_, dstvar);
        };
    }

};



} // namespace glh

