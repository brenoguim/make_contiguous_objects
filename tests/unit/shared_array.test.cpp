#include <catch.hpp>
#include <mco.hpp>

#include <iostream>
#include <cstring>
#include <vector>
#include <string>

struct Ctrl
{
    Ctrl() : isAdjacent(false), refCount(0) {}
    bool isAdjacent : 1;
    unsigned refCount : 31;
};

template<class T>
struct SharedArray
{
    using Layout = std::tuple<xtd::Rng<Ctrl>, xtd::Rng<unsigned>, xtd::Rng<T>>;
    SharedArray(unsigned sz)
    {
        bool trackSize = !std::is_trivially_destructible_v<T>;
        auto t = xtd::make_contiguous_objects<Ctrl, unsigned, T>(
            1,
            xtd::arg(xtd::ctor, trackSize ? 1 : 0, sz),
            sz);

        m_ctrl = std::get<0>(t).begin();
        m_array = std::get<2>(t).begin();
        m_ctrl->refCount++;
    }

    ~SharedArray()
    {
        if (! --m_ctrl->refCount)
        {
            xtd::destroy_contiguous_objects(getLayout());
        }
    }

    auto getArraySizeAddr() const
    {
        assert(!std::is_trivially_destructible_v<T>);
        return xtd::get_adjacent_address<unsigned>(m_ctrl+1);
    }

    auto getLayout()
    {
        if constexpr (std::is_trivially_destructible_v<T>)
        {
            Layout t {{m_ctrl, m_ctrl+1}, {nullptr, nullptr}, {nullptr, nullptr}};
            return t;
        }
        else
        {
            auto szPtr = getArraySizeAddr();
            Layout t {{m_ctrl, m_ctrl+1}, {szPtr, szPtr+1}, {m_array, m_array + *szPtr}};
            return t;
        }
    }

    T* m_array;
    Ctrl* m_ctrl;
};

TEST_CASE( "SharedArray", "[constructor]" )
{
    SharedArray<int> sai {10};
    SharedArray<std::string> sas {10};
}
