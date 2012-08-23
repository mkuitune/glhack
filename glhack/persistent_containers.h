/** \file persistent_containers.h Persistent containers.
 *
 * Since C++ does not contain a standard garbage collector and our intent is not to use the persistent
 * data structures universally, each datastructure has it's own allocator and implementation of
 * garbage collection interface that's required to call explicitly. 
 *
 * Then plan is to implementent:
 *  - persistent list PersistentList
 *      - a simple linked list with distinct heads nodes for reference counting
 *  - persistent map PersistentMap
 *      - a simple persistent map with node copying
 *  - persistent vector PersistentVector
 *      - an implementation similar to Clojure's persistent vector
 *  - persistent hashmap PersistentUnorderedMap
 *      - an implementation similiar to Clojure's
 *
 * TODO: Should be straightforward to modify chunkbuffer so it supports parallel deallocation.
 *
 * For parallel marking, a portion of the chunks must be 'locked' so no new space can be allocated
 * from them prior to collecting. So, to parallellize:
 *
 * 1) divide the chunks in chunkbuffer to gc-groups.
 * 2) one gc-group is selected for marking and deallocation (mad)
 * 3) during mad the gc-group is locked - no new slots can be allocated from it's chunks
 *    (otherwise a slot could be reserved from a chunk that's alread gone through marking and would be
 *     erroneously collected even though it should be reachable)
 *  4) pass marked gc-group for deallocation
 *  5) once deallocated, release gc-group once more into complete allocation pool
 *
 *  There could be, say, two or three allocation groups? TODO: Needs more attention.
 *
 * Once locked, the marking phase and deallocation phase can run in separate threads.
 * 1) Main thread locks allocation group g for mad (markeds pushed to concurrent filo queue)
 * 2) marking thread gets the next item from marking queue, marks them, and adds them to concurrent cleanup -queue
 * 3) deallocation thread gets the next item from cleanup -queue, deallocates unused slots and unlocks the allocation group
 *
 * Should all types of chunkbuffers implement a collection -interface so two mark and deallocation queues can be used for all types of chunkbuffers.
 * The mark thread should contain different marking implementations for all data structures (i.e. how to navigate a hash array trie forest v.s. a list forest)
 * */

#pragma once


#include "math_tools.h"
#include "shims_and_types.h"

#include<unordered_map>
#include<functional>
#include<sstream>

namespace glh{

#define CHUNK_BUFFER_SIZE 32

/** Chunk. Can be used only for storing classes with parameterless constructor and a destructor.*/
template<class T>
struct Chunk
{
    unsigned char buffer[CHUNK_BUFFER_SIZE * sizeof(T)];

    /** Used for storing the allocation state of buffer, with array position matching bit position
     *  1<<i : buffer[i]
     *  i.e the expression
     *  used_elements = 1<<n means buffer[n] is allocated.*/
    uint32_t used_elements;
    uint32_t mark_field; // use for garbage collection
          
    Chunk*   next;

    Chunk(){
        used_elements = 0;
        mark_field = 0;
        next = 0;
        memset(buffer, 0 , CHUNK_BUFFER_SIZE * sizeof(T));
    }

    bool is_full(){return (used_elements == 0xffffffff);}

    T* get_new()
    {
        if(is_full()) return 0;
        uint32_t index = lowest_unset_bit(used_elements);
        used_elements = set_bit_on(used_elements, index);
        T* address = ((T*)buffer) + index;
        T* t = new(address)T();
        return t;
    }

    T* get_new_array(const size_t count)
    {
        T* result = 0;

        if(count > CHUNK_BUFFER_SIZE) return result;

        // Return pointer to start of array if enough consecutive slots are found.
        // Try to find a position where there are count free bits-
 
        // First make a mask such as 000001111 where the number of set fields is equal to the
        // number of elements stored. 
        uint32_t mask = 0;
        uint32_t i = count;
        while(i--) mask |= 1<<i;
        
        uint32_t positions_to_search = CHUNK_BUFFER_SIZE - count + 1;
        uint32_t array_start_index = 0;

        while(positions_to_search--)
        {
            if((used_elements & mask) == 0)
            {
                result = buffer + array_start_index*sizeof(T);
                T* tmp;
                for(size_t i = 0; i  < count; ++i) tmp = new(result + i * sizeof(T))T();
                used_elements |= mask;
                break;
            }
            mask =<< 1;
            array_start_index++
        }

        return result;
    }

