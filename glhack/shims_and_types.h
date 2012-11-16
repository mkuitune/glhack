/** \file shims_and_types.h Declarations not linked directly to OpenGL.
    Dependencies:
    Eigen
*/
#pragma once

#include<set>
#include<vector>
#include<iterator>
#include<algorithm>
#include<iostream>
#include<list>
#include<map>
#include<utility>
#include<functional>

#include "allocators.h"
#include "glh_typedefs.h"
#include "math_tools.h"

/////////// Utility macros /////////////

#define DeclInterface(name_param, fields_params)\
class name_param{ public: virtual ~name_param(){};\
    fields_params}

#define static_array_size(array_param) (sizeof(array_param)/sizeof(array_param[0]))

/////////// Generic overloads /////////////

template < typename T, typename U >
std::ostream & operator<< ( std::ostream & os, std::pair < T, U > const & p )
{
  os << "( " << p.first << ", " << p.second << " )";
  return os;
}


namespace glh {

/////////// Conditional utilities /////////////

template<class T>
bool any_of(const T& val, const T& ref0, const T& ref1){return val == ref0 || val == ref1;}
template<class T>
bool any_of(const T& val, const T& ref0, const T& ref1, const T& ref2){return val == ref0 || val == ref1 || val == ref2;}
template<class T>
bool any_of(const T& val, const T& ref0, const T& ref1, const T& ref2, const T& ref3){return val == ref0 || val == ref1 || val == ref2 || val == ref3;}

template<class T>
bool none_of(const T& val, const T& ref0, const T& ref1){return val != ref0 && val != ref1;}
template<class T>
bool none_of(const T& val, const T& ref0, const T& ref1, const T& ref2){return val != ref0 && val != ref1 && val != ref2;}
template<class T>
bool none_of(const T& val, const T& ref0, const T& ref1, const T& ref2, const T& ref3){return val != ref0 && val != ref1 && val != ref2 && val != ref3;}


//////// Helpfull small types /////////

/** An immutable accessor to a valid or non-valid pointer. */
template<class T>
class ConstOption
{
public:
    ConstOption(const T* value):ptr(value){}
    bool is_valid() const {return ptr != 0;}
    const T& operator*() const {return *ptr;}
    const T& operator->() const {return *ptr;}
    const T* get(){return ptr;}
private:
    const T* ptr;
};

/////// String utilities /////////

/** Structure to hold one line of text and the number of the line to which it belongs to. */
struct TextLine
{
    std::string string;
    int         line_number;

    const char* begin() const {return string.c_str();}
    const char* end() const {return string.c_str() + string.size();}
    int length(){return string.size();}
};

/** Construct a text line instance. */
TextLine make_text_line(const char* buffer, int begin, int end, int line);
/** Split string to TextLine instances based on the delimiter. */
std::list<TextLine> string_split(const char* str, const char* delim);

/** Get a string representation from arbitrary value supporting the << -operator. */
template<class T>
cstring to_string(const T& value)
{
    std::ostringstream stream;
    stream << value;
    return stream.str();
}

///////////// Container operations ////////////////

template<class Container, class Function>
void foreach(Container& c, Function& f)
{
    auto iter = c.begin();
    auto end = c.end();
    for(;iter!=end;++iter)
    {
        f(*iter);
    }
}

template<class Container, class Function>
void foreach_value(Container& c, Function& f)
{
    auto iter = c.begin();
    auto end = c.end();
    for(;iter!=end;++iter)
    {
        f(iter->second);
    }
}

/** For a container of functions call each function with the parameter p.*/
template<class Container, class Parameter>
void calleach(Container& c, const Parameter& p)
{
    auto iter = c.begin();
    auto end = c.end();
    for(;iter!=end;++iter)
    {
        (*iter)(p);
    }
}

template<class Container, class Parameter1, class Parameter2>
void calleach(Container& c, const Parameter1& p1, const Parameter2& p2)
{
    auto iter = c.begin();
    auto end = c.end();
    for(;iter!=end;++iter)
    {
        (*iter)(p1, p2);
    }
}

template<class MapClass>
void foreach_value(MapClass& map, std::function<void(typename MapClass::mapped_type& t)> f)
{
    for(auto i = map.begin(); i != map.end(); ++i)
        f(i->second);
}

/** Unsafe version of std::copy */
template<class II, class OI>
void unsafe_copy(II begin, II end, OI out)
{
    while(begin != end) *out++ = *begin++;
}

/** Copy all elements within a container that pass the predicate test
 * to a new one*/
template<class Cont, class PredFun>
Cont copyif(Cont& c, PredFun pred)
{
    Cont result;
    auto inserter = std::inserter(result, result.begin());
    for(auto i = c.begin(); i != c.end(); ++i)
    {
        if(pred(*i))
        {
            *inserter++ = *i;
        }
    }
    return result;
}

/////// Collections /////////

/** Create a stack with a specific maximum depth. */
template<class T, int MaxDepth>
class FixedStack
{
public:
    FixedStack():level_(0){}

