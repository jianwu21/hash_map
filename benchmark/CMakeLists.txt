cmake_minimum_required(VERSION 3.6)    

project(hash_map_bechmark)

add_executable(hash_map_bechmark hash_map_benchmark.cpp)
target_link_libraries(hash_map_bechmark hash_map)
target_compile_options(hash_map_bechmark PRIVATE -mavx2)
target_compile_features(hash_map_bechmark INTERFACE cxx_std_17)
