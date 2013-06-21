/** \file glh_timebased_signals.h
    \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/
#pragma once

#include "glh_typedefs.h"

#include<functional>


namespace glh{

// Animation: source, target.

class Animation{
public:
    struct VarType{
        enum{Position, Orientation};};

    struct ChannelName{
        enum{Arbitrary};};

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

    void add(obj_id id, const Event& event){events_[id] = event;}

    void remove(obj_id id){events_.erase(id);}


};

} // namespace glh