    void free_element(T* ptr)
    {
        uint32_t index = ptr - buffer;
        // Note: if ptr < buffer, then index will wrap (to a very large number >> CHUNK_BUFFER_SIZE)
        // and the following clause will be false.
        if(index < CHUNK_BUFFER_SIZE)
        {
            used_elements = set_bit_off(used_elements, index);
        }
    }

    /** Free 'count' number of positions starting at ptr.*/
    void free_array(T* ptr, const size_t count)
    {
        for(size_t i = 0; i < count; ++i) free_element(ptr + i);
    }
    
    void set_used(T* ptr)
    {
        uint32_t index = ptr - ((T*)buffer);
        // Note: if ptr < buffer, then index will wrap (to a very large number >> CHUNK_BUFFER_SIZE)
        // and the following clause will be false.
        if(index < CHUNK_BUFFER_SIZE)
        {
            used_elements = set_bit_on(used_elements, index);
        }
    }
    
    void set_marked(T* ptr)
    {
        uint32_t index = ptr - ((T*)buffer);
        // Note: if ptr < buffer, then index will wrap (to a very large number >> CHUNK_BUFFER_SIZE)
        // and the following clause will be false.
        if(index < CHUNK_BUFFER_SIZE)
        {
            mark_field = set_bit_on(mark_field, index);
        }
    }

    T* begin(){return buffer;}
    T* end(){return buffer + CHUNK_BUFFER_SIZE;}

    bool contains(const T* ptr)
    {
        return ptr >= ((T*) buffer) &&
               ptr < (((T*) buffer) + CHUNK_BUFFER_SIZE);
    }

    void reset_marks(){mark_field = 0;}

    bool set_marked_if_contains(T* elem)
    {
        bool result = false;
        if(contains(elem))
        {
            set_marked(elem);
            result = true;
        }
        return result;
    }
    
    bool set_marked_if_contains_array(T* start, const size_t count)
    {
        bool result = false;
        if(contains(start) && ((start - buffer) + count) <= BUFFER_SIZE)
        {
            for(size_t i = 0; i < count; ++i) set_marked(start + i);

            result = true;
        }
        return result;
    }

    /** For each used/marked bit */
    void collect_marked()
    {
        uint32_t is_used = (used_elements ^ mark_field) & used_elements;
                              // 1 ^ 0 = 1
        for(int index = 0; index < CHUNK_BUFFER_SIZE; ++index)
        {
            // If field is used but unmarked as active
            if((is_used >> index)&0x1)
            {
                // Set slot unused and call destructor on allocated memory
                used_elements = set_bit_off(used_elements, index);
                T* t = &((T*)buffer)[index];
                t->~T();
            }
        }
    }

#if 0
    /** Use this for marking used elements. @returns true if element is contained in chunk.*/
    bool set_used_if_contains(T* elem)
    {
        bool result = false;
        if(contains(elem))
        {
            set_used(elem);
            result = true;
        }
        return result;
    }

    bool set_used_if_contains_array(T* start, const size_t count)
    {
        bool result = false;
        if(contains(start) && ((start - buffer) + count) <= BUFFER_SIZE)
        {
            for(size_t i = 0; i < count; ++i) set_used(start + i);

            result = true;
        }
        return result;
    }
#endif
};


template<class T>
class ChunkBox
{
public:
    typedef Chunk<T>                        chunk_type;
    typedef std::list<chunk_type>           chunk_container;
    typedef typename std::list<chunk_type>::iterator iterator;
    
    ChunkBox()
    {
        free_chunks_ = new_chunk();
    }

    chunk_type* new_chunk()
    {
        chunks_.push_back(chunk_type());
        return &chunks_.back();
    }

