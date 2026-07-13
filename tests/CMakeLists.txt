cmake_minimum_required(VERSION 3.16)
project(dish_counter_host_tests LANGUAGES CXX)

enable_testing()

add_executable(dish_counter_logic_test
    dish_counter_logic_test.cpp
    ../firmware/main/dish_counter_logic.cpp)
target_include_directories(dish_counter_logic_test PRIVATE ../firmware/main)
target_compile_features(dish_counter_logic_test PRIVATE cxx_std_17)
target_compile_options(dish_counter_logic_test PRIVATE -Wall -Wextra -Werror)

add_test(NAME dish_counter_logic COMMAND dish_counter_logic_test)
