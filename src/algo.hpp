#ifndef ALGO_HPP__
#define ALGO_HPP__
#include<utility>
template<class T>
struct enumerate {
    T& c;
        struct it{
            decltype(c.begin()) it;
            std::size_t ctr=0;
            auto operator*(){
                return std::make_pair(*it, ctr);
            }
            auto operator++(){
                ctr++;
                it++;
                return *this;
            }
            auto operator==(auto other){
                return it == other.it;
            }
        };
    auto begin(){
        return it{c.begin()};
    }
    auto end(){
        return it{c.end()};
    }
};
#endif //ALGO_HPP__