    T* reserve_element()
    {
        T* elem = 0;

        if(free_chunks_)
        {
            elem = free_chunks_->get_new();

            // Check if free chunks is still free or do we need new chunks
            if(free_chunks_->is_full())
            {
                if(free_chunks_->next)
                {
                    free_chunks_ = free_chunks_->next;
                }
                else
                {
                    free_chunks_ = new_chunk();
                }
            }
        }

        return elem;
    }

    /** Try to reserve element_count new consecutive elements. 
        @param element_count number of consecutive elements to reserve 
        @result First element in the allocated sequence or null*/
    T* reserve_consecutive_elements(const size_t element_count)
    {
        T* result = 0;
        if(element_count <= CHUNK_BUFFER_SIZE)
        {
            chunk_type* chunk = free_chunks_;
            chunk_type* prev_chunk = 0;
            while(chunk)
            {
                if(result = chunk->get_new_array(element_count)) 
                {
                    // Check if chunk still has space left and if not maintain free node list
                    if(chunk->is_full())
                    {
                        if(!prev_chunk && !chunk->next)
                        {
                            free_chunks_ = new_chunk();
                        }
                        else if(!prev_chunk && chunk->next)
                        {
                            free_chunks_ = chunk->next;
                        }
                        else if(prev_chunk && !chunk->next)
                        {
                            prev_chunk->next = 0;
                        }
                        else // prev_chunk && chunk->next
                        {
                            prev_chunk->next = chunk->next;
                        }
                   }

                   break;
               }
               else
               {
                   prev_chunk = chunk;
                   chunk = chunk->next;
               }
            }

            if(!chunk)
            {
                // Did not find a suitable chunk. Create a new chunk and add it to the front of the
                // free list if the array does not consume it completely.
                chunk_type* created_chunk = new_chunk();

                if(element_count < CHUNK_BUFFER_SIZE)
                {
                    created_chunk->next = free_chunks_;
                    free_chunks_ = created_chunk;
                }

                // At this point we must know the operation cannot fail.
                result = chunk->get_new_array(element_count);
            }
        }

        return result;
    }

    void refresh_free_chunk_list()
    {
        free_chunks_ = 0;
        for(auto chunk = chunks_.begin(); chunk != chunks_.end(); ++chunk)
        {
            if(!chunk->is_full())
            {
                chunk->next = free_chunks_;
                free_chunks_ = &(*chunk);
            }
        }
    }

    void mark_all_empty()
    {
        foreach(chunks_, [](chunk_type& c){c.used_elements = 0;});
    }

    void reset_marks()
    {
        foreach(chunks_, [](chunk_type& c){c.reset_marks();});
    }

    /** After the chunks have been marked, collect the unused memory in all of them. If 
     * chunk has been full and has some memory freed move it to the free_chunks_ list.*/
    void collect_chunks()
    {
        for(auto chunk = begin(); chunk != end(); ++chunk)
        {
            uint32_t pre_used_elements = chunk->used_elements;
            chunk->collect_marked();

            if(chunk->used_elements != pre_used_elements && pre_used_elements == CHUNK_BUFFER_SIZE)
            {
                chunk->next = free_chunks_;
                free_chunks_ = &(*chunk);
            }
        }
    }

    iterator begin(){return chunks_.begin();}
    iterator end(){return chunks_.end();}

    chunk_container& chunks(){return chunks_;}
    chunk_type* free_chunks(){return free_chunks_;}

private:
    chunk_container chunks_;
    chunk_type*     free_chunks_;
};


/////////// Persistent list //////////////

/** Pool manager and collector for persistent lists. 
 *  Not a particularly efficient implementation. */
template<class T>
class PersistentListPool
{
public:

    /** List node. */
    struct Node
    {
        Node* next;
        T     data;
    };

    /** Stores head to List */
    class List
    {
    public:
        struct iterator
        {
            Node* node;
            iterator():node(0){}
            iterator(Node* n):node(n){}
            const T& operator*() const {return node->data;}
            T& operator*() {return node->data;}
            const T& operator->() const {return node->data;}
            T* data_ptr() {return &node->data;}
            void operator++(){if(node) node = node->next;}
            bool operator!=(const iterator& i){return node != i.node;}
            bool operator==(const iterator& i){return node == i.node;}
        };

    public:

