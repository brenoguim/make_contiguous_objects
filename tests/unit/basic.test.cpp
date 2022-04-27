#include <catch.hpp>
#include <mco.hpp>

#include <iostream>
#include <cstring>

TEST_CASE( "Basic", "[make_contiguous_layout]" )
{
    auto l = xtd::make_contiguous_layout<int, long, char>(2, 1, 8);

    std::cout << "int* : " << std::get<0>(l).begin() << " to " << std::get<0>(l).end() << std::endl;
    std::cout << "long*: " << std::get<1>(l).begin() << " to " << std::get<1>(l).end() << std::endl;
    std::cout << "char*: " << (void*)std::get<2>(l).begin() << " to " << (void*)std::get<2>(l).end() << std::endl;

    ::operator delete((void*)std::get<0>(l).begin());
}

TEST_CASE( "Basic - holes", "[make_contiguous_layout]" )
{
    auto l = xtd::make_contiguous_layout<char, int, long>(1, 2, 1);

    std::cout << "char* : " << (void*)std::get<0>(l).begin() << " to " << (void*)std::get<0>(l).end() << std::endl;
    std::cout << "int*: " << std::get<1>(l).begin() << " to " << std::get<1>(l).end() << std::endl;
    std::cout << "long*: " << (void*)std::get<2>(l).begin() << " to " << (void*)std::get<2>(l).end() << std::endl;

    ::operator delete((void*)std::get<0>(l).begin());
}
