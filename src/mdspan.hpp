#ifndef MDSPAN_HPP__
#define MDSPAN_HPP__
#include<array>
#include<cstddef>
#include<ranges>
template<class T, size_t rank>
class mdspan;
template<class T, size_t rank>
class mdspan_iterator: std::random_access_iterator_tag{
public:
    mdspan<T,rank> c;
    size_t outer_stride;
    mdspan_iterator(mdspan<T,rank> _c, size_t _outer_stride):c(_c),outer_stride(_outer_stride){}
    auto operator<=>(mdspan_iterator other){
        return c.data<=>other.c.data;
    }
    mdspan_iterator& operator++(){
        return *this+=1;
    }
    mdspan_iterator operator--(int){
        auto a = *this;
        (*this)--;
        return a;
    }
    mdspan_iterator operator++(int){
        auto a = *this;
        (*this)++;
        return a;
    }
    mdspan_iterator& operator--(){
        return *this-=1;
    }
    mdspan<T,rank>& operator[](std::ptrdiff_t idx){
        auto a=c.data;
        a+=outer_stride*idx;
        return *this;
    }
    mdspan_iterator& operator-=(std::ptrdiff_t idx){
        c.data-=outer_stride*idx;
        return *this;
    }
    mdspan_iterator& operator+=(std::ptrdiff_t idx){
        c.data+=outer_stride*idx;
        return *this;
    }
    mdspan_iterator operator-(std::ptrdiff_t idx){
        auto ret = *this;
        ret-=idx;
        return ret;
    }
    mdspan_iterator operator+(std::ptrdiff_t idx){
        auto ret = *this;
        ret+=idx;
        return ret;
    }
    mdspan<T,rank>& operator*(){
        return c;
    }
    bool operator==(mdspan_iterator const& o){
        return c.data==o.c.data;
    }
};

