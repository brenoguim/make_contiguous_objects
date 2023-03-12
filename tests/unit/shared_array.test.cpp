#include <catch.hpp>
#include <mco.hpp>

#include <iostream>
#include <cstring>
#include <vector>
#include <string>

template<class T>
struct SharedArray
{
    struct Ctrl { unsigned refCount {0}; };

    enum Layout {
        Control,
        ArraySize,
        Elements,
    };

    using LayoutT = std::tuple<
        xtd::span<Ctrl>,
        xtd::span<unsigned>,
        xtd::span<T>
    >;

    SharedArray(unsigned sz)
    {
        bool trackSize = !std::is_trivially_destructible_v<T>;
        auto t = xtd::make_contiguous_objects<Ctrl, unsigned, T>(
            1,
            xtd::arg(xtd::ctor, trackSize ? 1 : 0, sz),
            sz);

        m_ctrl = std::get<Layout::Control>(t).begin();
        m_array = std::get<Layout::Elements>(t).begin();
        m_ctrl->refCount++;
    }

    ~SharedArray()
    {
        if (! --m_ctrl->refCount)
        {
            xtd::destroy_contiguous_objects(getLayout());
        }
    }

    LayoutT getLayout()
    {
        if constexpr (std::is_trivially_destructible_v<T>)
        {
            return {{m_ctrl, m_ctrl+1}, {nullptr, nullptr}, {nullptr, nullptr}};
        }
        else
        {
            auto szPtr = xtd::get_adjacent_address<unsigned>(m_ctrl+1);
            return {{m_ctrl, m_ctrl+1}, {szPtr, szPtr+1}, {m_array, m_array + *szPtr}};
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
