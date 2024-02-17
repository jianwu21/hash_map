/**
 * @file hash_map.h
 * @author jian wu (jian.wu_93@foxmail.com)
 * @brief A simple implementation of Hash map. Using open addressing with
 * linear probing to solve hash collision
 * 
 * Advantages:
 * 1. Linear probing ensures cache efficency. High performance in lookup
 *    https://en.wikipedia.org/wiki/Open_addressing
 *    https://en.wikipedia.org/wiki/Linear_probing
 * 
 * 2. Deletes items by rearranging items and marking slots as empty instead of
 *    marking items as deleted. High performance could be kept when there
 *    is a high rate of churn (many paired inserts and deletes) since otherwise
 *    most slots would be marked deleted and probing would end up scanning
 *    most of the table.
 *    https://en.wikipedia.org/wiki/Lazy_deletion
 * 
 * 3. Doesn't use the allocator unless load factor grows beyond 50%.
 * 
 * Disadvantages:
 * 1. Maximum load factor is up to 50%, memory inefficient.
 * 2. Memory is not reclaimed on erase.
 * 
 * @version 0.1
 * @date 2024-02-16
 * 
 * 
 */
#pragma once

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>

#include "power_of_two_growth_policy.h"

namespace jw 
{

static constexpr const float DEFAULT_MAX_LOAD_FACTOR = 0.500f;

template <typename Key, 
          typename T, 
          typename Hash         = std::hash<Key>,
          typename KeyEqual     = std::equal_to<void>,
          typename Allocator    = std::allocator<std::pair<Key, T>>,
          typename GrowthPolicy = details::power_of_two_growth_policy>
class hash_map : private GrowthPolicy
{
public:
    using GrowthPolicy::compute_index;
    using GrowthPolicy::compute_closest_capacity;
    using GrowthPolicy::minimum_capacity;

    using key_type        = Key;
    using mapped_type     = T;
    using value_type      = std::pair<Key, T>;
    using size_type       = std::size_t;
    using hasher          = Hash;
    using key_equal       = KeyEqual;
    using allocator_type  = Allocator;
    using reference       = value_type &;
    using const_reference = const value_type &;
    using buckets         = std::vector<value_type, allocator_type>;

    template <typename ContT, typename IterVal> 
    struct hash_map_iterator 
    {
        using difference_type   = std::ptrdiff_t;
        using value_type        = IterVal;
        using pointer           = value_type*;
        using reference         = value_type&;
        using iterator_category = std::forward_iterator_tag;

        bool operator==(const hash_map_iterator &other) const 
        {
            return other.hm_ == hm_ && other.idx_ == idx_;
        }

        bool operator!=(const hash_map_iterator &other) const 
        {
            return !(other == *this);
        }

        hash_map_iterator &operator++() 
        {
            ++idx_;
            advance_past_empty();
            return *this;
        }

        reference operator*() const 
        { 
            return hm_->m_buckets[idx_]; 
        }
        
        pointer operator->() const 
        { 
            return &hm_->m_buckets[idx_]; 
        }

    private:
        explicit hash_map_iterator(ContT* hm) : hm_(hm) 
        { 
            advance_past_empty(); 
        }

        explicit hash_map_iterator(ContT* hm, size_type idx) : hm_(hm), idx_(idx) { }

        template <typename OtherContT, typename OtherIterVal>
        hash_map_iterator(const hash_map_iterator<OtherContT, OtherIterVal>& other)
            : hm_(other.hm_), idx_(other.idx_) {}

        void advance_past_empty() 
        {
            while (idx_ < hm_->m_buckets.size() &&
                key_equal()(hm_->m_buckets[idx_].first, hm_->m_empty_key)) 
            {
                ++idx_;
            }
        }

        ContT* hm_ = nullptr;
        typename ContT::size_type idx_ = 0;
        friend ContT;
    };

    using iterator       = hash_map_iterator<hash_map, value_type>;
    using const_iterator = hash_map_iterator<const hash_map, const value_type>;

public:
    hash_map() : hash_map(minimum_capacity(), key_type())
    { }

