/**\file glh_scenemanagement.h Scene rendering etc.
    \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/
#pragma once

#include "glbase.h"
#include "glh_names.h"
#include "shims_and_types.h"


namespace glh{

//struct Transform{
//
//    vec3       position_;
//    vec3       scale_;
//    quaternion rotation_;
//
//    Transform(){initialize();}
//
//    void initialize(){
//        position_ = vec3(0.f, 0.f, 0.f);
//        scale_    = vec3(1.f, 1.f, 1.f);
//        rotation_ = quaternion(1.f, 0.f, 0.f, 0.f);
//    }
//
//    mat4 matrix(){return generate_transform<float>(position_, rotation_, scale_).matrix();}
//};


typedef std::vector<std::string> PathArray;
typedef ExplicitTransform<float> Transform;

PathArray string_to_patharray(std::string str);

class SceneTree{
public:
    class Node{
    public:
        typedef std::vector<Node*> ChildContainer;

        mat4  local_to_world_;
        // Might have still need for local to parent transform in shaders?
        std::string name_;
        int         id_;
        int         parent_id_;
        bool        pickable_; // TODO: Preferably, remove from here (UI stuff)
        bool        interaction_lock_; // Lock for when dragged and so on. TODO: Preferably, remove from here (UI stuff)

        RenderEnvironment material_; // todo, pointer

        Transform transform_; // local to parent transform

        Box3f local_bounds_AAB_;
        Box3f world_bounds_AAB_;
        Box3f tree_world_bounds_AAB_; // Bounds of object and children in world coordinates

        FullRenderable* renderable_;

        Node(FullRenderable* renderable):renderable_(renderable){
            reset_data();}

        Node(FullRenderable* renderable, int id):renderable_(renderable), id_(id){
            reset_data();}

        Node():renderable_(0){
            reset_data();}

        void reset_data(){
            pickable_ = true;
            interaction_lock_ = false;
            transform_.initialize();

            local_to_world_ = mat4::Identity();
        }

        // TODO: Must update bounds!
        void set_renderable(FullRenderable* renderable){renderable_ = renderable;}

        bool has_renderable(){return renderable_ != 0;}

        FullRenderable* renderable(){return renderable_;}

        bool empty(){return children_.empty();}

        void add_child(Node* node){
            children_.push_back(node);
            node->parent_id_ = id_;
        }

        void remove_child(Node* node){
            erase(children_, node);
            node->parent_id_ = node->id_;
        }

        void update_transforms(const mat4& parent_local_to_world){
            local_to_world_ = parent_local_to_world * transform_.matrix();
            update_transforms();
        }

        void update_transforms(){
            for(auto c:children_) c->update_transforms(local_to_world_);
        }

        Box3f& update_bounds(){
            world_bounds_AAB_ = transform_box<float,3>(local_to_world_, local_bounds_AAB_);
            tree_world_bounds_AAB_ = world_bounds_AAB_;
            for(auto c:children_){
                tree_world_bounds_AAB_ = cover(c->update_bounds(), tree_world_bounds_AAB_);
            }

            return tree_world_bounds_AAB_;
        }

        Node* get_child(const std::string& name){
            Node* res = 0;
            foreach(children_, [&](Node* child){if(child->name_ == name){res = child;}});
            return res;
        }

        Node* get_child(const PathArray path){
            Node* cur = this;
            PathArray::const_iterator iter = path.begin();
            while(cur && iter != path.end()){
                cur = cur->get_child(*iter);
                iter++;
            }
            return cur;
        }

        ChildContainer::iterator begin(){return children_.begin();}
        ChildContainer::iterator end(){return children_.end();}

        ChildContainer children_;
    };

     //Traversal order: first visit the active node, then it's children by setting each as active in turn,
     // then pop stack
    class tree_iterator{
    public:

        struct Iterator{
            Node::ChildContainer::iterator i_;
            Node::ChildContainer::iterator end_;

            Iterator(Node* n):i_(n->begin()), end_(n->end()){}
            void next(){i_++;}
            bool valid(){return i_ != end_;}
            Node* value(){return *i_;}
        };

        std::stack<Iterator> stack_;
        Node*               current_;

        tree_iterator(Node* root):current_(root){}

        Node* operator*(){return current_;}
        Node* operator->(){return current_;}

        // Non-Stl-like utility function to get node parent
        Node* parent(){/*??? do we need this*/}

        Iterator& top(){return stack_.top();}

        void push(){stack_.push(Iterator(current_));}

        bool next_from_top(){
            bool has_value = top().valid();
            if(has_value){
                current_ = top().value();
                top().next();
            }
            return has_value;
        }

        void next_from_stack(){
            if(stack_.empty()){current_ = 0;}
            else if(!next_from_top()){
                stack_.pop();
                next_from_stack();}}

        void operator++(){
            if(current_){
                push();
                next_from_stack();}}

        bool operator!=(const tree_iterator& other){
            return current_ != other.current_;
        }
    };

    typedef tree_iterator iterator;

    SceneTree(){
       nodes_.push_back(Node(0));
       root_ = &nodes_.back();
       root_->id_ = id_generator_.new_id();
       root_->name_ = "root";
       vec4 color = ObjectRoster::color_of_id(root_->id_);
       root_->material_.set_vec4(UICTX_SELECT_NAME, color);
    }

    Node* add_node(Node* parent){
        Node* newnode;
        if(recycled_nodes_.empty()){
            nodes_.push_back(Node());
            newnode = &nodes_.back();
        }else{
            newnode = recycled_nodes_.back();
            recycled_nodes_.pop_back();
            newnode->reset_data();
        }
        newnode->id_ = id_generator_.new_id();
        // TODO: It's kinda hacky to create UI context colors here as well. Figure out a better way.
        vec4 color = ObjectRoster::color_of_id(newnode->id_);
        newnode->material_.set_vec4(UICTX_SELECT_NAME, color);
        if(parent) parent->add_child(newnode);

        id_to_node_[newnode->id_] = newnode;

        return newnode;
    }

    Node* add_node(Node* parent, FullRenderable* r){
        Node* newnode = add_node(parent);
        newnode->set_renderable(r);
        return newnode;
    }

    Node* root(){return root_;}

    // TODO: Compute transforms and bounding boxes
    void update(){
        root_->update_transforms();
        root_->update_bounds();
    }

    void apply_to_renderables(){
        for(auto& n: nodes_){
                n.material_.set_mat4(GLH_LOCAL_TO_WORLD, n.local_to_world_);}}

    iterator begin() {return tree_iterator(root_);}

    iterator end() {return tree_iterator(0);}

    void finalize(Node* node){
        node->children_.clear();
        recycled_nodes_.push_back(node);
        id_to_node_.erase(node->id_);
        id_generator_.release(node->id_);
    }

    Node* get(int id){
        auto n = try_get_value(id_to_node_, id);
        if(n) return *n;
        else return 0;
    }

    Node* get(const PathArray path){
        return root_->get_child(path);
    }