    T* top()
    {
        if(level_ > 0) return ((T*)stack_) + (level_ - 1);
        else return 0;
    }

    bool push(const T& data)
    {
        bool result = false;
        if(level_ < MaxDepth)
        {
            ((T*)stack_)[level_] = data;
            level_++;
            result = true;
        }
        return result;
    }

    T& at(size_t level){return ((T*) stack_)[level];}

    void alloc(const T& t, size_t level)
    {
        T* address = ((T*) stack_) + level;
        new(address)T(t);
    }

    bool pop()
    {
        bool result = false;
        if(level_ > 0)
        {
            level_--;
            result = true;
        } 
        return result;
    }

    size_t depth(){return level_;}

private:
    typename std::aligned_storage <sizeof(T), std::alignment_of<T>::value>::type stack_[MaxDepth];
    size_t level_;
};

/** Manage a static array of data aligned 16 byte boundary. */
template<class T>
class AlignedArray
{
public:
    typedef T  value_type;
    typedef T* iterator;

    // TODO: Check alignment of value type!
    // TODO: Implement move

    AlignedArray():data_(nullptr), capacity_(0), size_(0)
    {
        // Make sure T is aligned to 16 bytes
        static_assert((sizeof(T) == 1 )||
                      (sizeof(T) < 8 && sizeof(T) % 2 == 0) ||
                       (sizeof(T)%16 == 0 ),
                       "Size of stored datatype is not alignable to 16 bytes!");
        size_t hint = 12;
        realloc_data(hint);
    }

    AlignedArray(size_t count, const T& value = T()):data_(nullptr), capacity_(0), size_(0)
    {
        resize(count, value);
    }


    AlignedArray(const AlignedArray& old)
    {
        *this = old;
    }

    ~AlignedArray()
    {
        if(data_) aligned_free(data_);
    }

    AlignedArray& operator=(const AlignedArray& old)
    {
        realloc_data(old.capacity());
        unsafe_copy(old.begin(), old.end(), data_);
        size_ = old.size();
    }

    T& operator[](size_t index){ return data_[index];}

    T* data(){return data_;}

    T& append()
    {
        add_data(1);
        return &data_[size_ - 1];
    }

    void resize(size_t new_size, const T& init_value)
    {
        realloc_data(new_size);
        if(new_size > size_)
        {
            size_t delta = new_size - size_;
            size_t oldSize = size_;
            add_data(delta);
            for(size_t i = oldSize; i < new_size; ++i) data_[i] = init_value;
        }
    }

    iterator begin() { return data_;}
    iterator end() { return data_ + size_;}

    size_t size() const {return size_;}
    size_t capacity() { return capacity_;}
    size_t value_size(){ return sizeof(T);}

    void assign(T* start_ptr, T* end_ptr)
    {
        size_t count = end_ptr - start_ptr;
        size_ = 0;
        add_data(count);
        unsafe_copy(start_ptr, end_ptr, data_);
    }
    void clear() { size_  = 0;}
private:

    void add_data(size_t count)
    {
        size_t new_size = size_ + count;
        if(new_size > capacity_)
        {
            auto new_capacity = capacity_ * 2;
            if(new_size > new_capacity) new_capacity = new_size + new_size / 5;

            realloc_data(new_capacity);
        }

        size_ = new_size;
    }

    // TODO add safety checks
    void realloc_data(size_t new_capacity)
    {
        if(new_capacity <= capacity_) return;

        size_t new_size_bytes =  count_in_bytes(new_capacity);

        if(data_)
        {
            T* new_data = (T*) aligned_alloc(new_size_bytes);
            unsafe_copy(begin(), end(), new_data);
            aligned_free(data_);
            data_ = new_data;
        }
        else
        {
            data_ = (T*) aligned_alloc(new_size_bytes);
            memset(data_, 0, new_size_bytes);
        }

        capacity_ = new_capacity;
    }

    size_t count_in_bytes(size_t count){return count * value_size();}

