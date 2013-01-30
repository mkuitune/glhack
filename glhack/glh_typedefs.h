/**\file glh_typedefs.h Common types.
    \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/
#pragma once

#include<string>
#include<cstdint>
#include<utility>
#include<limits>

namespace glh{

class TypeId{
public:
    enum t{Float32};

    static size_t size(const t type){
        switch(type){
            case Float32: return sizeof(float);
            default: return 0;
        }
    }
};

class EntityType{
public:
    enum t
    {
        Unknown       = 0,
        BufferHandle  = 1,
        VertexChunk   = 2,
        ShaderVar     = 3,
        ShaderProgram = 4,
        LAST_TYPE
    };

    static t to_t(uint32_t num){return num < LAST_TYPE ? t(num) : Unknown;}

};

typedef uint64_t obj_id;
typedef uint32_t obj_key;

inline obj_id make_obj_id(EntityType::t type, obj_key k){
    uint64_t utype = type;
    return (utype << 32) + k;
}

inline EntityType::t obj_id_type(obj_id oid){
    uint32_t t = oid >> 32;
    return EntityType::to_t(t);
}

inline obj_key obj_id_key(obj_id oid){
    return (obj_key) (oid & 0xffffffff);
}


}