    hash_map(size_type bucket_count) : hash_map(bucket_count, key_type())
    { }

    hash_map(size_type bucket_count, key_type empty_key,
            const allocator_type &alloc = allocator_type())
        : m_empty_key(empty_key), m_buckets(alloc) 
    {
        std::size_t count = compute_closest_capacity(bucket_count);
        m_buckets.resize(count, std::make_pair(m_empty_key, T()));
    }

    hash_map(const hash_map &other, size_type bucket_count)
        : hash_map(bucket_count, other.m_empty_key, other.get_allocator()) 
    {
        for (auto it = other.cbegin(); it != other.cend(); ++it) 
        {
            insert(*it);
        }
    }

    allocator_type get_allocator() const noexcept 
    {
        return m_buckets.get_allocator();
    }

    // Iterators
    iterator begin() noexcept 
    { 
        return iterator(this); 
    }

    const_iterator begin() const noexcept 
    { 
        return const_iterator(this); 
    }

    const_iterator cbegin() const noexcept 
    { 
        return const_iterator(this); 
    }

    iterator end() noexcept 
    { 
        return iterator(this, m_buckets.size()); 
    }

    const_iterator end() const noexcept 
    {
        return const_iterator(this, m_buckets.size());
    }

    const_iterator cend() const noexcept 
    {
        return const_iterator(this, m_buckets.size());
    }

    // Capacity
    bool empty() const noexcept 
    { 
        return size() == 0; 
    }

    size_type size() const noexcept
    { 
        return m_size; 
    }

    size_type max_size() const noexcept 
    { 
        return m_buckets.max_size() / 2; 
    }

    // Modifiers
    void clear() noexcept 
    {
        for (auto& b : m_buckets) 
        {
            if (b.first != m_empty_key) 
            {
                b.first = m_empty_key;
            }
        }
        m_size = 0;
    }

    std::pair<iterator, bool> insert(const value_type &value) 
    {
        return emplace_impl(value.first, value.second);
    }

    std::pair<iterator, bool> insert(value_type&& value) 
    {
        return emplace_impl(value.first, std::move(value.second));
    }

    template <typename... Args>
    std::pair<iterator, bool> emplace(Args &&... args) 
    {
        return emplace_impl(std::forward<Args>(args)...);
    }

    void erase(iterator it) 
    { 
        erase_impl(it); 
    }

    size_type erase(const key_type &key) 
    { 
        return erase_impl(key); 
    }

    template <typename K> 
    size_type erase(const K& x) 
    { 
        return erase_impl(x); 
    }

    void swap(hash_map &other) noexcept 
    {
        std::swap(m_buckets, other.m_buckets);
        std::swap(m_size, other.m_size);
        std::swap(m_empty_key, other.m_empty_key);
    }

    // Lookup
    mapped_type &at(const key_type &key) 
    { 
        return at_impl(key); 
    }

    template <typename K> 
    mapped_type &at(const K &x) 
    { 
        return at_impl(x); 
    }

    const mapped_type &at(const key_type &key) const 
    { 
        return at_impl(key); 
    }

    template <typename K> 
    const mapped_type &at(const K &x) const 
    {
        return at_impl(x);
    }

    mapped_type &operator[](const key_type &key) 
    {
        return emplace_impl(key).first->second;
    }

    size_type count(const key_type &key) const 
    { 
        return count_impl(key); 
    }

    template <typename K> 
    size_type count(const K &x) const 
    {
        return count_impl(x);
    }

    iterator find(const key_type &key) 
    { 
        return find_impl(key); 
    }

    template <typename K> 
    iterator find(const K &x) 
    { 
        return find_impl(x); 
    }

    const_iterator find(const key_type &key) const 
    { 
        return find_impl(key); 
    }

    template <typename K> 
    const_iterator find(const K &x) const 
    {
        return find_impl(x);
    }

    // Bucket interface
    size_type bucket_count() const noexcept 
    { 
        return m_buckets.size(); 
    }

    size_type max_bucket_count() const noexcept 
    { 
        return m_buckets.max_size(); 
    }

    // Hash policy
    float max_load_factor() const noexcept 
    { 
        return m_max_load_factor; 
    }

