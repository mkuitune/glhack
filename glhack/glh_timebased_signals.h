/** \file glh_timebased_signals.h
    \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)

    */
#pragma once

#include "glh_typedefs.h"

#include<tuple>
#include<functional>


namespace glh{

class Animation{
public:

    struct Easing{
        enum t{Linear, Smoothstep};
    };

    static float easing(Easing::t e, float v){
             if(e == Easing::Linear){return v;}
        else if(e == Easing::Smoothstep){return smoothstep(v);}
    }

    struct VarType{
        enum t{Scalar, Verson};}; // Either regular scalar or rotation encoded in verson (unit quaternion).

    struct ChannelName{
        enum t{Arbitrary};};

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

