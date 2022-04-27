#pragma once

#include <random>

struct rng
{
    inline rng() : dev{}, gen( dev() ) {}

    constexpr auto generate(auto min, auto max)
    { return std::uniform_int_distribution(min, max)(gen); }

private:
    std::random_device dev;
    std::mt19937 gen;
};
