#include "masp.h"
#include "persistent_containers.h"

#include<stack>

//// Iterator ////

Masp::iterator::iterator()
{

}

bool Masp::iterator::operator++()
{

}


class Masp::Env
{
public:

    typedef glh::PMapPool<std::string, Masp::Value> value_pool;
    typedef glh::PMapPool<std::string, Masp::Value>::Map value_map;

    Masp::Env():env_pool_()
    {
        env_stack_.push(env_pool_.new_map());
    }

    value_pool            env_pool_;
    std::stack<value_map> env_stack_;
};

// Masp implementation

Masp::Masp()
{
    env_ = new Env();
}

Masp::~Masp()
{
    delete env_;
}

