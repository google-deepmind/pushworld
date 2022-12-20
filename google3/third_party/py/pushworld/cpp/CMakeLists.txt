cmake_minimum_required(VERSION 3.10)

project(PushWorld VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(Boost 1.78.0 COMPONENTS hash test)
include_directories(include ${Boost_INCLUDE_DIRS})

add_library(pushworld_puzzle src/pushworld_puzzle.cc)
target_link_libraries(pushworld_puzzle ${Boost_LIBRARIES})
set_target_properties(
    pushworld_puzzle
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

add_library(search src/search/search.cc)
target_link_libraries(search pushworld_puzzle)
set_target_properties(
    search
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

add_library(novelty_heuristic src/heuristics/novelty.cc)
set_target_properties(
    novelty_heuristic
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

add_library(weighted_sum_heuristic src/heuristics/weighted_sum.cc)
set_target_properties(
    weighted_sum_heuristic
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

add_library(random_action_iterator src/search/random_action_iterator.cc)
set_target_properties(
    random_action_iterator
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

add_library(domain_transition_graph src/heuristics/domain_transition_graph.cc)
set_target_properties(
    domain_transition_graph
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

add_library(recursive_graph_distance src/heuristics/recursive_graph_distance.cc)
target_link_libraries(recursive_graph_distance domain_transition_graph pushworld_puzzle)
set_target_properties(
    recursive_graph_distance
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

add_executable(run_planner src/run_planner.cc)
target_link_libraries(
    run_planner
    pushworld_puzzle
    search
    random_action_iterator
    recursive_graph_distance
    novelty_heuristic
    weighted_sum_heuristic
)
set_target_properties(
    run_planner
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

add_subdirectory(test)
