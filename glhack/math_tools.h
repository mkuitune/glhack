/** \file math_tools.h Usefull wrappers and tools for numbers.
    \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/
#pragma once

#include <Eigen/Dense>

#include "tinymt32.h"
//#include "tinymt64.h"
#define _USE_MATH_DEFINES
#include <cmath>
#include <cstdint>
#include <list>
#include<array>
#include<map>

#define PIf 3.141592846f


/////////////// Hash functions //////////////

uint32_t hash32(const char* data, int len);

uint32_t hash32(const std::string&  string);

template<class T>
uint32_t hash32(const T& hashable)
{
    int len = sizeof(T);
    return hash32((char*)&hashable, len);
}


////////////// Numeric conversions /////////////

template<class R, class P>
R to_number(P in){return (R) in;}


namespace glh{

/////////////// Types //////////////

typedef Eigen::Vector4f vec4;
typedef Eigen::Vector3f vec3;
typedef Eigen::Vector2f vec2;
typedef Eigen::Vector2i vec2i;

typedef Eigen::Matrix4f mat4;

template<class T>
class Math
{
public:
    typedef std::array<T,2> span_t;

    static span_t null_span(){return span_t(T(0), T(0));}
};


/////////////// Bit operations //////////////


/** Count bits in field */
inline const uint32_t count_bits(uint32_t field)
{
    uint32_t count = 0;
    for(;field;count++) field &= field - 1;
    return count;
}

/** Return index of lowest unset bit*/
inline const uint32_t lowest_unset_bit(uint32_t field)
{
    if(field == 0xffffffff) return 32; //> not found
    uint32_t index = 0;
    for(;(field&0x1); field >>= 1 , index++){}

    return index;
}

/** Set nth bit in field. Return result. */
inline uint32_t set_bit_on(const uint32_t field, uint32_t index)
{
    return index < 32 ? (field | 1 << index) : field;
}

/** Unset nth bit in field. Return result. */
inline uint32_t set_bit_off(uint32_t field, uint32_t index)
{
    return index < 32 ? (field & ~(1 << index)) : field;
}

/** Return true if bit is on */
inline bool bit_is_on(const uint32_t field, const uint32_t bit)
{
    return  (field & 1 << bit) != 0;
}


//////////////////// Geometric entities ///////////////////////////


/** All points in box are min + s * (max - min) where 0 <= s <= 1*/
template<class T, int N>
struct Box
{
    typedef Eigen::Matrix<T, N, 1> corner;
    corner min;
    corner max;

    Box(const corner& min_, const corner& max_):min(min_), max(max_){}
    Box(){}
};

typedef Box<int, 2> Box2i;

/** If s is in range (a,b) return true. */
template<class T>
inline bool in_range(const T& a, const T& b, const T& s)
{
    return (s >= a) && (s <= b);
}

/* check intersections of spans (a[0],a[1]) and (b[0],b[1])
 * */

template<class T>
typename Math<T>::span_t intersect_spans(const typename Math<T>::span_t& a, const typename Math<T>::span_t& b)
{
    typedef typename Math<T>::span_t span_t;
    span_t span(0,0); // This is the null-span

    if(b[0] < a[0])
    {
        if(a[1] < b[1]) span = span_t(a[0], a[1]);
        else
        {
            if(a[0] < b[1]) span = span_t(a[0],b[1]);
        }
    }
    else
    {
        if(b[1] < a[1]) span = span_t(b[0], b[1]);
        else
        {
            if(b[0] < a[1]) span = span_t(b[0], a[1]);
        }
    }

    return span;
}

template<class T>
bool span_is_empty(const typename Math<T>::span_t& span){return span[0] >= span[1];}

/**
 * @return (box, boxHasVolume) - in case of n = 2 this means the area of course
 */
template<class T, int N>
std::tuple<Box<T,N>, bool> intersect(const Box<T,N>& a, const Box<T,N>& b)
{
    typedef Math<T>::span_t span_t;

    bool has_volume = true;
    Box<T,N> box;

    for(int i = 0; i < N; ++i)
    {
        span_t spanA(a.min[i], a.max[i]);
        span_t spanB(b.min[i], b.max[i]);

        span_t span = intersect_spans(spanA, spanB);

        if(span_is_empty(span)) has_volume = false;

        box.min[i] = span[0];
        box.max[i] = span[1];

    }

    return std::make_tuple(box, has_volume);
}


//////////////////// Generators ///////////////////////////

/** Generator for a start-inclusive an end-exclusive range := (start, end].*/
template<class T> class Range {
public:

    struct iterator {
        T current_;
        T increment_;
        bool increasing_;

        // A bit abusive for boolean operations but semantically conforms
        // to common iterator usage where != signals wether to continue iteration
        // or not.
        bool operator!=(const iterator& i){
            return increasing_ ? current_ <  i.current_ : current_ >  i.current_ ;
        }

        const T& operator*(){return current_;}

        void operator++(){current_ += increment_;}

        iterator(T val, T increment):current_(val), increment_(increment){
            increasing_ = increment_ > to_number<T, int>(0); 
        }
    };

    T range_start_;
    T range_end_;
    T increment_;

    Range(T range_start, T range_end):
        range_start_(range_start), range_end_(range_end), increment_(to_number<T, int>(1)){}

    Range(T range_start, T increment, T range_end):
        range_start_(range_start), range_end_(range_end), increment_(increment)
    {
        if((range_end < range_start) && increment_ > to_number<T,int>(0))
                increment_ *= to_number<T,int>(-1); 
    }

    iterator begin() const {return iterator(range_start_, increment_);}
    iterator end() const {return iterator(range_end_, increment_);}
};