private:
    std::deque<Node, Eigen::aligned_allocator<Node>> nodes_;
    std::list<Node*> recycled_nodes_;

    std::map<int, Node*> id_to_node_;

    Node*                     root_;
    ObjectRoster::IdGenerator id_generator_;
};

SceneTree::iterator begin_iter(SceneTree::Node* node);
SceneTree::iterator end_iter(SceneTree::Node* node);

///////////// Utility functions for scene elements /////////////

void set_material(SceneTree::Node& node, RenderEnvironment& material);
void set_material(SceneTree::Node& node, cstring& name, const vec4& var);
void set_material(SceneTree::Node& node, cstring& name, const float var);

///////////// RenderQueue //////////////

class RenderQueue{
public:

    typedef ArenaQueue<SceneTree::Node*> node_ptr_sequence_t;

    typedef std::function<bool(SceneTree::Node*)> node_filter_fun_t;

    RenderQueue(){}

    void add(SceneTree::Node* node){
        renderables_.push(node);}

    void add(SceneTree& scene){
        for(SceneTree::Node* node: scene){
            if(node->renderable()) add(node);}}

    void add(SceneTree& scene, node_filter_fun_t filter){
        for(SceneTree::Node* node: scene){
            if(node->renderable() && filter(node)) add(node);}}

    void add_rec(SceneTree::Node* noderoot){
        auto first = begin_iter(noderoot);
        auto last  = end_iter(noderoot);
        for(auto node = first; node != last; ++node){
            if(node->renderable()) add(*node);}}

    void add_rec(SceneTree::Node* noderoot, node_filter_fun_t filter){
        auto first = begin_iter(noderoot);
        auto last  = end_iter(noderoot);
        for(auto node = first; node != last; ++node){
            if(node->renderable() && filter(*node)) add(*node);}}


    // Render items in queue
    void render(GraphicsManager* manager, glh::RenderEnvironment& env){
        for(auto n:renderables_){
            manager->render(*n->renderable_, n->material_,env);}}

    // Render items in queue by overloading program
    void render(GraphicsManager* manager, ProgramHandle& program, glh::RenderEnvironment& env){
        for(auto n:renderables_){
            manager->render(*n->renderable_, program, n->material_, env);}}

    void clear(){renderables_.clear();}

    node_ptr_sequence_t::iterator begin(){
        return renderables_.begin();}

    node_ptr_sequence_t::iterator end(){
        return renderables_.end();}

    node_ptr_sequence_t renderables_;
};


/////////////////////// Camera ///////////////////////

class Camera{
public:
    SceneTree::Node* node; // word_to_camera

