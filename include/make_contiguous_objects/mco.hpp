#include <memory>

namespace xtd
{

template<class T>
struct Rng
{
    using type = T;
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

struct uninit_t{}; static constexpr uninit_t uninit;
struct default_ctor_t{}; static constexpr default_ctor_t default_ctor;
struct aggregate_t{}; static constexpr aggregate_t aggregate;

template<class T>
struct ArgSize 
{
    using command = default_ctor_t;
    std::size_t m_count;
};

template<class T>
struct ArgSizeUninit
{
    using command = uninit_t;
    std::size_t m_count;
};

template<class T>
struct ArgSizeDefaultCtor
{
    using command = default_ctor_t;
    std::size_t m_count;
};


template<class T>
struct ArgSizeAggregate
{
    using command = aggregate_t;
    std::size_t m_count;
};



auto arg(std::size_t count) { return ArgSize<void>{count}; }
auto arg(uninit_t, std::size_t count) { return ArgSizeUninit<void>{count}; }
auto arg(default_ctor_t, std::size_t count) { return ArgSizeDefaultCtor<void>{count}; }
auto arg(aggregate_t, std::size_t count) { return ArgSizeAggregate<void>{count}; }

template<class T>
struct RangeGuard
{
    using type = T;
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
void initRanges(Tup& t, T&& ac, U&&... acs)
{
    constexpr int tupPos = std::tuple_size_v<Tup> - sizeof...(acs) - 1;

    auto& rng = std::get<tupPos>(t);

    RangeGuard g(rng);
    using DT = std::remove_cv_t<std::remove_reference_t<T>>;
    using TheT = typename decltype(g)::type;

    for (; g.m_next != rng.end(); ++g.m_next)
    {
        if constexpr (std::is_same_v<typename DT::command, uninit_t>) 
            new (g.m_next) TheT;

        if constexpr (std::is_same_v<typename DT::command, default_ctor_t>) 
            new (g.m_next) TheT();

        if constexpr (std::is_same_v<typename DT::command, aggregate_t>) 
            new (g.m_next) TheT{};
    }

    if constexpr (sizeof...(acs)>0)
        initRanges(t, acs...);

    g.release();
}


std::size_t get_size(std::size_t sz) { return sz; }
template<class T>
std::enable_if_t<!std::is_integral_v<T>, std::size_t> get_size(const T& t) { return t.m_count; }

auto convert_arg(std::size_t sz) { return arg(sz); }
template<class T>
std::enable_if_t<!std::is_integral_v<T>, const T&> convert_arg(const T& t) { return t; }


template<class... Args, class... Args2>
auto make_contiguous_objects(const Args2&... args) -> std::tuple<Rng<Args>...>
{
    auto layout = make_contiguous_layout<Args...>(get_size(args)...);

    initRanges(layout, convert_arg(args)...);

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
