/** \file managed_structures.h Garbage collectable and persistent data structures.
    \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/
#pragma once

/** Interface for garbage collectable data structures.  */
class GarbageCollecting
{
public:
    virtual ~GarbageCollecting(){}
    /** Return total consumed memory.*/
    virtual size_t allocated_size() = 0;
    /** Run garbage collection.*/
    virtual void gc() = 0;
};

class Collector
{
public:
};



/** Interface for a datastructure that can be visited 
    to get addresses and sizes of the data it contains.
    Can be used for pointer traversal in garbage collection
    and serialization.
*/
class Collectable
{
public:
    virtual ~Collectable(){}
    /**
        collec
    */
    virtual void collect_fields(Collector& c) = 0;
};
