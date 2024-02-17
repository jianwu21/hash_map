# hash_map
## How to build
```shell
cd {work_path}
mkdir build && cd build
cmake ..
cd .. 
cmake --build build/
```
## Usage
The same as `std::unordered_map`

## Benchmark

A simple benchmark `benchmark/hash_map_benchmark.cpp` is included with the sources. The
benchmark simulates 100,000 inserting firstly, then lookup and deleting.

I ran this benchmark on the following configuration:

- Intel(R) Core(TM) i5-7267U CPU @ 3.10GHz, 2 Cores
- Darwin 22.2.0
- clang++ 13.1.6

When memory is reserved for working set (`hash_map_bechmark -c 100000 -i 100000000 -r 1`):

| name                | insert mean(ns) | insert max(ns) | lookup mean(ns) | lookup max(ns) | delete mean(ns) | delete max(ns) | Memory(bytes) |
| ------------------- | --------------- | -------------- | --------------- | -------------- | --------------- | -------------- | ------------- |
| jw::hash_map        | 439             | 1062858        | 412             | 170189         | 454             | 47512          | 0             |   
| std::unordered_map  | 909             | 616495         | 594             | 108112         | 468             | 533182         | 8800000       |

When memory is not reserved for working set(allocator is called according to growth policy) (`hash_map_bechmark -c 100000 -i 1000000000 -r 0`):

| name                | insert mean(ns) | insert max(ns) | lookup mean(ns) | lookup max(ns) | delete mean(ns) | delete max(ns) | Memory(bytes) |
| ------------------- | --------------- | -------------- | --------------- | -------------- | --------------- | -------------- | ------------- |
| jw::hash_map        | 1110            | 53697474       | 418             | 135780         | 463             | 179172         | 37748160      |   
| std::unordered_map  | 1176            | 5992961        | 603             | 25660928       | 474             | 509101         | 9623016       |
