#include <catch.hpp>
#include <mco.hpp>

#include <iostream>
#include <cstring>
#include <vector>
#include <string>

template<class T>
void default_construct(T* begin, T* end)
{
    for (auto it = begin; it != end; ++it) new (it) T();
}

// UBSAN will catch any misaligned write
template<class T>
void writeToRange(xtd::span<T>& rng)
{
    for (auto b = rng.begin(); b != rng.end(); ++b)
        *b = T();
}

template<class Tup>
struct Guard
{
    Guard(Tup t) : m_t(t) {}

    ~Guard()
    {
        std::apply([] (auto&... rngs) {
            (default_construct(rngs.begin(), rngs.end()),...);
        }, m_t);

        std::apply([] (auto&... rngs) {
            (writeToRange(rngs),...);
        }, m_t);

        xtd::destroy_contiguous_objects(m_t);
    }

    Tup m_t;
};

TEST_CASE( "Basic", "[make_contiguous_layout]" )
{
    auto g = Guard{xtd::make_contiguous_layout<int, long, char>(2, 1, 8)};
}

TEST_CASE( "Basic - holes", "[make_contiguous_layout]" )
{
    auto g = Guard{xtd::make_contiguous_layout<char, int, long>(1, 2, 1)};
}

template<class A, class B, class C>
void test_combinations()
{
    for (int x = 0; x < 32; ++x)
    for (int y = 0; y < 32; ++y)
    for (int z = 0; z < 32; ++z)
    {
        Guard{xtd::make_contiguous_layout<A, B, C>(x,y,z)};
        Guard{xtd::make_contiguous_layout<A, C, B>(x,y,z)};
        Guard{xtd::make_contiguous_layout<B, A, C>(x,y,z)};
        Guard{xtd::make_contiguous_layout<B, C, A>(x,y,z)};
        Guard{xtd::make_contiguous_layout<C, A, B>(x,y,z)};
        Guard{xtd::make_contiguous_layout<C, B, A>(x,y,z)};
    }

}

TEST_CASE( "Basic - stress", "[make_contiguous_layout]" )
{
    test_combinations<int, char, long>();
    test_combinations<int, char, std::string>();
    test_combinations<int, std::string, long>();
    test_combinations<std::string, char, long>();
    test_combinations<std::string, std::string, long>();
    test_combinations<std::string, std::string, std::string>();
}

TEST_CASE( "Basic - mco", "[make_contiguous_objects]" )
{
    struct Foo
    {
        int i;
        long k;
    };

    std::vector<std::string> v(15, "test");

    auto g = xtd::make_contiguous_objects<int, long, char, Foo, std::string, std::string>(
            2,
            xtd::arg(xtd::uninit, 1),
            xtd::arg(xtd::ctor, 8, -17),
            xtd::arg(xtd::aggregate, 4, -18, -19),
            xtd::arg(xtd::input_iterator, 10, v.begin()),
            xtd::arg(xtd::functor, 10, [] { return "this is a very long string to avoid sbo"; })
            );

    for (auto& e : std::get<0>(g)) { CHECK(e == 0); }
    for (auto& e : std::get<2>(g)) { CHECK(e == -17); }
    for (auto& e : std::get<3>(g)) { CHECK(e.i == -18); CHECK(e.k == -19); }
    for (auto& e : std::get<4>(g)) { CHECK(e == "test"); }
    for (auto& e : std::get<5>(g)) { CHECK(e == "this is a very long string to avoid sbo"); }

    xtd::destroy_contiguous_objects(g);
}
