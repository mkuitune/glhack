/**\file glh_scenemanagement.h Scene rendering etc.
    \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/
#pragma once

#include "glbase.h"
#include "glh_names.h"
#include "shims_and_types.h"

namespace glh{


class SceneTree{
public:
    class Node{
    public:
        typedef std::vector<Node*> ChildContainer;

        mat4  local_to_world_;
        mat4  local_to_parent_;

        std::string name_;

        RenderEnvironment material_;

        vec3       location_;
        vec3       scale_;
        quaternion rotation_;

        Box3f local_bounds_AAB_;
        Box3f world_bounds_AAB_;
        Box3f tree_world_bounds_AAB_; // Bounds of object and children in world coordinates

        FullRenderable* renderable_;

        Node(FullRenderable* renderable):renderable_(renderable){
            reset_data();
        }

        Node():renderable_(0){
            reset_data();
        }

        void reset_data(){

            location_ = vec3(0.f, 0.f, 0.f);
            scale_    = vec3(1.f, 1.f, 1.f);
            rotation_ = quaternion(1.f, 0.f, 0.f, 0.f);

            local_to_world_ = mat4::Identity();
            local_to_parent_ = mat4::Identity();
        }

        // TODO: Must update bounds!
        void set_renderable(FullRenderable* renderable){renderable_ = renderable;}

        bool has_renderable(){return renderable_ != 0;}

        FullRenderable* renderable(){return renderable_;}

        bool empty(){return children_.empty();}

        void add_child(Node* node){
            children_.push_back(node);
        }

        void update_transforms(const mat4& parent_local_to_world){

            transform3 t = generate_transform<float>(location_, rotation_, scale_);

            local_to_parent_ = t.matrix();

            local_to_world_ = local_to_parent_ * parent_local_to_world;
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
        Node* parent(){}

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
                next_from_stack();
            }
        }

        void operator++(){
            if(current_){
                push();
                next_from_stack();
            }
        }

        bool operator!=(const tree_iterator& other){
            return current_ != other.current_;
        }
    };

    typedef tree_iterator iterator;

    SceneTree(){
       nodes_.push_back(Node(0));
       root_ = &nodes_.back();
    }

    Node* add_node(Node* parent){
        nodes_.push_back(Node());
        Node* newnode = &nodes_.back();
        if(parent) parent->add_child(newnode);
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

private:
    std::deque<Node, Eigen::aligned_allocator<Node>> nodes_;
    Node*           root_;
};

///////////// Utility functions for scene elements /////////////

void set_material(SceneTree::Node& node, RenderEnvironment& material);
void set_material(SceneTree::Node& node, cstring& name, const vec4& var);

///////////// RenderQueue //////////////

class RenderQueue{
public:

    RenderQueue(){}

    void add(SceneTree::Node* node){
        renderables_.push(node);}

    void add(SceneTree& scene){
        for(SceneTree::Node* node: scene){
            if(node->renderable()) add(node);}}

    // Render items in queue
    void render(GraphicsManager* manager, glh::RenderEnvironment& env){
        for(auto n:renderables_){
            manager->render(*n->renderable_, n->material_,env);}}

    // Render items in queue by overloading program
    void render(GraphicsManager* manager, ProgramHandle& program, glh::RenderEnvironment& env){
        for(auto n:renderables_){
            manager->render(*n->renderable_, program, n->material_, env);}}

    void clear(){renderables_.clear();}

    ArenaQueue<SceneTree::Node*> renderables_;
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

    SceneTree::Node* node_;

    UiEntity(SceneTree::Node* node):node_(node){}

    // TODO: Do not store selection color in node material?
    void render(GraphicsManager& gm, ProgramHandle& selection_program, RenderEnvironment& env){
        FullRenderable* r = node_->renderable();
        if(r){
            gm.render(*r, selection_program, node_->material_, env);
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

    typedef std::vector<UiEntity*> uientity_container_t;

    struct PickedContext{
        RenderPicker& picker_;
        PickedContext(RenderPicker& picker):picker_(picker){}

        uientity_container_t::iterator begin(){return picker_.picked_.begin();}
        uientity_container_t::iterator end(){return picker_.picked_.end();}
    };

    RenderPicker(App& app):app_(app){}

    void add(UiEntity* e){
        int id = idgen_.new_id();
        vec4 color = ColorSelection::color_of_id(id);
        e->node_->material_.set_vec4(UICTX_SELECT_NAME, color);
        entity_to_id_[e] = id;
        id_to_entity_[id] = e;
    }

     void remove(UiEntity* e){
        int id = entity_to_id_[e];
        entity_to_id_.erase(e);
        idgen_.release(id);
        e->node_->material_.remove(UICTX_SELECT_NAME);
    }

    std::tuple<Box<int,2>, bool> setup_context(){
         // set up scene
         RenderPassSettings settings(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT,
                                            glh::vec4(0.0f,0.0f,0.0f,1.f), 1);
         // Constrain view box
        const int w = app_.config().width;
        const int h = app_.config().height;

        Box<int,2> screen_bounds = make_box2(0, 0, w, h);
        int mousey = h - pointer_y_;
        Box<int,2> mouse_bounds  = make_box2(pointer_x_ - 1, mousey - 1, pointer_x_ + 2, mousey + 2);
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

        return ColorSelection::id_of_color(*r,*g,*b);
    }

    void reset_context(){
         glDisable(GL_SCISSOR_TEST);
    }

    PickedContext render_selectables(RenderEnvironment& env){
        GraphicsManager* gm = app_.graphics_manager();
        Box<int,2> read_bounds;
        bool bounds_ok;

        picked_.clear();

        std::tie(read_bounds, bounds_ok) = setup_context();

        if(bounds_ok){
            for(auto& id_uientity: entity_to_id_){
                id_uientity.first->render(*gm, *selection_program_, env);}

            //Once all the items are rendered do picking
            selected_id_ = do_picking(read_bounds);
        }

        reset_context();

        UiEntity* picked = 0;

        auto ie = id_to_entity_.find(selected_id_);
        if(ie != id_to_entity_.end()){
            picked = ie->second;

            if(picked == 0) throw GraphicsException("Trying to pick null entity!");

            picked_.push_back(picked);
        }

        return PickedContext(*this);
    }

    void pointer_move_cb(int x, int y){
       pointer_x_ = x;
       pointer_y_ = y;
    }

    static std::function<void(int,int)> get_pointer_move_cb(RenderPicker& ctx){
        using namespace std::placeholders;
        return std::bind(&RenderPicker::pointer_move_cb, ctx, _1, _2);
    }

    int pointer_x_;
    int pointer_y_;

    std::map<UiEntity*, int> entity_to_id_;
    std::map<int, UiEntity*> id_to_entity_;

    ColorSelection::IdGenerator idgen_;

    App& app_;

    ProgramHandle* selection_program_;

    uientity_container_t picked_;

    int selected_id_;
};

typedef std::shared_ptr<RenderPicker> RenderPickerPtr;


/** Stored picker states etc. */
class FocusContext{
public:

    typedef SortedArray<UiEntity*> entity_container_t;

    struct Focus{
        FocusContext& ctx_;

        Focus(FocusContext& ctx):ctx_(ctx){}
        ~Focus(){}

        void on_focus(UiEntity* e){
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


} // namespace glh