    T* data_;
    size_t capacity_;
    size_t size_;
};

/** A linear storage of items */
template<class T>
class Array
{
public:
    Array(size_t hint):data_(hint){}
    T& operator[](size_t i){return data_[i];}
    T* data(){return &data_[0];}
private:
    std::vector<T> data_;
};


/** Store pool of data chunks. */
template<class T>
class Pool {
public:
    typedef std::vector<T>                   DataContainer;
    typedef typename DataContainer::iterator iterator;

    typedef T key_type;
    typedef T value_type;
    typedef const T&  const_reference;

    Pool():live_count_(0){}

    // Allocate new item
    T& push()
    {
        if(live_count_ >= data_.size())
        {
            size_t new_size = max(16, data_.size() * 2);
            data_.resize(new_size);
        }

        return data_[live_count_++];
    }

    T& push(const T& data)
    {
        T& last(push());
        last = data;
        return last;
    }

    void push_back(const T& data)
    {
        push(data);
    }

    // Free space to beginning of buffer. Does not initialize area.
    void clear()
    {
        live_count_ = 0;
    }

    iterator begin(){return data_.begin();}
    iterator end(){return begin() + live_count_;}
    
private:
    DataContainer data_;
    size_t live_count_;
};


/** Linked list whose elements are allocated from the data pool assigned as reference
 *  in the beginning. */
template<class T>
class PooledList {
public:

    typedef T value_type;
    typedef const T&  const_reference;

    struct Node{
        T data;
        Node* next;
    };

    struct iterator
    {
        Node* n;

        bool operator !=(const iterator& i){return n!=i.n;}
        T& operator*(){return n->data;}
        void operator++(){if(n) n = n->next;}
        iterator():n(nullptr){}
        iterator(Node* node):n(node){}
    };

    struct const_iterator
    {
        Node* n;

        bool operator !=(const const_iterator& i){return n!=i.n;}
        const T& operator*(){return n->data;}
        void operator++(){if(n) n = n->next;}
        const_iterator():n(nullptr){}
        const_iterator(Node* node):n(node){}
    };


    typedef Pool<Node> ListPool;

    PooledList(ListPool& pool_):tail_(nullptr), head_(nullptr){}

    void push_back(const T& var)
    {
        Node* node = &pool_.push();
        node->data = var;
        
        if(tail_)
        {
            tail_->next = node;
            tail_ = node;
        }
        else
        {
            head_ = node;
            tail_ = node;
        }
    }

    iterator begin(){return iterator(head_);}
    iterator end(){return iterator(nullptr);}

    const_iterator begin() const {return const_iterator(head_);}
    const_iterator end() const {return const_iterator(nullptr);}


private:
    Pool<Node> pool_;
    Node* head_;
    Node* tail_;
};

template<class Fun>
struct funcompare
{
    funcompare(){}
    bool operator()(const Fun& a, const Fun& b){return &a < &b;}
};


/** Stl-vector like container that is always ordered.*/
template<class T, class Compare = std::less<T>>
struct ArraySet
{
    typedef std::vector<T>                      Container;
    typedef typename Container::iterator        iterator;
    typedef typename Container::const_iterator  const_iterator;
    typedef typename Container::const_reference const_reference;

    typedef T key_type;
    typedef T value_type;

    std::vector<T> data_;
    Compare cmp_;

    ArraySet(const Compare& c = Compare()):data_(), cmp_(c){}

    template <class InputIterator>
    ArraySet(InputIterator first, InputIterator last, const Compare& c = Compare()):data_(first, last), cmp_(c)
    {
        std::sort(begin(), end(), cmp_);
        // Remove duplicates
        data_.erase(std::unique(begin(), end()), end());
    }

    iterator insert(const T& t)
    {
        iterator i = std::lower_bound(begin(), end(), t, cmp_);
        if(i == end() || cmp_(t, *i)) data_.insert(i, t);
        return i;
    }

    template<class ITER>
    iterator insert(ITER ii, const T& t)
    {
        iterator i = std::lower_bound(begin(), end(), t, cmp_);
        if(i == end() || cmp_(t, *i)) data_.insert(i, t);
        return i;
    }

    const_iterator find(const T& t) const
    {
        const_iterator i = std::lower_bound(begin(), end(), t, cmp_);
        return i == end() || cmp_(t, *i) ? end() : i;
    }

    void erase(const T& t)
    {
        data_.erase(std::remove(begin(), end(), t), end());
    }

    void push_back(const T& t)
    {
        insert(t);
    }

    iterator begin(){return data_.begin();}
    iterator end(){return data_.end();}