    mat4             view_to_screen; // To NDC/Pixel coordinates
    mat4             camera_to_view;  // Projection

};

/////////////////////// RenderPass ///////////////////////

class RenderPass{
public:
    RenderPassSettings settings_;
    RenderQueue        queue_;
    std::string        root_path_;
    RenderEnvironment  env_;

    Camera* camera_;

    RenderPass(){}

    void update_queue(SceneTree& scene){
        queue_.clear();
        auto root = scene.get(string_to_patharray(root_path_));
        if(root){queue_.add(root);}
    }

    void camera_parameters_to_env(){
        // Take 
        // world_to_camera, camera_to_view and view_to_screen matrices
        // to env.
    }

    void render_queue(GraphicsManager* gm){
        apply(settings_);
        
        queue_.render(gm, env_);
    }
};

///////////// UiEvents //////////////

// Tree of widgets, operation wise. Not a spatial tree, but context tree?
// 

//class UiElement{
//public:
//    virtual void focus_gained() = 0;
//    virtual void focus_lost() = 0;
//    
//    virtual void button_activate() = 0;
//    virtual void button_deactivate() = 0;
//};

class UiEntity{
public:
    // TODO: Do not store selection color in node material?
    static void render(SceneTree::Node* node, GraphicsManager& gm, ProgramHandle& selection_program, RenderEnvironment& env){
        FullRenderable* r = node->renderable();
        if(r){
            gm.render(*r, selection_program, node->material_, env);
        }
    }
};

typedef std::shared_ptr<UiEntity> UiEntityPtr; 


/** User interface state. 

Usage (refactor):
0. Scene rendering: scene graph is flattened to render queue (per frame?)
1. Each UiEntity is assigned a FullRenderable entity (or just a color - a texture
should be renderable in selection pass)
2. If a UiEntity is assigned a FullRenderable and it is attached to a RenderPicker,
The FullRenderable environment is given a selection color (overriding any properties there, if any)
3. When rendering selection scene, the same mechanism is used as per visible scene. The only
difference is that the program for the FullRenderables is overloaded with the selection rendering
program.
*/

class RenderPicker{
public:

    // TODO: Enable several pickers to select items in same frame
    // In this case set scissor bounds to full screen, render once,
    // then for each picker location pick id.

    typedef ArenaQueue<SceneTree::Node*> node_ptr_container_t;

    struct PickedContext{
        RenderPicker& picker_;
        PickedContext(RenderPicker& picker):picker_(picker){}

        node_ptr_container_t::iterator begin(){return picker_.picked_.begin();}
        node_ptr_container_t::iterator end(){return picker_.picked_.end();}
    };

    RenderPicker(App& app):app_(app){}

    void add_node(SceneTree::Node* node){
        id_to_entity_[node->id_] = node;
    }

    void detach_node(SceneTree::Node* node){
        int id = node->id_;
        id_to_entity_.erase(id);
    }

     void attach_render_queue(RenderQueue* queue){
         render_queue_ = queue;
         for(auto r:(*render_queue_)) add_node(r);
    }

     void detach_queue(){
         render_queue_ = 0;
         for(auto r:(*render_queue_)) detach_node(r);
    }

    // TODO: To support multiple pointer handling, either
    // a) calculate bounding box for all pointers and render the entire box
    // b) render scene multiple times with scissors set around each pointer
    // We probably need some sane test load for profiling before making the change
    // and figure out which technique we want to use based on the findings.
    std::tuple<Box<int,2>, bool> setup_context(int pointer_x, int pointer_y){
         // set up scene
         RenderPassSettings settings(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT,
                                            glh::vec4(0.0f,0.0f,0.0f,1.f), 1);
         // Constrain view box
        const int w = app_.config().width;
        const int h = app_.config().height;

        Box<int,2> screen_bounds = make_box2(0, 0, w, h);
        int mousey = h - pointer_y;
        Box<int,2> mouse_bounds  = make_box2(pointer_x - 1, mousey - 1, pointer_x + 2, mousey + 2);
        Box<int,2> read_bounds;
        bool       bounds_ok;

        std::tie(read_bounds, bounds_ok) = intersect(screen_bounds, mouse_bounds);
        auto read_dims = read_bounds.size();

        glDisable(GL_SCISSOR_TEST);
        
        apply(settings);

        glEnable(GL_SCISSOR_TEST);
        glScissor(read_bounds.min_[0], read_bounds.min_[1], read_dims[0], read_dims[1]);

        return std::make_tuple(read_bounds, bounds_ok);
    }

