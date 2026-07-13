cmake_minimum_required(VERSION 3.16)
project(dish_counter_host_tests LANGUAGES CXX)

enable_testing()

function(enable_strict_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /WX)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Werror)
    endif()
endfunction()

add_library(dish_counter_logic STATIC ../firmware/main/dish_counter_logic.cpp)
target_include_directories(dish_counter_logic PUBLIC ../firmware/main)
target_compile_features(dish_counter_logic PUBLIC cxx_std_17)
enable_strict_warnings(dish_counter_logic)

add_executable(dish_counter_logic_test dish_counter_logic_test.cpp)
target_link_libraries(dish_counter_logic_test PRIVATE dish_counter_logic)
enable_strict_warnings(dish_counter_logic_test)

add_executable(dish_counter_simulator ../tools/dish_counter_simulator.cpp)
target_link_libraries(dish_counter_simulator PRIVATE dish_counter_logic)
enable_strict_warnings(dish_counter_simulator)

add_test(NAME dish_counter_logic COMMAND dish_counter_logic_test)

add_test(
    NAME simulator_clean_count
    COMMAND dish_counter_simulator ${CMAKE_CURRENT_SOURCE_DIR}/../examples/traces/clean-count.trace)
set_tests_properties(simulator_clean_count PROPERTIES PASS_REGULAR_EXPRESSION "Tray 1 result: COUNTED")

add_test(
    NAME simulator_noisy_count
    COMMAND dish_counter_simulator ${CMAKE_CURRENT_SOURCE_DIR}/../examples/traces/noisy-count.trace)
set_tests_properties(simulator_noisy_count PROPERTIES PASS_REGULAR_EXPRESSION "Tray 1 result: COUNTED")

add_test(
    NAME simulator_rejected_tray
    COMMAND dish_counter_simulator ${CMAKE_CURRENT_SOURCE_DIR}/../examples/traces/rejected-tray.trace)
set_tests_properties(simulator_rejected_tray PROPERTIES PASS_REGULAR_EXPRESSION "Tray 1 result: REJECTED")
