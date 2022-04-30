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

}