    int do_picking(Box<int,2>& read_bounds){
        auto read_dims = read_bounds.size();
        GLenum read_format = GL_BGRA;
        GLenum read_type = GL_UNSIGNED_INT_8_8_8_8_REV;
        const int pixel_count = 9;
        const int pixel_channels = 4;
        uint8_t imagedata[pixel_count * pixel_channels] = {0};
        glFlush();
        glReadBuffer(GL_BACK);
        glReadPixels(read_bounds.min_[0], read_bounds.min_[1], read_dims[0], read_dims[1], read_format, read_type, imagedata);

        const uint8_t* b = &imagedata[4 * pixel_channels];
        const uint8_t* g = b + 1;
        const uint8_t* r = g + 1;
        const uint8_t* a = r + 1;

        //char buf[1024];
        //sprintf(buf, "r%hhu g%hhu b%hhu a%hhu", *r, *g, *b, *a);
        //std::cout << "Mouse read:" << buf << std::endl;

        return ObjectRoster::id_of_color(*r,*g,*b);
    }

    void reset_context(){
         glDisable(GL_SCISSOR_TEST);
    }

    PickedContext render_selectables(RenderEnvironment& env, int pointer_x, int pointer_y){
        GraphicsManager* gm = app_.graphics_manager();
        Box<int,2> read_bounds;
        bool bounds_ok;

        picked_.clear();

        std::tie(read_bounds, bounds_ok) = setup_context(pointer_x, pointer_y);

        if(bounds_ok){
            for(auto node: (*render_queue_)){
                if(node->pickable_) UiEntity::render(node, *gm, *selection_program_, env);}

            //Once all the items are rendered do picking
            selected_id_ = do_picking(read_bounds);
        }

        reset_context();

        SceneTree::Node* picked = 0;

        auto ie = id_to_entity_.find(selected_id_);
        if(ie != id_to_entity_.end()){
            picked = ie->second;

            if(picked == 0) throw GraphicsException("Trying to pick null entity!");

            picked_.push(picked);
        }

        return PickedContext(*this);
    }

    RenderQueue*                    render_queue_;
    std::map<int, SceneTree::Node*> id_to_entity_;

    ObjectRoster::IdGenerator idgen_;

    App& app_;

    ProgramHandle* selection_program_;

     node_ptr_container_t picked_;

    int selected_id_;
};

typedef std::shared_ptr<RenderPicker> RenderPickerPtr;


/** Stored picker states etc. */
class FocusContext{
public:

    typedef SortedArray<SceneTree::Node*> entity_container_t;

    struct Focus{
        FocusContext& ctx_;

        Focus(FocusContext& ctx):ctx_(ctx){}
        ~Focus(){}

        void on_focus(SceneTree::Node* e){
            ctx_.currently_focused_.insert(e);
        }

        void update_event_state(){
            ctx_.currently_focused_.set_difference(ctx_.previously_focused_, ctx_.focus_gained_);
            ctx_.previously_focused_.set_difference(ctx_.currently_focused_, ctx_.focus_lost_);

            for(auto& entity:ctx_.focus_lost_){
                ctx_.previously_focused_.erase(entity);}
            for(auto& entity:ctx_.focus_gained_){
                ctx_.previously_focused_.insert(entity);}

            ctx_.event_handling_done_ = true;
        }

    };

    Focus start_event_handling(){
        event_handling_done_ = false;
        focus_gained_.clear();
        focus_lost_.clear();
        currently_focused_.clear();
        return Focus(*this);
    }

    entity_container_t previously_focused_;
    entity_container_t currently_focused_;
    entity_container_t focus_gained_;
    entity_container_t focus_lost_;

    bool event_handling_done_ ;
};

// TODO remove below
//class TransformGadget{
//public:
//
//    glh::App& app_;
//
//    TransformGadget(glh::App& app):app_(app){
//        // Transforms mouse movement to normalized device coordinates.
//    }
//
//     //TODO: Into a graph node. Graph drives Gadget transform by forwarding mouse/pointer/vec source data
//    // to it. TransformableSet (add definition and instance) is then transformed through the gadget transform.
//    // Items are removed from transformable set when items are removed from selection.
//    void mouse_move_node(vec2i mouse_delta, glh::SceneTree::Node* node)
//    {
//        glh::mat4 screen_to_view   = app_orthographic_pixel_projection(&app_);
//        glh::mat4 view_to_world    = glh::mat4::Identity();
//        glh::mat4 world_to_object  = glh::mat4::Identity();
//        glh::mat4 screen_to_object = world_to_object * view_to_world * screen_to_view ;
//    
//        // should be replaced with view specific change vector...
//        // projection of the view change vector onto the workplane...
//        // etc.
//    
//        glh::vec4 v((float) mouse_delta[0], (float) mouse_delta[1], 0, 0);
//        glh::vec3 node_pos_delta = glh::decrease_dim<float, 4>(screen_to_object * v);
//    
//        node->transform_.position_ = node->transform_.position_ + node_pos_delta;
//    }
//
//};


} // namespace glh