        List(PersistentListPool& pool, Node* head):pool_(pool), head_(head){if(head_) pool_.add_ref(head_);}
        ~List(){if(head_) pool_.remove_ref(head_);}
       
        List(const List& old_list):pool_(old_list.pool_), head_(old_list.head_)
        {
            if(head_) pool_.add_ref(head_);
        }

        List(List&& temp_list):pool_(temp_list.pool_), head_(temp_list.head_)
        {
            temp_list.head_ = 0;
        }

        List& operator=(const List& list)
        {
            if(this != &list)
            {
                head_ = list.head_;
                pool_ = list.pool_;
                if(head_) pool_.add_ref(head_);
            }
            return *this;
        }

        List& operator=(List&& list)
        {
            if(this != &list)
            {
                head_ = list.head_;
                pool_ = list.pool_;
                list.head_ = 0;
            }
            return *this;
        }

        /** Find first element from list matching with predicate or return end. */ 
        iterator find(const List* list, std::function<bool(const T&)>& pred)
        {
            Node* n = head_;
            while(n && ! pred(n->data)) n = n->next;
            return iterator(n);
        }

        /** Add element to list */
        List add(const List& list, const T& data)
        {
            Node* n = pool_.new_node(data);
            n->next = list->head_; // prepend new element to head
            return List(pool_, n);
        }

        /** Remove element from list. */
        List remove(const List& list, const iterator& i)
        {
            // Neither head nor node to remove can be null.
            if(i.node != 0 && head_)
            {
                // If node is first, just return a list starting from next node.
                if(n == i.node) return List(pool_, n->next);

                // Otherwise must duplicate nodes prior to node to remove
                // [a]->[b]->[c]->[d]
                //
                //[a']->[b']->[d]

                // find position
                Node* n        = head_;
                Node* new_head = pool_.copy(n);;
                Node* build    = new_head;

                n = n->next;

                while(n && n != i.node)
                {
                    build->next = pool_.copy(n);
                    build = build->next;
                    n = n->next;
                }
                
                // Finally tie to the next cell from to be removed
                build->next = i.node->next;

                return List(pool_, new_head);
            }
        }

        iterator begin(){return iterator(head_);}
        iterator end(){return iterator(0);}

        const size_t size() const
        {
            size_t s = 0;
            Node* n = head_;
            while(n){ n = n->next; s++;}
            return s;
        }

    private:
        PersistentListPool& pool_;
        Node*               head_; 
    };

    typedef Chunk<Node>                    node_chunk;
    typedef ChunkBox<Node>                 node_chunk_box;

    typedef std::unordered_map<Node*, int> ref_count_map;
    
    PersistentListPool()
    {
    }

    ~PersistentListPool()
    {
        // Deleting ListPool before the end of the lifetime of all heads will result
        // in undefined behaviour.
    }

    /** Create new empty list. */
    List new_list()
    {
        return List( *this, 0);
    }

    /** Remove reference to node */
    void remove_ref(Node* n)
    {
        int* ref_count = try_get_value(ref_count_, n);
        if(ref_count && (*ref_count > 0)) --(*ref_count);
    }

    /** Add reference to node*/
    void add_ref(Node* n)
    {
        if(has_key(ref_count_, n)) ref_count_[n]++;
        else ref_count_[n] = 1;
    }

    /** Create new list from stl compatible container. */
    template<class Cont>
    List new_list(const Cont& container)
    {
        if(container.size() == 0) return List(*this, 0);

        auto i = container.begin();
        Node* head = new_node(*i);
        Node* node = head;
        i++;

        for(;i != container.end(); ++i)
        {
            node->next = new_node(*i);
            node = node->next;
        }

        return List( *this, head);
    }

    /** Create new list from a number of input values. */
    List new_list(const T& a)
    {
        Node* head = new_node(a);

        return List( *this, head);
    }

    /** Create new list from a number of input values. */
    List new_list(const T& a, const T& b)
    {
        Node* head = new_node(a);
        Node* node = head;

        node->next = new_node(b);

        return List( *this, head);
    }

    /** Create new list from a number of input values. */
    List new_list(const T& a, const T& b, const T& c)
    {
        Node* head = new_node(a);
        Node* node = head;

        node->next = new_node(b);
        node = node->next;

        node->next = new_node(c);

        return List( *this, head);
    }

