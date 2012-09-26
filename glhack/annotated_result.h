#include<string>

namespace glh
{

/** A container that can either return a valid value or an error message. */
template<class T>
class AnnotatedResult
{
public:
    explicit AnnotatedResult(const T& value):valid_(true)
    {
        new(as_value())T(value);
    }

    explicit AnnotatedResult(const std::string& message):valid_(false),message_(message){}

    AnnotatedResult(const AnnotatedResult& r)
    {
        valid_ = r.valid_;
        if(r.valid_)
        {
            new(as_value())T(*(r.as_value()));
        }
        else
        {
            message = r.message_;
        }
    }

    AnnotatedResult(AnnotatedResult&& moved)
    {
        valid_ = moved.valid_;

        if(moved.valid_)
        {
            memcpy(value_, moved.value_, sizeof(T));
            moved.valid_ = false;
        }
        else message_ = std::move(moved.message_);
    }

    ~AnnotatedResult()
    {
        if(message_.empty())
        {
            as_value()->~T();
        }
    }

    T* as_value(){return ((T*) value_);}
    const std::string& message(){return message_;}
    bool valid(){return valid_;}

private:
    AnnotatedResult& operator=(const AnnotatedResult& a);

    bool valid_;
    char value_[sizeof(T)];
    std::string message_;
};

}