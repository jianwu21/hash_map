/**
 * @file count_allocator.h
 * @author jian wu (jian.wu_93@foxmail.com)
 * @brief Custome allocator(mainly for memory allocated measurement)
 * @version 0.1
 * @date 2024-02-16
 * 
 * 
 */
#pragma once

#include <limits>

namespace jw::count 
{

template<typename T>
class count_allocator;

class memory_count 
{
public:
    memory_count(const memory_count&) = delete;
    memory_count(memory_count&&)      = delete;

    memory_count& operator=(const memory_count&) = delete;
    memory_count& operator=(memory_count&&)      = delete;

    template<class T> friend class count_allocator;

    static memory_count* instance() 
    {
        static memory_count* instance_ = new memory_count();

        return instance_;
    }

    void ResetPeakBytes() 
    {
        m_peakBytes = 0;
    }

    size_t cur_bytes() const 
    {
        return m_curBytes;
    }

    size_t peak_bytes() const 
    {
        return m_peakBytes;
    }

    void Reset()
    {
        m_peakBytes = 0;
        m_curBytes = 0;
    }
private:
    memory_count(): m_curBytes(0), m_peakBytes(0) {};
    size_t m_curBytes;
    size_t m_peakBytes;

    void UseMemory(size_t bytes) 
    {
        m_curBytes += bytes;
        m_peakBytes = std::max(m_curBytes, m_peakBytes);
    }

    void ReclaimMemory(size_t bytes) 
    {
        m_curBytes -= bytes;
    }

};

template <typename T>
class count_allocator 
{
public:
    constexpr static std::size_t HUGE_PAGE_SIZE            = 1 << 21; // 2 MiB
    constexpr static std::size_t ALLOC_HUGE_PAGE_THRESHOLD = 1 << 16; // 64KB

    using value_type = T;

    count_allocator() = default;

    template <class U>
    constexpr count_allocator(const count_allocator<U> &) noexcept 
    { }

    count_allocator(const count_allocator&) = default;

    friend bool operator==(const count_allocator&, const count_allocator&) 
    {
        return true;
    }

    value_type* allocate(std::size_t n) 
    {
        void* p = m_std_allocator.allocate(n);

        size_t used_bytes = n * sizeof(value_type);
        memory_count::instance()->UseMemory(used_bytes);

        if (p == nullptr)
            throw std::bad_alloc();

        return static_cast<value_type*>(p);
    }

    void deallocate(T* p, std::size_t n) 
    {
        m_std_allocator.deallocate(p, n);
        size_t bytes_num = n * sizeof(value_type);
        memory_count::instance()->ReclaimMemory(bytes_num);
    }
private:
    std::allocator<value_type> m_std_allocator;
};

template<class T>
using Allocator = count_allocator<T>;

}