    const_iterator begin() const {return data_.begin();}
    const_iterator end() const {return data_.end();}
};

/** Store a two-way mapping between variables. */
template<class Key, class Value>
class BiMap
{
public:
    typedef Key key_type;
    typedef Value mapped_type;
    typedef std::pair<Key,Value> value_type;

    typedef std::map<Key,Value> Map;
    typedef std::map<Value,Key> InverseMap;

    typedef typename Map::iterator iterator;
    typedef typename Map::const_iterator const_iterator;

    struct ValueInserter
    {
        BiMap& parent_;
        const Key& k_;
        ValueInserter(BiMap& parent, const Key& k):parent_(parent), k_(k){}
        void operator=(const Value& v)
        {
            parent_.keys_to_values_[k_] = v;
            parent_.values_to_keys_[v] = k_; 
        }
    };

    struct KeyInserter
    {
        BiMap& parent_;
        const Value& v_;
        KeyInserter(BiMap& parent, const Value& v):parent_(parent), v_(v){}
        void operator=(const Key& k)
        {
            parent_.keys_to_values_[k] = v_;
            parent_.values_to_keys[v_] = k;
        }
    };

    const Map& map() const {return keys_to_values_;}
    const InverseMap& inverse_map() const {return values_to_keys_;}

    ValueInserter operator[](const Key& k)
    {
        return ValueInserter(*this, k);
    }

    KeyInserter operator[](const Value& v)
    {
        return KeyInserter(*this, k);
    }

    const ValueInserter operator[](const Key& k) const
    {
        return ValueInserter(*this, k);
    }

    const KeyInserter operator[](const Value& v) const
    {
        return KeyInserter(*this, k);
    }