    // Return duplicate of existing node sans the links
    Node* copy(Node* n)
    {
        Node* c = 0;
        if(n)
        {
           c = new_node(n->data); 
        }
        return c; 
    }

    Node* new_node(const T& data)
    {
        Node* n = chunks_.reserve_element();
        n->data = data;
        n->next = 0;
        return n;
    }

    void mark_referenced(Node* node)
    {
        auto chunks_end = chunks_.end();
        auto chunk_iterator = chunks_end;

        // Follow referenced nodes
        while(node)
        {
            // Store reference to previous chunk visited. If there is not too much
            // fragmentation there is a fair chance that nodes have been reserved
            // linearly and that the previous chunk contains this node.

            if(chunk_iterator != chunks_end && chunk_iterator->set_marked_if_contains(node))
            {
                node = node->next;
                continue;
            }

            // Find the chunk that contains the node and label used.
            for(chunk_iterator = chunks_.begin(); chunk_iterator != chunks_end; ++chunk_iterator)
            {
                if(chunk_iterator->set_marked_if_contains(node)) break;
            }

            node = node->next;
        }   
    }
    
    // Collect all slots taken by unvisitable nodes //TODO: 
    void gc()
    {
        // Currently bit expensive - the cost is 
        //    constant * block_count (free all nodes) + m * block_count/2 (mark all visited nodes)
        // where m is the number of live nodes in all the lists

        // First mark all as empty
        chunks_.mark_all_empty();

        // Then clean up unused references, visit all heads and mark visited nodes as active
        ref_count_ = copyif(ref_count_, [](const std::pair<Node*, int>& p){return p.second > 0;});

        // Now ref_count_ contains as keys the head nodes of active lists.
        for(auto r = ref_count_.begin(); r != ref_count_.end(); ++r) mark_referenced(r->first);

        // Lastly, go through the blocks, deallocate free's slots and move chunks to free list
        // if space became available on a full one
        chunks_.collect_chunks();
    }

private:
    node_chunk_box        chunks_;
    ref_count_map         ref_count_;   //> Head node reference counts

};


/////////// Persistent map //////////////

#if 1
/** A persistent, self garbage-collecting version of Bagwell's persistent hash tries.
    Hash key: uint32_t

    The key is split to five 5-bit length fields and one 2-bit length field as

level  0     1     2     3     4     5   6
    |aaaaa|bbbbb|ccccc|ddddd|eeeee|fffff|gg
     0-31  0-31  0-31  0-31  0-31  0-31  0-3

    where each level give the index to use at the child-node array at the given level.


    Node: used_field: uint32_t
          ref_array = Ref * [huffmanlength(used_field)]

    Each node contains 32 slots for children.

    The ref_array is allocated only to the huffman length of the used_field (i.e. numbers of bits 1
    in the field).

    Each element in ref-array stores either: a) A reference to a child-node, b) A reference to the stored element or c) A reference to a list of stored elements in
    case the hash-key is overwritten.

    The datum for the element is not stored inside the node, only a reference to it.

    The storage used by a node _without the stored elements_ is 

    4 bytes (used_field) + 8 bytes (ref_array pointer) + elem_count(1 to 32) * 8 (ref_array contents) -> 20 to 268 bytes

    The hash trie is used in a persistent manner - each modifications return a new root-node
    with the path to the changed node re-allocated. The stored elements themselves are not
    copied, only the references to them.

    Each non-value type (i.e. that encompass more memory than just sizeof(T), std::string etc,
    stored in the map must have it's own implementation of the hash32 function.

*/


template<class K, class V>
class PersistentMapPool
{
public:

    /** Key-value pair. */
    struct KeyValue{K first; V second;};

    /* Data structure types. */
    
    typedef typename PersistentListPool<KeyValue*>       KeyValueListPool;
    typedef typename PersistentListPool<KeyValue*>::List KeyValueList;

    struct RefCell;

    struct Node
    {
        struct Ref
        {
            Node* node;
            Node& operator*(){return *node;}
            Node& operator->(){return *node;}
        };

        enum NodeType{EmptyNode, ValueNode, CollisionNode};

