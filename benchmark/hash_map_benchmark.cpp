/**
 * @file hash_map_benchmark.cpp
 * @author jian wu (jian.wu_93@foxmail.com)
 * @brief A simple benchmark for jw::hash_map and std::unordered_map
 * Key: int64_t, Value: an array of char, Hasher: _mm_crc32_u64
 * 1. We insert 100,000 element to a map and measure average/max time cost
 * 2. We lookup 100,000 a random key value in the map and measure average/max time cost 
 * 
 * @version 0.1
 * @date 2024-02-16
 * 
 * 
 */

#include <chrono>
#include <iostream>
#include <iomanip>
#include <nmmintrin.h> // _mm_crc32_u64
#include <random>
#include <unistd.h>
#include <unordered_map>

#include <jw/count_allocator.h>
#include <jw/hash_map.h>

class stop_watch
{
public:
    void start()
    {
        m_start = std::chrono::steady_clock::now();
    }

    int64_t elapsedTimeNanoseconds()
    {
        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        std::chrono::nanoseconds duration = now - m_start;

        return std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
    }
private:
    std::chrono::steady_clock::time_point m_start;
};

void printUsage()
{
    std::cerr << "hash_map_benchmark" << std::endl
              << "usage: hash_map_benchmark [-c count] [-i iters] [-r reserved]" << std::endl
              << std::endl;
}

int main(int argc, char *argv[]) {
    (void)argc, (void)argv;

    size_t count = 100000;
    size_t iters = 1000000;
    bool callReserve = false;
    int type = -1;

    int opt;
    while ((opt = getopt(argc, argv, "i:c:r:")) != -1) 
    {
        switch (opt) 
        {
        case 'i':
            iters = std::stol(optarg);
            break;
        case 'c':
            count = std::stol(optarg);
            break;
        case 'r':
            callReserve = std::stol(optarg);
            break;
        default:
            printUsage();
            break;
        }
    }

    if (optind != argc) {
        printUsage(); 
        exit(1);
    }

    using key = int64_t;
    struct value 
    {
        char buf[64];
    };

    struct hash {
        size_t operator()(size_t h) const noexcept 
        { 
            return _mm_crc32_u64(0, h); 
        }
    };

    auto test = [&](const std::string name, auto &m) 
    {
        std::minstd_rand gen(0);
        std::uniform_int_distribution<int> ud(2, count);

        stop_watch watch;
        stop_watch itrWatch;

        watch.start();
        int64_t maxInsert = 0;

        jw::count::memory_count::instance()->Reset();
        std::size_t start_mem = jw::count::memory_count::instance()->cur_bytes();

        for (size_t i = 0; i < count; ++i) 
        {
            const int64_t val = i + 1;
            itrWatch.start();
            m.insert({val, {}});
            maxInsert = std::max(itrWatch.elapsedTimeNanoseconds(), maxInsert);
        }

        std::size_t end_mem = jw::count::memory_count::instance()->cur_bytes();
        std::size_t memoryUsed = end_mem - start_mem;

        int64_t insertDur = watch.elapsedTimeNanoseconds();

        watch.start();

        int64_t maxLookup = 0;
        for (size_t i = 0; i < iters; ++i) 
        {
            const int64_t val = ud(gen);
            itrWatch.start();
            m.find(val);
            maxLookup = std::max(itrWatch.elapsedTimeNanoseconds(), maxLookup);
        }

        int64_t lookupDuration = watch.elapsedTimeNanoseconds();

        watch.start();
        int64_t maxErase = 0;
        for (size_t i = 0; i < iters; ++i) 
        {
            const int64_t val = ud(gen);
            itrWatch.start();
            m.erase(val);
            maxErase = std::max(itrWatch.elapsedTimeNanoseconds(), maxErase);
        }

        int64_t eraseDuration = watch.elapsedTimeNanoseconds();

        std::cout << std::left << std::setw(20) <<  name << "|"
                  << std::setw(17) << insertDur / count      << "|" 
                  << std::setw(17) << maxInsert              << "|"
                  << std::setw(17) << lookupDuration / iters << "|"
                  << std::setw(17) << maxLookup              << "|"
                  << std::setw(17) << eraseDuration / iters  << "|"
                  << std::setw(17) << maxErase               << "|"
                  << std::setw(17) << memoryUsed             << std::endl;
    };

    std::cout << std::left << std::setw(20) <<  "name" << "|"
              << std::setw(17) << "insert mean(ns) " << "|" 
              << std::setw(17) << "insert max(ns) "  << "|"
              << std::setw(17) << "lookup mean(ns) " << "|"
              << std::setw(17) << "lookup max(ns) "  << "|"
              << std::setw(17) << "delete mean(ns) " << "|"
              << std::setw(17) << "delete max(ns) "  << "|"
              << std::setw(17) << "Memory(bytes)"    << std::endl;

    if (type == -1 || type == 1) 
    {
        jw::hash_map<key, 
                     value, 
                     hash, 
                     std::equal_to<>,
                     jw::count::Allocator<std::pair<key, value>>> hm;
        if (callReserve)
            hm.reserve(count);
        
        test("jw::hash_map", hm);
    }

    if (type == -1 || type == 4) 
    {
        std::unordered_map<key, 
                           value, 
                           hash,
                           std::equal_to<>, 
                           jw::count::Allocator<std::pair<const key, value>>> hm;
        if (callReserve)
            hm.reserve(count);
        test("std::unordered_map", hm);
    }

    return 0;
}
