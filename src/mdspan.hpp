#include<array>
#include<cstddef>
#include<ranges>

template<class T, size_t dim>
struct mdspan{
    std::array<size_t, dim> strides;
    std::array<size_t, dim> sizes;
    std::array<size_t, dim> offsets;
    T* data;
    mdspan& operator=(mdspan const& other){
        copy(*this, other);
        return *this;
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
                *rbegin = std::max(*ibegin, tbegin);
            std::copy(tbegin, tend, rbegin); // copy the rest of elements from our starting array so we don't get junk
        }{
            auto rbegin = ret.sizes.begin();
            auto rend   = ret.sizes.end();
            auto tbegin = this->sizes.begin();
            auto tend   = this->sizes.end();
            auto ibegin = ends.begin();
            auto iend   = ends.end();
            for (; rbegin!=rend && ibegin != iend; ++rbegin, ++ibegin, ++tbegin)
                *rbegin = std::min(*ibegin, tbegin);
            std::copy(tbegin, tend, rbegin); // copy the rest of elements from our starting array so we don't get junk
        }
        return ret;
    }
    private:
    friend void copy(mdspan& a, mdspan const& b)requires(dim == 1){
        std::copy(
            b.data+b.offsets[0],
            b.data+b.offsets[0]+std::min(b.sizes[0], a.sizes[0]),
            a.data+a.offsets[0]
        );
    }
    friend void copy(mdspan& a, mdspan const& b){
        auto cast = []<class T1, size_t D>(std::array<T1, D>const& x)
            ->std::array<T1, D - 1>{
            std::array<T1, D - 1> ret;
            std::copy(x.begin(), x.end()-1, ret.begin());
            return ret;
        };
        mdspan<T, dim-1> suba{.strides=cast(a.strides), .sizes=cast(a.sizes), .offsets=cast(a.offsets)};
        mdspan<T, dim-1> subb{.strides=cast(b.strides), .sizes=cast(b.sizes), .offsets=cast(b.offsets)};
        for (size_t i = std::min(b.sizes.back(), a.sizes.back()); i; --i){
            suba.data = a.data+(a.offsets.back()+i-1)*a.strides.back();
            subb.data = b.data+(b.offsets.back()+i-1)*b.strides.back();
            copy(suba, subb);
        }
    }
};