template<class T>
Range<T> make_range(T begin, T end){return Range<T>(begin, end);}


///////////// Generators: Random number ///////////

#define GLH_RAND_SEED 7894321

/** PRN generator. */
template<class T>
struct Random{
    Random();
    Random(T seed);
    T rand();
};

template<>
struct Random<int32_t>
{
    tinymt32_t state;
    void init(int seed)
    {
        tinymt32_init(&state, seed);
    }

    Random(int seed){init(seed);}
    Random(){init(GLH_RAND_SEED);}

    int32_t rand()
    {
        uint32_t result = tinymt32_generate_uint32(&state);
        return *((int32_t*) &result);
    }
};

template<>
struct Random<float>
{
    tinymt32_t state;
    void init(int seed)
    {
        tinymt32_init(&state, seed);
    }

    Random(int seed){init(seed);}
    Random(){init(GLH_RAND_SEED);}

    float rand(){return tinymt32_generate_float(&state);}
};

/** Random range generates numbers in inclusive range(start, end)*/
template<class T>
struct RandomRange
{
    RandomRange(T start, T end);
    T rand();
};

template<>
struct RandomRange<int32_t>
{
    Random<float> random;
    int32_t start; //> Inclusive range start.
    int32_t end; //> Inclusive range end.
    float offset;

    void init()
    {
        offset = (float) (end - start);
    }

    RandomRange(int32_t start_, int32_t end_):start(start_), end(end_){init();}
    RandomRange(int32_t start_, int32_t end_, int seed):random(seed),start(start_), end(end_){init();}

    int32_t rand()
    {
        const float f = random.rand();
        return start + (int32_t) (floor(offset * f + 0.5f));
    }
};

template<>
struct RandomRange<float>
{
    Random<float> random;
    float start; //> Inclusive range start.
    float end; //> Inclusive range end.
    float offset;

    void init(){offset = end - start;}

    RandomRange(float start_, float end_):start(start_), end(end_){init();}
    RandomRange(float start_, float end_, int seed):random(seed),start(start_), end(end_){init();}

    float rand()
    {
        return start + offset * tinymt32_generate_float(&random.state);
    }
};

////////////////// Mappings /////////////////////

/** Given value within [begin, end], map it's position to range [0,1] */
template<class T> inline T interval_range(const T value, const T begin, const T end){return (value - begin)/(end - begin;)}

/** Smoothstep polynomial between [0,1] range. */
inline float smoothstep(const float x){return (1.0f - 2.0f*(-1.0f + x))* x * x;}
inline double smoothstep(const double x){return (1.0 - 2.0*(-1.0 + x))* x * x;}

/** Linear interpolation between [a,b] range (range of x goes from 0 to 1). */
template<class I, class G> inline G lerp(const I x, const G& a, const G& b){return a + x * (b - a);}

// Interpolators
template<class I, class G> class Lerp{public: 
    static G interpolate(const I x, const G& a, const G& b){
        return lerp(x,a,b);
    }
};

// Interpolating map, useful for e.g. color gradients
template<class Key, class Value, class Interp = Lerp>
class InterpolatingMap{
public:
    typedef std::map<Key,Value> map_t;

    map_t map_;

    void insert(const Value& value, const Key& key){
        map_[value] = key;
    }

    Value interpolate(const Key& key) const {
        // Three possibilities: key in [min_key, max_key], key < min_key, key > max_key

        map_t::const_iterator lower = map_.lower_bound(key); // first element before key
        map_t::const_iterator upper = map_.upper_bound(key); // first element after key
        map_t::const_iterator end   = map_.end();

        if(lower == end){
            // No elements before key 
            return map_.begin()->second;
        }
        else if(upper == end){
           // No elements after key
            return map_.rbegin()->second;
        } else
        {
            const Key low(lower->second);
            const Key high(upper->second);
            Key interp = interval_range(low, high);
            return Interp::interpolate(interp, low, high);
        }
    }
};


////////////////// Combinatorial stuff /////////////////////

/** From list of elements {a} return all the pairs generated from the sequence {a_i * a_j} */
template<class V>
std::list<std::pair<typename V::value_type, typename V::value_type>> all_pairs(V& seq)
{
    typedef std::pair<typename V::value_type, typename V::value_type> pair_elem;
    std::list<pair_elem> result;
    for(auto i = seq.begin(); i != seq.end(); ++i)
    {
        for(auto j = seq.begin(); j != seq.end(); ++j)
        {
            result.push_back(pair_elem(*i,*j));
        }
    }

    return result;
}


////////////// Linear algebra ////////////////



} // namespace glh