        uint32_t used;
        Ref*     child_array; //> Pointer to an allocated child sequence
       
        /** For ValueNodes the keyvalue field is referenced. For
         *  CollisionNodes the collision_list field is referenced. */ 
        union
        {
            KeyValue*     keyvalue;
            KeyValueList* collision_list;
        }value;

        NodeType type; 

        Node():used(0), child_array(0)
        ~Node()
        {
            if(type == CollisionNode && value.collision_list) delete value.collision_list;
        }

        /** Return number of elements stored */
        size_t size(){ return count_bits(used);}

        /** Return index matching the bitfield.*/
        uint32_t index(uint32_t bitfield){return count_bits(used & (bitfield - 1));}

        /** Return reference matching the bit pattern or null if array is not large enough.*/
        Ref* get(uint32_t key, uint32_t level)
        {
            Ref* result = 0;
            uint32_t local_index = (key >> (level*5)) & 0x1f;
            if(local_index < size()) result = child_array + index(local_index);
            return result;
        }

        Ref* begin(){return child_array;}
        Ref* end(){return child_array + size();}
    };

    bool node_has_children(Node* n) {return n->children != 0;}
    bool node_has_keyvalues(Node* n) {return n->type != Node::EmptyNode;}

    /**
     * Iterator for the values in one node.
     */
    struct NodeValueIterator
    {
        Node* node;
        typename KeyValueList::iterator iter;
        typename KeyValueList::iterator end;
        
        bool      iteration_ongoing;

        KeyValue* keyvalue;

        NodeValueIterator():node(0), keyvalue(0), iteration_ongoing(false){}

        NodeValueIterator(Node* node_in):node(node_in), keyvalue(0), iteration_ongoing(false)
        {
            if(node->type == Node::CollisionNode) end = node->value.collision_list->end();
        }

        /** Returns true as long as the iterator is ongoing. Once the iteration is complete
         *  the internal reference to keyvalue is in undefined state. */
        bool move_next()
        {
            bool result = false;
            switch(node->type)
            {
                case Node::ValueNode:
                {
                    if(!keyvalue)
                    {
                        // Can visit the only value only once.
                        keyvalue = node->value.keyvalue;
                        result = true;
                    }
                    break;
                }
                case Node::CollisionNode:
                {
                    if(iteration_ongoing)
                    {
                        iter++;
                        if(iter != end)
                        {
                            keyvalue = *iter;
                            result = true;
                        }
                    }
                    else
                    {
                        iteration_ongoing = true;
                        // The list has at least two elements. No need to check for nil head.
                        iter = node->value.collision_list->begin();
                        keyvalue = *iter;
                        result = true;
                    }
                    break;
                }
                case Node::EmptyNode:
                default:
                    break;
            }
            return result;
        }

        KeyValue* current(){return keyvalue;}
    };

    // Itearator for children inside node.
    struct NodeChildIterator
    {
        Node* node;
        Node::Ref* iter;  
        Node::Ref* end;  
        bool iterating;

        NodeChildIterator(Node* node_in):node(node_in), iterating(false)
        {
            end = node->end();
        }

        bool move_next()
        {
            bool result = false;

            if(iterating) 
            {
                iter++;
            }
            else
            {
                iter = node->begin();
            }

            if(iter != end) result = true;
        }
            
        Node* current(){return iter->node;}
    };

#if 0
    /** Iterator for cells inside node*/
    struct NodeValueIterator
    {
        enum IterMode{IterNone, IterCell, IterList};
        
        IterMode iter_mode;
        RefType element_type;

        Node* parent_node;
        RefCell* current;
        RefCell* end;

        typename RefListPool::List::iterator list_iterator;
        typename RefListPool::List::iterator list_iterator_end;

        NodeValueIterator(){}

        NodeValueIterator(Node* nodeIn):parent_node(nodeIn),iter_mode(IterNone)
        {
            current = 0;
            end = parent_node->ref_array + parent_node->size();
        }

        bool is_keyvalue(){return element_type == RefValue;}

        KeyValue* get_list_keyvalue()
        {
            RefCell* c = (RefCell*) list_iterator.data_ptr();
            KeyValue* k = c->data.keyvalue;
            return k;
        }
        
