/**
 * @file power_of_two_growth_policy.h
 * @author jian wu (jian.wu_93@foxmail.com)
 * @brief Grow policy(power of 2)
 * @version 0.1
 * @date 2024-02-16
 * 
 * 
 */
#pragma once

#include <cassert>
#include <cstddef>
#include <limits>

namespace jw::details
{

struct power_of_two_growth_policy
{
    static std::size_t compute_index(std::size_t hash, std::size_t capacity)
    {
        return hash & (capacity - 1);
    }

    static std::size_t compute_closest_capacity(std::size_t min_capacity)
    {
        std::size_t highest_capacity =
            (std::size_t{1} << (std::numeric_limits<std::size_t>::digits - 1));

        if (min_capacity > highest_capacity)
        {
            assert(false && "Maximum capacity for the dense_hash_map reached.");
            return highest_capacity;
        }

        --min_capacity;

        for (int i = 1; i < std::numeric_limits<std::size_t>::digits; i <<= 1)
        {
            min_capacity |= min_capacity >> i;
        }

        return ++min_capacity;
    }

    static std::size_t minimum_capacity() 
    { 
        return 8u; 
    }
};

}