    iterator begin(){return keys_to_values_.begin();}
    iterator end(){return keys_to_values_.end();}
    const_iterator begin() const {return keys_to_values_.begin();}
    const_iterator end() const {return keys_to_values_.end();}

private:
    Map        keys_to_values_;
    InverseMap values_to_keys_;
};

//////////////// Container insertion ////////////////

/** Inserter1. Facilitates linked ()-calls to add for one parameter.*/
template<class Container>
struct Inserter1{
    Container& parent_;
    Inserter1(Container& parent):parent_(parent){}
    Inserter1& operator()(typename const Container::value_type& v)
    {
        add(parent_, v);
        return *this;
    }
};

/** Inserter2. Facilitates linked ()-calls to add for two parameters.*/
template<class Container>
struct Inserter2{
    Container& parent_;
    Inserter2(Container& parent):parent_(parent){}
    Inserter2& operator()(typename const Container::key_type& k, typename const Container::mapped_type& v)
    {
        add(parent_, k, v);
        return *this;
    }
};


template<class T>
Inserter1<ArraySet<T>> add(ArraySet<T>& c, const T& p)
{
    c.insert(p);
    return Inserter1<ArraySet<T>>(c);
}

template<class T>
Inserter1<std::list<T>> add(std::list<T>& c, const T& p)
{
    c.push_back(p);
    return Inserter1<std::list<T>>(c);
}

template<class C, class V>
Inserter1<C> add(C& c, const V& v)
{
    c.push_back(v);
    return Inserter1<C>(c);
}

template<class Map>
Inserter2<Map> add(Map& map, typename const Map::key_type& k, typename const Map::mapped_type& v)
{
    map[k] = v;
    return Inserter2<Map>(map);
}

template<class Container, class Iter>
void add_range(Container& c, Iter start, Iter end)
{
    std::copy(start, end, std::back_inserter(c));
}

//////////// Map utilities //////////////
template<class M, class KEY>
bool has_key(M& map, KEY key)
{
    return map.count(key) > 0;
}

/** Try to find value matching key. */ 
template<class M>
typename M::mapped_type* try_get_value(M& map, typename M::key_type& key)
{
    typename M::mapped_type* result = nullptr;
    if(map.count(key) > 0)
    {
        result = &map[key];
    }
    return result;
}


//////////// Container functions //////////////


/** Create lists from input parameters*/

template<class T>
std::list<T> list(const T& p0)
{
    std::list<T> l; l.push_back(p0); return l;
}
template<class T>
std::list<T> list(const T& p0, const T& p1)
{
    std::list<T> l; l.push_back(p0); l.push_back(p1); return l;
}
template<class T>
std::list<T> list(const T& p0, const T& p1,  const T& p2)
{
    std::list<T> l; l.push_back(p0); l.push_back(p1); l.push_back(p2); return l;
}
template<class T>
std::list<T> list(const T& p0, const T& p1, const T& p2, const T& p3)
{
    std::list<T> l; l.push_back(p0); l.push_back(p1); l.push_back(p2); l.push_back(p3); return l;
}

/** Create pair from input elements. */
template<class T, class V>
std::pair<T, V> to_pair(const T& t, const V& v){return std::pair<T, V>(t,v);}

/** Append the second container to the first.*/
template<class T>
T append(const T& first_container, const T& second_container)
{
    T out(first_container);
    std::copy(second_container.begin(),second_container.end(), std::back_inserter(out)); 
    return out;
}

/** Join containers and return result. */
template<class Cont>
Cont join(const Cont& c0, const Cont& c1){return append(c0, c1);}
template<class Cont>
Cont join(const Cont& c0, const Cont& c1, const Cont& c2){return append(append(c0, c1), c2);}
template<class Cont>
Cont join(const Cont& c0, const Cont& c1, const Cont& c2, const Cont& c3){return append(append(append(c0, c1), c2),c3);}

/** Split elements evenly within a container to n containers.
 *  if n > element count then count - n empty containers are returned.*/
template<class C>
std::vector<C> split_container(const C& container, size_t n)
{
    std::vector<C> result;
    if(n > 1)
    {
        for(size_t m = 0; m < n; ++m) add(result, C());

        size_t c_size = container.size();
        size_t split_index = c_size % n;
        size_t n_pre_split = c_size / n + 1;

        // Insert ranges to each container.
        size_t vi = 0;
        typename C::const_iterator iter = container.begin();

        for(size_t i = 0; i < n; ++i)
        {
            size_t n_elems = i < split_index ? n_pre_split : n_pre_split - 1;
            typename C::const_iterator iter_last = iter;
            size_t m = 0; while(m++ < n_elems) iter_last++;
            typename C::iterator insert_start = result[i].begin();
            std::copy(iter, iter_last, std::inserter(result[i], insert_start));
            iter = iter_last;
        }
    }
    else
    {
        result.push_back(container);
    }
    return result;
}

/** Return a new container with the elements of the container for which the predicate function returns true */
template<class Collection, class Fun>
Collection filter(const Collection& collection, const Fun fun)
{
    Collection out;

    for(auto i = colllection.begin(); i != collection.end(); ++i)
    {
        if(fun(*i)) out.push_back(*i);
    }

    return out;
}


/** Fold left operation */
template<class RES, class CONT>
RES fold_left(RES res, std::function<RES(const RES& r, const typename CONT::value_type& v)> fun, const CONT& c)
{
    for(auto i = c.begin(); i != c.end(); ++i)
    {
        res = fun(res, *i);
    }

    return res;
}


/** Iterat*/

//////////// Streams //////////////

template<class Key, class Value>
std::ostream& operator<<(std::ostream& os, const std::pair<Key, Value>& p)
{
    os << "(" <<p.first << ", " << p.second << ")";
    return os;
}

template<class Iter>
std::ostream& each_elem_to_os(std::ostream& os, Iter begin, Iter end)
{
    for(;begin != end; ++begin) os << *begin << ", ";
    return os;
}

template<class Iter>
std::string range_to_string(Iter begin, Iter end)
{
    std::ostringstream os;
    each_elem_to_os(os, begin, end);
    return os.str();
}

/** Generic collection printer */
template<class T>
std::ostream& operator<<(std::ostream& os, const std::list<T>& list)
{
    each_elem_to_os(os, list.begin(), list.end());
    return os;
}

template<class T>
std::ostream& operator<<(std::ostream& os, const PooledList<T>& list)
{
    each_elem_to_os(os, list.begin(), list.end());
    return os;
}

template<class T>
std::ostream& operator<<(std::ostream& os, const ArraySet<T>& list)
{
    each_elem_to_os(os, list.begin(), list.end());
    return os;
}

template<class T, class V>
std::ostream& operator<<(std::ostream& os, const BiMap<T, V>& map)
{
    each_elem_to_os(os, map.begin(), map.end());
    return os;
}

template<class T, class V>
std::ostream& operator<<(std::ostream& os, const std::map<T, V>& map)
{
    each_elem_to_os(os, map.begin(), map.end());
    return os;
}


//template<class T>
//void add(std::set<T>& c, const T& p){c.insert(p);}

//template<class T>
//void add(Pool<T>& c, const T& p){c.push(p);}

//////////////////////// Hashing /////////////////////////////

uint32_t get_hash32(cstring& string);

template<class T>
uint32_t get_hash32(const T& elem)
{
    const char* data = (const char*) (&elem);
    int len = sizeof(T) / sizeof(char);
    return hash32(data, len);
}

}// namespace glh

