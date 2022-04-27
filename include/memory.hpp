#include <memory>

namespace xtd
{

template<class T>
struct Rng
{
    T *begin, *end;
};

template<class T>
struct init
{
    init(std::size_t count) : m_count(count) {}
    std::size_t count;
};

template<class... Args>
auto make_contiguous_layout(init<Args>...) -> std::tuple<Rng<Args>...>
{
    using R = std::tuple<Rng<Args>...>;

    return R{};
}

}
