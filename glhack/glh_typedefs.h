/**\file glh_typedefs.h Common types.
    \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/
#pragma once

#include<string>
#include<cstdint>
#include<utility>
#include<limits>
#include<stack>

#include "math_tools.h"

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

class ChannelType
{
public:
    /** Semantic */
    enum s{ Position = 0,
            Normal   = 1, 
            Color    = 2,
            Texture  = 3,
            S_LAST
    };
   
    /** Type*/
    enum t{Value,Index, T_LAST};
};

///////////// Selection ID:s /////////////

/** Utilities for color/id based selection buffer. */
class ObjectRoster {
public:
    static const int max_id = 16777215;

    static vec4 color_of_id(int id);
    static int id_of_color(const vec4& color);
    static int id_of_color(uint8_t r, uint8_t g, uint8_t b);

    class IdGenerator{
    public:

        static const int null_id = 0;

        IdGenerator();
        int  new_id();
        void release(int id);
    private:
        int             m_next;
        std::stack<int> m_unused;
    };
};

/* TODO: Figure out if this level of genericity below is required for 
graph based state handling or can we manage with simpler structures.
*/
class EntityType{
public:
    enum t
    {
        Unknown       = 0,
        BufferHandle  = 1,
        VertexChunk   = 2,
        ShaderVar     = 3,
        ShaderProgram = 4,
        ProgramHandle = 5,
        BufferSet     = 6,
        DefaultMesh   = 7,
        Texture       = 8,

        Vec4          = 1008,
        Mat4          = 1009,
        Image8        = 1010,
        Image32       = 1011,

        // Scene assets
        Camera         = 2001,
        RenderPass     = 2002,
        TextField      = 2003,
        SceneTreeNode  = 2004,
        GlyphPane      = 2005,
        LayoutGroup    = 2006,


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

} // namespace glh