        Node* get_list_nodevalue()
        {
            RefCell* c = (RefCell*) list_iterator.data_ptr();
            Node* n = (Node*) c->data.node;
            return n;
        }

        Node* node()
        {
            Node* n = 0;

            switch(iter_mode)
            {
                case IterCell:
                    n =current->data.node;
                    break;
                case IterList:
                    n = get_list_nodevalue();
                    break;
                case IterNone:
                default:
                    break;
            }
            return n;
        }

        KeyValue* keyvalue()
        {
            KeyValue* kv = 0;

            switch(iter_mode)
            {
                case IterCell:
                    kv =current->data.keyvalue;
                    break;
                case IterList:
                    kv = get_list_keyvalue();
                    break;
                case IterNone:
                default:
                    break;
            }
            return kv;
        }

        bool move_next()
        {
            if(!current)
            {
                current = parent_node->ref_array;
                
                switch(current->type_)
                {
                    case RefNode:
                        element_type = RefNode;
                        iter_mode = IterCell;
                        break;
                    case RefValue:
                        element_type = RefValue;
                        iter_mode = IterCell;
                        break;
                    case RefList:
                        iter_mode = IterList;
                        list_iterator = current->data.list->begin();
                        list_iterator_end = current->data.list->end();
                        break;
                }
                return true;
            }

            if(iter_mode == IterList)
            {
                ++list_iterator;
                if(list_iterator != list_iterator_end) return true;
                else iter_mode = IterNone;
            }

            if(++current != end)
            {
                if(current->type_ != RefList)
                {
                    iter_mode = IterCell;
                }
                else
                {
                    iter_mode = IterList; 
                    list_iterator = current->data.list->begin();
                    list_iterator_end = current->data.list->end();
                }

                return true;
            }

            return false;
        }
    };
#endif

    typedef Chunk<KeyValue>  keyvalue_chunk;
    typedef Chunk<Node>      node_chunk;
    typedef Chunk<Node::Ref> ref_chunk;

    typedef ChunkBox<KeyValue> keyvalue_chunk_box;
    typedef ChunkBox<Node>     node_chunk_box;
    typedef ChunkBox<RefCell>  ref_chunk_box;

    typedef std::unordered_map<Node*, int> refcount_map;

    // Unordered iterator to map nodes
    //
    // The purpose is to support stl -like iteration.
    class node_iterator
    {
        // Use stack of node child iterators to keep track of state.
        // Algorithm:
        //  i)  Go to the leftmost node foundable
        //  ii) iterate it's values
        //  ii) go to node sibling, 
        
        FixedStack<NodeChildIteator, 8> iter_stack; //Max tree depth is 7 plus root.
        NodeValueIterator value_iter;

        KeyValue* current;

#define CURRENT_NODE iter_stack.top()->current()
#define CURRENT_KV top_value_iterator.current()
#define TOP_ITER iter_stack.top()

        // For each node:
        // Iterate over each value in array. If value is a node, go into node.
        public:

        node_iterator(Node* node):current(0)
        {
            if(node && node_has_children(node))
            {
                push_node(node);
            }
        }

        // Latch onto the children of the node - make sure the node has children prior to pushing it
        void push_node(Node* parent_node)
        {
            iter_stack.push(NodeChildIterator(parent_node));
            TOP_ITER->move_next(); // Must succeed as node has children - now have a valid node

            Node* node = CURRENT_NODE;
            if(node_has_keyvalues(node))
            {
                value_iter = NodeValueIterator(node);
                advance(); // Move value_iter to valid value
            }
            else
            {
                // Push until a node with values is found
                push_node(node)
            }
        }

        void advance()
        {
            if(value_iter.move_next())
            {
                current = CURRENT_KV;
            }
            else
            {
                Node* node = CURRENT_NODE;
                // Ran out of values - find next node.
                if(node_has_children(node))
                {
                    push_node(node);
                }
                else
                {
                    // Just a leaf node - pop stack
                    pop();
                }
            }
        }


