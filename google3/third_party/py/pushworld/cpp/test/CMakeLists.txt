
add_executable(
    run_tests
    main.cc
    test_pushworld_puzzle.cc
    heuristics/test_domain_transition_graph.cc
    heuristics/test_novelty_heuristic.cc
    heuristics/test_recursive_graph_distance.cc
    heuristics/test_weighted_sum.cc
    search/test_best_first_search.cc
    search/test_priority_queue.cc
    search/test_random_action_iterator.cc
    search/test_search.cc
)
target_link_libraries(
    run_tests
    pushworld_puzzle search novelty_heuristic weighted_sum_heuristic
    domain_transition_graph recursive_graph_distance random_action_iterator
    ${Boost_LIBRARIES}
)
set_target_properties(
    run_tests
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/test"
)

# Create a symlink to data required by the tests
ADD_CUSTOM_TARGET(test_puzzles ALL
                  COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_SOURCE_DIR}/test/puzzles ${CMAKE_BINARY_DIR}/bin/test/puzzles)
