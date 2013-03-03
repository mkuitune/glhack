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

typedef Eigen::Vector4f vec4;
typedef Eigen::Vector3f vec3;
typedef Eigen::Vector2f vec2;
typedef Eigen::Vector2i vec2i;

typedef Eigen::Matrix4f mat4;


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

