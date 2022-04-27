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
struct init
{
    init(std::size_t count) : m_count(count) {}
    std::size_t m_count;

    void addRequiredBytes(std::size_t& pos)
    {
        pos += findDistanceOfNextAlignedPosition(pos, alignof(T)) + m_count*sizeof(T);
    }

    void setRange(Rng<T>& rng, std::byte*& mem)
    {
        mem += findDistanceOfNextAlignedPosition(reinterpret_cast<std::uintptr_t>(mem), alignof(T));
        rng.m_begin = reinterpret_cast<T*>(mem);
        mem += m_count*sizeof(T);
        rng.m_end = reinterpret_cast<T*>(mem);
    }
};

template<class... Args>
auto make_contiguous_layout(init<Args>... args) -> std::tuple<Rng<Args>...>
{
    std::size_t numBytes = 0;
    ((args.addRequiredBytes(numBytes), ...));

    auto mem = (std::byte*) ::operator new(numBytes);

    std::tuple<Rng<Args>...> r;

    std::apply([&] (auto&... targs) {
        (args.setRange(targs, mem),...);
    }, r);

    return r;
}

}
