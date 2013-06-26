/** \file glh_timebased_signals.h
    \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/
#pragma once

#include "glh_typedefs.h"

#include<tuple>
#include<functional>


namespace glh{

// Animation: source, target.

/** Adjacency list based graph. */
template<class V>
class AdjacencyListGraph{
    struct ListNode{
        V data_; ListNode* next_;
        ListNode():next_(0){}
        ListNode(const V& data):data_(data), next_(0){}
    };

    struct ListNodeCreator {
        std::vector<ListNode> data_;

        ListNode* new_node(const V& v){data_.push_back(ListNode(v)); return *data_.rbegin();}
        ListNode* new_node(){data_.push_back(ListNode()); return *data_.rbegin();}
    };

    struct List {
        struct iterator{
            ListNode* current_;
            iterator(ListNode* current):current_(current){}
            void operator++(){current_ = current_->next_;}
            bool operator!=(const iterator& rhs){return current_ != rhs.current_;}
            V& operator*(){return current_.data_;}
            V& operator->(){return current_.data_;}
        };

        ListNode*        head_;
        ListNodeCreator& creator_

        List(ListNodeCreator& node_creator):head_(0), creator_(node_creator){}

        void insert(const V& value){
            if(!head_){head_ = creator_.new_node(vertex);}

            ListNode* last = head_;
            while(last->next_){last = last->next_;}

            ListNode* new_node =  creator_.new_node(value);
            last->next = new_node;
            return new_node;
        }

        iterator begin(){iterator(0);}
        iterator end(){return iterator(0);}
    };

    typedef std::map<T, List> ListMap;

    void add_vertex(const V& v){
        if(heads_.find(v) != heads_.end()) heads_[v] = List(node_creator_);}

    void add_edge(const V& from, const V& to){
        add_vertex(from);
        heads_[from].insert(to);
    }

    ListMap         heads_;
    ListNodeCreator node_creator_;
};

class DynamicGraph{
public:

    struct Value{
        enum t{EMPTY, SCALAR, VECTOR3, VECTOR4};
        Value():type_(EMPTY){}
        Value(t type):type_(type){}
        Value(t type, float value):type_(type){
            value_.fill(value);
        }

        void export(vec3& res) const {res = value_.change_dim<3>();}
        void export(vec4& res) const {res = value_.to_vec();}
        void export(quaternion& res) const {res = quaternion(value_.data_);}

        t type_;
        array4 value_;
    };

    typedef const Value   constvalue_t;
    typedef constvalue_t* constvalue_t_ptr;
    typedef std::string   node_id_t;
    typedef std::string   var_id_t;

    typedef std::tuple<node_id_t, var_id_t, constvalue_t_ptr> datalink_t;

    static const node_id_t& link_node_id(datalink_t& link){return std::get<0>(link);}
    static const var_id_t&  link_var_id(datalink_t& link){return std::get<1>(link);}
    static constvalue_t_ptr&  link_varptr(datalink_t& link){return std::get<2>(link);}


    struct DynamicNode{
        std::map<std::string, datalink_t> inputs_;  //> sinkname, link to value to read
        std::map<std::string, Value>      sinks_;   //> Available sinks - "prototypes".
        std::map<std::string, Value>      sources_; //> Available sources.
   
        void add_link(const std::string& sinkname, const node_id_t& linked_node, const var_id_t& source_var){
            inputs_.emplace(std::make_pair(sinkname, std::make_tuple(linked_node, source_var, 0)));}

        constvalue_t_ptr try_get_sourcevar(const std::string& varname){
            auto iter = sources_.find(varname);
            if(iter != sources_.end()) return &(iter->second);
            else return 0;
        }

        constvalue_t_ptr read_input(const std::string& input_name){
            return link_varptr(inputs_[input_name]);
        }

        void set_output(const std::string& output_name, const Value& val){sources_[output_name] = val;}

        void add_input(const std::string& varname, Value::t type){sinks_[varname] = Value(type);}

        void set_input_address(const std::string& input_name, constvalue_t_ptr ptr){
            link_varptr(inputs_[input_name]) = ptr;
        }

        /** Call eval. Eval can read values using read_input and write values using set_output.*/
        virtual void eval() = 0;
    };

    typedef AdjacencyListGraph<std::string>         dependency_graph_t;
    typedef std::shared_ptr<DynamicNode>            dynamic_node_ptr_t;
    typedef std::map<node_id_t, dynamic_node_ptr_t> node_dictionary_t;