template<class T, size_t dim>
class mdspan{
    public:
    std::array<size_t, dim> strides;
    std::array<size_t, dim> sizes;
    std::array<size_t, dim> offsets;
    T* data;
    friend class mdspan<T,dim-1>;
    public:
    size_t size(size_t i){
        return sizes[i];
    }
    mdspan& operator=(mdspan const& other){
        size_t i = std::min(sizes.back(), other.sizes.back());
        for (size_t x = 0; x < i; ++x){
            (*this)[x] = other[x];
        }
        return *this;
    }
    mdspan_iterator<T, dim-1> begin()const{
        auto cast = []<class T1, size_t D>(std::array<T1, D>const& x)
            ->std::array<T1, D - 1>{
            std::array<T1, D - 1> ret;
            std::copy(x.begin(), x.end()-1, ret.begin());
            return ret;
        };
        return mdspan_iterator{mdspan<T,dim-1>{
            .strides=cast(strides),
            .sizes=cast(sizes),
            .offsets=cast(offsets),
            .data = data+(offsets.back())*strides.back()
        }, strides.back()};
    }
    mdspan_iterator<T, dim-1> begin(){
        auto cast = []<class T1, size_t D>(std::array<T1, D>const& x)
            ->std::array<T1, D - 1>{
            std::array<T1, D - 1> ret;
            std::copy(x.begin(), x.end()-1, ret.begin());
            return ret;
        };
        return mdspan_iterator{mdspan<T,dim-1>{
            .strides=cast(strides),
            .sizes=cast(sizes),
            .offsets=cast(offsets),
            .data = data+(offsets.back())*strides.back()
        }, strides.back()};
    }
    mdspan_iterator<T, dim-1> end()const{
        auto cast = []<class T1, size_t D>(std::array<T1, D>const& x)
            ->std::array<T1, D - 1>{
            std::array<T1, D - 1> ret;
            std::copy(x.begin(), x.end()-1, ret.begin());
            return ret;
        };
        return {mdspan<T,dim-1>{
            .strides=cast(strides),
            .sizes=cast(sizes),
            .offsets=cast(offsets),
            .data = data+(offsets.back()+sizes.back())*strides.back()
        },strides.back()};
    }
    mdspan_iterator<T, dim-1> end(){
        auto cast = []<class T1, size_t D>(std::array<T1, D>const& x)
            ->std::array<T1, D - 1>{
            std::array<T1, D - 1> ret;
            std::copy(x.begin(), x.end()-1, ret.begin());
            return ret;
        };
        return {mdspan<T,dim-1>{
            .strides=cast(strides),
            .sizes=cast(sizes),
            .offsets=cast(offsets),
            .data = data+(offsets.back()+sizes.back())*strides.back()
        },strides.back()};
    }
    mdspan<T, dim-1> operator[](size_t idx)const{
        return *(begin()+idx);
    }
    mdspan<T, dim-1> operator[](size_t idx){
        return *(begin()+idx);
    }
    T const& operator()(std::same_as<size_t> auto... idxs) const{
        auto offsetbegin = offsets.begin();
        auto stridebegin = strides.begin();
        auto idx = (((idxs+offsetbegin++)*strides++) +...+ 0);
        return data[idx];
    }
    T& operator()(std::integral auto... idxs){
        static_assert(sizeof...(idxs)==dim, "index count must be equal to the array dimension");
        auto f = [i=0, this](auto a) mutable {
            auto temp = (a+offsets[i])*strides[i];
            i++;
            return temp;
        };
        auto idx = (f(idxs) +...+ 0);
        return data[idx];
    }
    mdspan subspan(std::initializer_list<size_t> starts, std::initializer_list<size_t> ends) const{
        mdspan ret;
        ret.strides = this->strides;
        ret.data = this->data;
        {
            auto rbegin = ret.offsets.begin();
            auto rend   = ret.offsets.end();
            auto tbegin = this->offsets.begin();
            auto tend   = this->offsets.end();
            auto ibegin = starts.begin();
            auto iend   = starts.end();
            for (; rbegin!=rend && ibegin != iend; ++rbegin, ++ibegin, ++tbegin)
                *rbegin = std::max(*ibegin, *tbegin);
            std::copy(tbegin, tend, rbegin); // copy the rest of elements from our starting array so we don't get junk
        }{
            auto rbegin = ret.sizes.begin();
            auto rend   = ret.sizes.end();
            auto tbegin = this->sizes.begin();
            auto tend   = this->sizes.end();
            auto ibegin = ends.begin();
            auto iend   = ends.end();
            for (; rbegin!=rend && ibegin != iend; ++rbegin, ++ibegin, ++tbegin)
                *rbegin = std::min(*ibegin, *tbegin);
            std::copy(tbegin, tend, rbegin); // copy the rest of elements from our starting array so we don't get junk
        }
        return ret;
    }
};
template<class T>
class mdspan<T,1>{
    public:
    std::array<size_t, 1> strides;
    std::array<size_t, 1> sizes;
    std::array<size_t, 1> offsets;
    T* data;
    public:
    size_t size(size_t i){
        return sizes[i];
    }
    mdspan& operator=(mdspan const& other){
        size_t i = std::min(sizes.back(), other.sizes.back());
        for (size_t x = 0; x < i; ++x){
            (*this)[x] = other[x];
        }
        return *this;
        //std::copy(
        //    other.data+other.offsets[0],
        //    other.data+other.offsets[0]+std::min(other.sizes[0], this->sizes[0]),
        //    this->data+this->offsets[0]
        //);
        //return *this;
    }
    T const* begin() const{
        return data+offsets[0];
    }
    T* begin(){
        return data+offsets[0];
    }
    T const* end() const{
        return data+offsets[0]+sizes[0];
    }
    T& operator[](size_t idx){
        return *(begin()+idx);
    }
    T const& operator[](size_t idx)const{
        return *(begin()+idx);
    }
    T const& operator()(std::same_as<size_t> auto idxs) const{
        auto idx = idxs+offsets.front();
        return data[idx];
    }
    T& operator()(std::integral auto... idxs){
        static_assert(sizeof...(idxs)==1, "index count must be equal to the array dimension");
        auto f = [i=0, this](auto a) mutable {
            auto temp = (a+offsets[i])*strides[i];
            i++;
            return temp;
        };
        auto idx = (f(idxs) +...+ 0);
        return data[idx];
    }
    mdspan subspan(std::initializer_list<size_t> starts, std::initializer_list<size_t> ends){
        mdspan ret;
        ret.strides = this->strides;
        ret.data = this->data;
        {
            auto rbegin = ret.offsets.begin();
            auto rend   = ret.offsets.end();
            auto tbegin = this->offsets.begin();
            auto tend   = this->offsets.end();
            auto ibegin = starts.begin();
            auto iend   = starts.end();
            for (; rbegin!=rend && ibegin != iend; ++rbegin, ++ibegin, ++tbegin)
                *rbegin = std::max(*ibegin, *tbegin);
            std::copy(tbegin, tend, rbegin); // copy the rest of elements from our starting array so we don't get junk
        }{
            auto rbegin = ret.sizes.begin();
            auto rend   = ret.sizes.end();
            auto tbegin = this->sizes.begin();
            auto tend   = this->sizes.end();
            auto ibegin = ends.begin();
            auto iend   = ends.end();
            for (; rbegin!=rend && ibegin != iend; ++rbegin, ++ibegin, ++tbegin)
                *rbegin = std::min(*ibegin, *tbegin);
            std::copy(tbegin, tend, rbegin); // copy the rest of elements from our starting array so we don't get junk
        }
        return ret;
    }
};
//template<class T>
//class mdspan<T,0>{};
#endif //MDSPAN_HPP__
