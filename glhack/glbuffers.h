/** \file glbuffers.h Buffer management
 \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/
#pragma once

#include "glsystem.h"
#include "glh_typedefs.h"
#include "math_tools.h"
#include "shims_and_types.h"

#include <map>
#include <vector>
#include <cassert>
#include <memory>

namespace glh{


struct BufferSignature {
    int32_t   components_; //> Number of components.I.e 2 for vec2, 3 for vec3 etc.
    TypeId::t type_;       //> Type of data held.
    ptrdiff_t size_bytes_; //> Size of data in bytes.

    BufferSignature():components_(0), size_bytes_(0){}
    BufferSignature(const TypeId::t type, const int32_t components):type_(type), components_(components), size_bytes_(0){}

    BufferSignature& operator=(const BufferSignature& sig){
        components_ = sig.components_;
        type_ = sig.type_;
        size_bytes_ = sig.size_bytes_;
        return *this;
    }

    /** Return total count of components stored in buffer. */
    int32_t component_count() const {return size_bytes_ / (components_ * TypeId::size(type_));}
};

class VertexChunk {
public:
    VertexChunk(){}
    VertexChunk(const BufferSignature& t):sig_(t){}

    VertexChunk(const VertexChunk& vc):sig_(vc.sig_), data_(vc.data_){}

    VertexChunk(VertexChunk&& vc):sig_(vc.sig_), data_(std::move(vc.data_)){}


    VertexChunk& operator=(const VertexChunk& vc){
        data_ = vc.data_;
        sig_ = vc.sig_;
        return *this;
    }

     VertexChunk& operator=(VertexChunk&& vc){
        if(this != &vc){
            data_ = std::move(vc.data_);
            sig_ = vc.sig_;
        }
        return *this;
    }

    /** @param data:  any data to store to buffer
        @param count: number of elements of type T total in buffer
    */
    template<class T>
    void set(T* data, size_t count){
        sig_.size_bytes_ = sizeof(*data) * count;
        data_.resize(sig_.size_bytes_, 0);
        memcpy(data_.data(), data, sig_.size_bytes_);
    }

    template<class T>
    void set(const T val, size_t count){
        const size_t valsize  = sizeof(val);

        sig_.size_bytes_ = valsize * count;
        data_.resize(sig_.size_bytes_, 0);

        for(size_t offset = 0; offset < size_.bytes_; offset += valsize) memcpy(&data_[offset], &val, valsize);
    }

    uint8_t*              data() {return &data_[0];}
    template<class T> T*  data() {return reinterpret_cast<T>(&data_[0]);}

    BufferSignature signature() const {return sig_;}

    size_t size(){return sig_.size_bytes_;}

    BufferSignature      sig_;
private:
    AlignedArray<uint8_t> data_;

};

class BufferHandle {
public:

    BufferHandle(const BufferSignature& sig): mapped_sig_(sig), handle_(0), on_gpu_(false){ glGenBuffers(1, &handle_);}

    ~BufferHandle(){if(handle_) glDeleteBuffers(1, &handle_);}

    void reset()
    {
        glDeleteBuffers(1, &handle_);
        handle_ = 0;
        on_gpu_= false;
        glGenBuffers(1, &handle_);
    }

    void map_to_gpu(VertexChunk& chunk){
        assert(chunk.sig_.type_ == mapped_sig_.type_);
        on_gpu_      = true;
        mapped_sig_ = chunk.sig_;

        glBindBuffer(GL_ARRAY_BUFFER, handle_);
        uint8_t* data = chunk.data();
        glBufferData(GL_ARRAY_BUFFER, mapped_sig_.size_bytes_, data, GL_STATIC_DRAW);
    }

    void bind(GLenum target){glBindBuffer(target, handle_);}

    int32_t components()const{return mapped_sig_.components_;}
    
    /** Return number of individual elements stored in buffer. */
    int32_t component_count() const {return mapped_sig_.component_count();}

    BufferSignature mapped_sig_;

private:
    uint32_t   handle_;
    bool       on_gpu_;
};

typedef std::shared_ptr<BufferHandle>  BufferHandlePtr;
typedef std::map<std::string, BufferHandlePtr > NamedBufferHandles;



class BufferSet {
public:

    /** Resizes buffers. Resets data. */
    void assign(const std::string& name, VertexChunk& chunk){
        auto buf = try_get_value(buffers_, name);
        if(buf) {
            (*buf)->map_to_gpu(chunk);
        }
    }

    void create_handle(const char* name, const BufferSignature& sig){
        buffers_[name] = std::make_shared<BufferHandle>(sig);
    }
    void create_handle(const std::string& name, const BufferSignature& sig){
        buffers_[name] = std::make_shared<BufferHandle>(sig);
    }

    void reset(){
        for(auto& b:buffers_){
            b.second->reset();
        }

        /// must eventually call assign
    }
    NamedBufferHandles buffers_;
};



}// end namespace glh
