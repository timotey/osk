#ifndef MDVEC_HPP__
#define MDVEC_HPP__
#include "mdspan.hpp"
#include <numeric>

template<class T, size_t rank>
struct mdvec:mdspan<T, rank> {
    mdvec():mdspan<T,rank>(){}
    mdvec(auto... idx):
        mdspan<T,rank>{
            .strides={},
            .sizes={idx...},
            .offsets={},
            .data=0,
        }{
        std::exclusive_scan(
            this->sizes.begin(),
            this->sizes.end(),
            this->strides.begin(),
            1,
            std::multiplies{}
        );
        this->data = new T[this->strides.back()*this->sizes.back()];
    }
    mdvec(mdvec const& other):mdspan<T, rank>(other){
        this->data = new T[this->strides.back()*this->sizes.back()];
        static_cast<mdspan<T, rank>&>(*this) = other;
    }
    mdvec(mdvec&& other):mdspan<T, rank>(mdspan<T,rank>{
            .strides = other.strides,
            .sizes   = other.sizes,
            .offsets = other.offsets,
            .data=std::exchange(other.data, nullptr)
            }){
        this->data = new T[this->strides.back()*this->sizes.back()];
        static_cast<mdspan<T, rank>&>(*this) = other;
    }
};
#endif //MDVEC_HPP__