    void rehash(size_type count) 
    {
        count = std::max(minimum_capacity(), count);
        count = std::max(count, static_cast<size_type>(size() / max_load_factor()));

        count = compute_closest_capacity(count);
        hash_map other(*this, count);
        swap(other);
    }

    void reserve(std::size_t count)
    {
        rehash(std::ceil(count / max_load_factor()));
    }

    void check_for_rehash()
    {
        if (size() + 1 > bucket_count() * max_load_factor())
        {
            rehash(bucket_count() << 2);
        }
    }

    // Observers
    hasher hash_function() const 
    { 
        return hasher(); 
    }

    key_equal key_eq() const 
    { 
        return key_equal(); 
    }

private:
    template <typename K, typename... Args>
    std::pair<iterator, bool> emplace_impl(const K& key, Args&& ...args) 
    {
        assert(!key_equal()(m_empty_key, key) && "empty key shouldn't be used");

        check_for_rehash();

        for (size_t idx = key_to_idx(key); ; idx = probe_next(idx)) 
        {
            if (key_equal()(m_buckets[idx].first, m_empty_key)) 
            {
                m_buckets[idx].second = mapped_type(std::forward<Args>(args)...);
                m_buckets[idx].first = key;
                m_size++;
                return {iterator(this, idx), true};
            } 
            else if (key_equal()(m_buckets[idx].first, key)) 
            {
                return {iterator(this, idx), false};
            }
        }
    }

    void erase_impl(iterator it) 
    {
        size_t bucket = it.idx_;

        for (size_t idx = probe_next(bucket); ; idx = probe_next(idx)) 
        {
            if (key_equal()(m_buckets[idx].first, m_empty_key)) 
            {
                m_buckets[bucket].first = m_empty_key;
                --m_size;

                return;
            }

            size_t ideal = key_to_idx(m_buckets[idx].first);
            if (diff(bucket, ideal) < diff(idx, ideal)) 
            {
                // swap, bucket is closer to ideal than idx
                m_buckets[bucket] = m_buckets[idx];
                bucket = idx;
            }
        }
    }

    template <typename K> 
    size_type erase_impl(const K &key) 
    {
        auto it = find_impl(key);
        if (it != end()) {
            erase_impl(it);
            return 1;
        }

        return 0;
    }

    template <typename K> 
    mapped_type &at_impl(const K &key) 
    {
        iterator it = find_impl(key);
        if (it != end()) 
        {
            return it->second;
        }
        throw std::out_of_range("hash_map::at");
    }

    template <typename K> 
    const mapped_type &at_impl(const K &key) const 
    {
        return const_cast<hash_map*>(this)->at_impl(key);
    }

    template <typename K> 
    size_t count_impl(const K& key) const 
    {
        return find_impl(key) == end() ? 0 : 1;
    }

    template <typename K> 
    iterator find_impl(const K &key) 
    {
        assert(!key_equal()(m_empty_key, key) && "empty key shouldn't be used");

        for (size_t idx = key_to_idx(key); ; idx = probe_next(idx)) 
        {
            if (key_equal()(m_buckets[idx].first, key)) 
            {
                return iterator(this, idx);
            }

            if (key_equal()(m_buckets[idx].first, m_empty_key)) 
            {
                return end();
            }
        }
    }

    template <typename K> 
    const_iterator find_impl(const K &key) const 
    {
        return const_cast<hash_map*>(this)->find_impl(key);
    }

    template <typename K>
    size_t key_to_idx(const K& key) const noexcept(noexcept(hasher()(key))) 
    {
        return compute_index(hasher()(key), m_buckets.size());
    }

    size_t probe_next(size_t idx) const noexcept 
    {
        return compute_index(idx + 1, m_buckets.size());
    }

    size_t diff(size_t a, size_t b) const noexcept 
    {
        return compute_index(m_buckets.size() + (a - b), m_buckets.size());
    }

private:
    key_type m_empty_key;
    buckets  m_buckets;
    size_t   m_size            = 0;
    float    m_max_load_factor = DEFAULT_MAX_LOAD_FACTOR;
};
}
