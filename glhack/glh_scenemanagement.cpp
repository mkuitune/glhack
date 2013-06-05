/**\file glh_typedefs.cpp
    \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/

#include "glh_typedefs.h"

#include "glsystem.h"

namespace glh {

vec4 ColorSelection::color_of_id(int id){
    if(id > max_id) throw GraphicsException("ColorSelection::Id above max");
    uint8_t r = (id >> 16)&0xff;
    uint8_t g = (id >> 8)&0xff;
    uint8_t b = (id >> 0)&0xff;

    float torange = 1.f / 255.f;

    float fr = torange * Math<float>::to_type<uint8_t>(r);
    float fb = torange * Math<float>::to_type<uint8_t>(b);
    float fg = torange * Math<float>::to_type<uint8_t>(g);

    return vec4(fr,fg,fb, 1.f);
}

int ColorSelection::id_of_color(const vec4& color){
    uint8_t r = Math<uint8_t>::to_type<float>(color[0] * 255.f);
    uint8_t g = Math<uint8_t>::to_type<float>(color[1] * 255.f);
    uint8_t b = Math<uint8_t>::to_type<float>(color[2] * 255.f);
    
    return id_of_color(r,g,b);
}

int ColorSelection::id_of_color(uint8_t r, uint8_t g, uint8_t b){
    int id = (r << 16) + (g << 8) + b;
    if(id > max_id) throw GraphicsException("ColorSelection::Id above max");
    return id;
}

ColorSelection::IdGenerator::IdGenerator(){
    m_next = null_id + 1;
}

int ColorSelection::IdGenerator::new_id(){
    int id;

    if(m_unused.empty()){
        id = m_next++;
        if(id > max_id) throw GraphicsException("ColorSelection::IdGenerator new id above max");
    } else{
        id = m_unused.top();
        m_unused.pop();
    }
    return id;
}

void ColorSelection::IdGenerator::release(int id){
    m_unused.push(id);
}



}
