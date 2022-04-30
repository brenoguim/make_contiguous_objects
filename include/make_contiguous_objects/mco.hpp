#include <memory>

namespace xtd
{

template<class T>
struct Rng
{
    auto begin() const { return m_begin; }
    auto end() const { return m_end; }
    T* m_begin;
    T* m_end;
};

constexpr std::uintptr_t findDistanceOfNextAlignedPosition(std::uintptr_t pos, std::size_t desiredAlignment)
{
    return pos ? ((pos - 1u + desiredAlignment) & -desiredAlignment) - pos : 0;
}

template<class T>
struct ArraySize
{
    ArraySize(std::size_t count) : m_count(count) {}
    std::size_t numBytes() const { return m_count*sizeof(T); }
    std::size_t m_count;
};

template<class T>
void addRequiredBytes(ArraySize<T>& init, std::size_t& pos)
{
    pos += findDistanceOfNextAlignedPosition(pos, alignof(T)) + init.numBytes();
}

template<class T>
void setRange(ArraySize<T>& init, Rng<T>& rng, std::byte*& mem)
{
    mem += findDistanceOfNextAlignedPosition(reinterpret_cast<std::uintptr_t>(mem), alignof(T));
    rng.m_begin = reinterpret_cast<T*>(mem);
    mem += init.numBytes();
    rng.m_end = reinterpret_cast<T*>(mem);
}

template<class... Args>
auto make_contiguous_layout(ArraySize<Args>... args) -> std::tuple<Rng<Args>...>
{
    std::size_t numBytes = 0;
    ((addRequiredBytes(args, numBytes), ...));

    auto mem = (std::byte*) ::operator new(numBytes);

    std::tuple<Rng<Args>...> r;

    std::apply([&] (auto&... targs) {
        (setRange(args, targs, mem),...);
    }, r);

    return r;
}

template<class T>
struct ArrayConstructor
{
    ArrayConstructor(std::size_t sz) : m_arrSize(sz) {}
    ArraySize<T> m_arrSize;
};

template<class T>
struct RangeGuard
{
    RangeGuard(Rng<T>& rng) : m_rng(rng), m_next(m_rng.begin()) {}

    void release()
    {
        m_next = nullptr;
    }

    ~RangeGuard()
    {
        if (m_next)
            while (m_next != m_rng.begin())
            {
                --m_next;
                std::destroy_at(m_next);
            }
    }

    Rng<T>& m_rng;
    T* m_next;
};

template<class Tup, class T, class... U>
void initRanges(Tup& t, ArrayConstructor<T>& rng, ArrayConstructor<U>&... rngs)
{
    constexpr int tupPos = std::tuple_size_v<Tup> - sizeof...(rngs) - 1;

    auto& rng = std::get<tupPos>(t);

    RangeGuard<T> g(rng);

    for (; g.m_next != rng.end(); ++g.m_next)
    {
        new (g.m_next) T;
    }

    if constexpr (sizeof...(rngs)>0)
        initRanges(rngs...);

    g.release();
}

template<class... Args>
auto make_contiguous_objects(const ArrayConstructor<Args>&... args) -> std::tuple<Rng<Args>...>
{
    auto layout = make_contiguous_layout(args.m_arrSize...);

    initRanges(layout, args...);

    return layout;
}

template<class... Args>
void destroy_contiguous_objects(std::tuple<Args...>& t)
{
    // TODO: invert
    std::apply([] (auto&... rngs) {
        (std::destroy(rngs.begin(), rngs.end()),...);
    }, t);

    ::operator delete((void*)std::get<0>(t).begin());
}

}