    DynamicNode* try_get_node(const node_id_t& node){
        auto iter = node_dictionary_.find(node);
        if(iter == node_dictionary_.end()) return 0;
        else return iter->second.get();}

    bool add_link(const node_id_t& sourcename, const var_id_t& sourcevar, const node_id_t& targetname, const var_id_t& targetvar){
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
        node_dictionary_.insert(std::make_pair(name, dynamic_node_ptr_t(nodeptr)));
        return true;}

    //Call after each time the graph is reconfigured. No need to call every frame. 
    void solve_dependencies(){
        dependency_graph_t graph;

        // Depth first sort graph

        // TODO TODO TODO

        // Collect source var addresses
        for(auto& inode: node_dictionary_){
            for(auto& input: inode.second->inputs_){
                auto srcnodename = link_node_id(input.second);
                DynamicNode* srcnode = try_get_node(srcnodename);
                if(srcnode){
                    auto srcvarname = link_var_id(input.second);
                    if(constvalue_t_ptr var = srcnode->try_get_sourcevar(srcvarname)){
                        link_varptr(input.second) = var;
                    } else {
                        throw GraphicsException(std::string("Invalid link from node") + inode.first + std::string(" to ") +  srcnodename + std::string("/") + srcvarname);
                    }
                } else {
                    throw GraphicsException(std::string("Invalid link from node") + inode.first + std::string(" to ") +  srcnodename);
                }
            }}
    }

    void execute(){
        // Go through the evaluation_queue_;
        for(auto& node:evaluation_queue_){
            node->eval();
        }
    }

    std::vector<DynamicNode*> evaluation_queue_;
    node_dictionary_t node_dictionary_;

};

/* TODO! Add verification code that source and sink datatypes match! */

class NodeReciever : public DynamicGraph::DynamicNode{
public:
    SceneTree::Node* node_;

    NodeReciever(SceneTree::Node* node):node_(node){
        if(!node) throw GraphicsException("Trying to init NodeReciever with empty node!");

        sinks_[GLH_CHANNEL_ROTATION] = DynamicGraph::Value();
        sinks_[GLH_CHANNEL_POSITION] = DynamicGraph::Value();
        sinks_[GLH_CHANNEL_SCALE]    = DynamicGraph::Value();
    }

    void eval() override {
        for(auto& v:sinks_){
            DynamicGraph::Value val = v.second;
            if(val.type_ != DynamicGraph::Value::EMPTY){
                if(v.first ==      GLH_CHANNEL_ROTATION){val.export(node_->rotation_);}
                else if(v.first == GLH_CHANNEL_POSITION){val.export(node_->location_);} //TODO MUSTFIX rename node location_ to position_
                else if(v.first == GLH_CHANNEL_SCALE)   {val.export(node_->scale_);}
            }
        }
    }
};

class SystemInput : public DynamicGraph::DynamicNode{
    App* app_;

    SystemInput(App* app):app_(app){
        set_output(GLH_CHANNEL_TIME, DynamicGraph::Value(DynamicGraph::Value::SCALAR, app_->time()));
    }
    
    void eval() override {
        set_output(GLH_CHANNEL_TIME, DynamicGraph::Value(DynamicGraph::Value::SCALAR, app_->time()));
    }
};

class Animation{
public:
    struct VarType{
        enum{Scalar, Verson};}; // Either regular scalar or rotation encoded in verson (unit quaternion).

    struct ChannelName{
        enum{Arbitrary};};

    struct Signature{VarType type; ChannelName name;};
};

class DynamicSystem{
public:

    class Event{
    public:
        typedef std::function<bool (float)> event_t;

        event_t event_;
        bool is_done_;

        Event(event_t event):event_(event), is_done_(false){}
        Event(){}
        void update(float t){if(!is_done_) is_done_ = event_(t);}
    };

    typedef std::map<obj_id, Event> event_container_t;

    event_container_t events_;

    void update(float t){
        for(auto& e:events_){e.second.update(t);}}

    void add(obj_id id, const std::function<bool (float)>& event){events_[id] = Event(event);}

    void remove(obj_id id){events_.erase(id);}

    template<class T>
    void add(T id, const std::function<bool (float)>& event){
        obj_id idc = (obj_id) id;
        events_[idc] = Event(event);}

    template<class T>
    void remove(T id){
        obj_id idc = (obj_id) id;
        events_.erase(idc);}

};

} // namespace glh