        void pop()
        {
            iter_stack.pop();

            if(iter_stack.depth() > 0)
            {
                if(TOP_ITER.move_next())
                {
                    if(node_hase_keyvalues(CURRENT_NODE))
                    {
                        value_iter = NodeValueIterator(CURRENT_NODE);
                        advance();
                    }
                    else
                    {
                        push_node(CURRENT_NODE);
                    }
                }
                else
                {
                    pop();
                }
            }
            else
            {
                // Stack emtpy, nothing left to do
                current = 0;
            }
        }

        void operator++(){advance();}

        bool operator==(const node_iterator& i){return current == i.current;}
        bool operator!=(const node_iterator& i){return current != i.current;}

        KeyValue& operator*(){return current;}
        KeyValue* operator->(){return current;}

#undef CURRENT_NODE
#undef CURRENT_KV
#undef TOP
    };


    /** The map class.*/
    class Map
    {
        public:
            typedef node_iterator iterator;

            Map(PersistentMapPool& pool, Node* root):pool_(pool), root_(root)
        {
            if(root_) pool_.add_ref(root_);
        }

            Map(const Map& map):pool_(map.pool_), root_(map.root_)
        {
            if(root_) pool_.add_ref(root_);
        }

            Map(Map&& map):pool_(map.pool_), root_(map.root_)
        {
            map.root_ = 0;
        }

            ~Map()
            {
                if(root_) pool_.remove_ref(root_);
            }

            Map& operator=(const Map& map)
            {
                if(this != &map)
                {
                    if(root_) pool_.remove_ref(root_);
                    pool_ = map.pool_;
                    root_ = map.root_;
                    pool_.add_ref(root_);
                }
                return *this;
            }

            Map& operator=(Map&& map)
            {
                if(this != &map)
                {
                    if(root_) pool_.remove_ref(root_);
                    pool_ = map.pool_;
                    root_ = map.root_;
                    map.root_ = 0;
                }
                return *this;
            }

            iterator begin(){return iterator(root_);}
            iterator end(){return iterator(0);}

        private:
            PersistentMapPool& pool_;
            Node* root_;
    };

    /** Remove reference to node */
    void remove_ref(Node* n)
    {
        int* ref_count = try_get_value(ref_count_, n);
        if(ref_count && (*ref_count > 0)) --(*ref_count);
    }

    /** Add reference to node*/
    void add_ref(Node* n)
    {
        if(has_key(ref_count_, n)) ref_count_[n]++;
        else ref_count_[n] = 1;
    }

    /** Create new empty map */
    Map new_map()
    {
        Map m(*this, 0);
        return m;
    }

    /** Create new map with one element*/
    Map new_map(const KeyValue& v)
    {
        Node* n = new_node(v);
        uint32_t hash_value = hash32(v);
        Map m(*this);
        return m;
    }

    /** Create a new map by adding an element to existing map*/
    Map add(const Map& old, const KeyValue& v)
    {
        // TODO
        old;
    }

    /** Garbage collection for map.*/
    void gc()
    {
        // Must cleanup keyvalues, nodes, refs and gc collided_list_pool_
        // Mark all empty

        // Visit all heads (iterate through map)
        // For each node: mark node, mark refs, go through all keyvalues and mark them
        // Then, collect all
        // Finally gc collided_list_pool_
    }

    /** Allocate new simple node */
    Node* new_node(const KeyValue& v)
    {
        Node*     n     = node_chunks_.reserve_element();
        RefCell*  cells = ref_chunks_.reserve_elements();
        KeyValue* kv    = keyvalue_chunks_.reserve_element();

        *kv = v;

        cells->bind_keyvalue(kv);
        n->ref_array = cells;
        return n;
    }


private:
    keyvalue_chunk_box                 keyvalue_chunks_;
    node_chunk_box                     node_chunks_;
    ref_chunk_box                      ref_chunks_;
    PersistentListPool<KeyValue>       collided_list_pool_;
    refcount_map                       ref_count_; // Store references to root nodes
};

#endif

#if 0
/** Generic collection printer */
template<class T>
std::ostream& operator<<(std::ostream& os, PersistentListPool<T>::List& list)
{
    each_elem_to_os(os, list.begin(), list.end());
    return os;
}
#endif

} //namespace glh

