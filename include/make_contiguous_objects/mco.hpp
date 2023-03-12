#include <memory>

namespace xtd
{

template<class It>
void reverse_destroy(It begin, It end)
{
    if constexpr (!std::is_trivially_destructible_v<std::remove_reference_t<decltype(*begin)>>)
    {
        while (begin != end)
        {
            --end;
            std::destroy_at(end);
        }
    }
}

template<class T>
struct span
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
void setRange(ArraySize<T>& init, span<T>& rng, std::byte*& mem)
{
    mem += findDistanceOfNextAlignedPosition(reinterpret_cast<std::uintptr_t>(mem), alignof(T));
    rng.m_begin = reinterpret_cast<T*>(mem);
    mem += init.numBytes();
    rng.m_end = reinterpret_cast<T*>(mem);
}

template<class... Args>
auto make_contiguous_layout(ArraySize<Args>... args) -> std::tuple<span<Args>...>
{
    std::size_t numBytes = 0;
    ((addRequiredBytes(args, numBytes), ...));

    auto mem = (std::byte*) ::operator new(numBytes);

    std::tuple<span<Args>...> r;

    std::apply([&] (auto&... targs) {
        (setRange(args, targs, mem),...);
    }, r);

    return r;
}

struct uninit_t{}; static constexpr uninit_t uninit;
struct ctor_t{}; static constexpr ctor_t ctor;
struct aggregate_t{}; static constexpr aggregate_t aggregate;
struct input_iterator_t{}; static constexpr input_iterator_t input_iterator;
struct functor_t{}; static constexpr functor_t functor;

template<class C, class T>
struct InitializerConfiguration
{
    using command = C;
    std::size_t m_count;
    T m_args;
};

template<class... Args>
auto arg(ctor_t, std::size_t count, Args&&... args)
{
    return InitializerConfiguration<ctor_t, decltype(std::forward_as_tuple(args...))>{count, std::forward_as_tuple(args...)};
}

template<class... Args>
auto arg(aggregate_t, std::size_t count, Args&&... args)
{
    return InitializerConfiguration<aggregate_t, decltype(std::forward_as_tuple(args...))>{count, std::forward_as_tuple(args...)};
}

auto arg(std::size_t count) { return arg(ctor, count); }
auto arg(uninit_t, std::size_t count) { return InitializerConfiguration<uninit_t, int>{count, 0}; }

template<class It>
auto arg(input_iterator_t, std::size_t count, const It& it) { return InitializerConfiguration<input_iterator_t, It>{count, it}; }

template<class Fn>
auto arg(functor_t, std::size_t count, const Fn& fn) { return InitializerConfiguration<functor_t, const Fn&>{count, fn}; }

template<class T>
struct RangeGuard
{
    using type = T;
    RangeGuard(span<T>& rng) : m_rng(rng), m_next(m_rng.begin()) {}

    void release()
    {
        m_next = nullptr;
    }

    ~RangeGuard()
    {
        if (m_next)
            reverse_destroy(m_rng.begin(), m_next);
    }

    span<T>& m_rng;
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

        if constexpr (std::is_same_v<typename DT::command, ctor_t>) 
            std::apply([&] (auto&&... args) { new (g.m_next) TheT(args...); }, ac.m_args);

        if constexpr (std::is_same_v<typename DT::command, aggregate_t>) 
            std::apply([&] (auto&&... args) { new (g.m_next) TheT{args...}; }, ac.m_args);

        if constexpr (std::is_same_v<typename DT::command, input_iterator_t>) 
            new (g.m_next) TheT(*ac.m_args++);

        if constexpr (std::is_same_v<typename DT::command, functor_t>) 
            new (g.m_next) TheT(ac.m_args());
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
std::enable_if_t<!std::is_integral_v<T>, T> convert_arg(const T& t) { return t; }

struct MemGuard
{
    ~MemGuard() { ::operator delete(m_mem); }
    void release() { m_mem = nullptr; }
    void* m_mem;
};

template<class... Args, class... Initializers>
auto make_contiguous_objects(Initializers... args) -> std::tuple<span<Args>...>
{
    auto layout = make_contiguous_layout<Args...>(get_size(args)...);
    MemGuard mg{std::get<0>(layout).begin()};

    initRanges(layout, convert_arg(args)...);

    mg.release();
    return layout;
}

template<class... Args>
void destroy_contiguous_objects(const std::tuple<Args...>& t)
{
    std::apply([] (auto&... rngs) {
        (reverse_destroy(rngs.begin(), rngs.end()),...);
    }, t);

    ::operator delete((void*)std::get<0>(t).begin());
}

template<class T, class U>
T* get_adjacent_address(U* end)
{
    auto endi = (std::uintptr_t)end;
    return (T*) (endi + findDistanceOfNextAlignedPosition(endi, alignof(T)));
} 

}
