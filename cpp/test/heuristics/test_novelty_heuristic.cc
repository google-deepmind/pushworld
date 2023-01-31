// Copyright 2022 DeepMind Technologies Limited
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <boost/test/unit_test.hpp>
#include <deque>
#include <numeric>
#include <unordered_set>
#include <vector>

#include "heuristics/novelty.h"
#include "pushworld_puzzle.h"

namespace pushworld {
namespace heuristic {

BOOST_AUTO_TEST_SUITE(novelty_heuristic)

namespace {

/**
 * This class is equivalent to `NoveltyHeuristic`, but it is less optimized and
 * more readable.
 */
class BaselineNoveltyHeuristic {
 private:
  int m_state_size;
  std::vector<std::unordered_set<Position2D>> m_visited_positions;
  std::vector<std::vector<std::unordered_set<PositionPair, PositionPairHash>>>
      m_visited_position_pairs;

 public:
  BaselineNoveltyHeuristic(const int state_size) : m_state_size(state_size) {
    m_visited_positions.resize(state_size);
    m_visited_position_pairs.resize(state_size);

    for (auto& position_pairs : m_visited_position_pairs) {
      position_pairs.resize(state_size);
    }
  }

  int estimate_cost_to_goal(const RelativeState& relative_state) {
    int i, j;
    int novelty = 3;

    // The novelty is 2 if any pair of objects are in a combination of positions
    // that has never occurred in any state previously provided to this method.
    for (i = 0; i < m_state_size; i++) {
      const auto& p_i = relative_state.state[i];

      for (j = i + 1; j < m_state_size; j++) {
        const auto& p_j = relative_state.state[j];
        const PositionPair p{p_i, p_j};

        if (m_visited_position_pairs[i][j].insert(p).second) {
          novelty = 2;
        }
      }
    }

    // The novelty is 1 if any object is in a position that has never occurred
    // in any state previously provided to this method.
    for (i = 0; i < m_state_size; i++) {
      if (m_visited_positions[i].insert(relative_state.state[i]).second) {
        novelty = 1;
      }
    }

    return novelty;
  }
};

}  // namespace

/* Checks `NoveltyHeuristic` on a sequence of states with known costs. */
BOOST_AUTO_TEST_CASE(test_novelty_heuristic) {
  NoveltyHeuristic heuristic(4);
  BOOST_TEST(
      heuristic.estimate_cost_to_goal(RelativeState{
          .state = {1, 2, 3, 4}, .moved_object_indices = {0, 1, 2, 3}}) == 1);
  BOOST_TEST(
      heuristic.estimate_cost_to_goal(RelativeState{
          .state = {2, 3, 4, 5}, .moved_object_indices = {0, 1, 2, 3}}) == 1);
  BOOST_TEST(heuristic.estimate_cost_to_goal(RelativeState{
                 .state = {1, 3, 4, 5}, .moved_object_indices = {0}}) == 2);
  BOOST_TEST(heuristic.estimate_cost_to_goal(RelativeState{
                 .state = {2, 3, 3, 5}, .moved_object_indices = {2}}) == 2);
  BOOST_TEST(heuristic.estimate_cost_to_goal(RelativeState{
                 .state = {1, 3, 3, 5}, .moved_object_indices = {0, 2}}) == 3);
  BOOST_TEST(heuristic.estimate_cost_to_goal(RelativeState{
                 .state = {1, 3, 3, 4}, .moved_object_indices = {3}}) == 2);
  BOOST_TEST(heuristic.estimate_cost_to_goal(RelativeState{
                 .state = {1, 3, 5, 4}, .moved_object_indices = {2}}) == 1);
  BOOST_TEST(heuristic.estimate_cost_to_goal(RelativeState{
                 .state = {1, 3, 5, 4}, .moved_object_indices = {}}) == 3);
}

/* Checks that `NoveltyHeuristic` and `BaselineNoveltyHeuristic` return
 * identical costs. */
BOOST_AUTO_TEST_CASE(test_alternative_implementation) {
  PushWorldPuzzle puzzle("puzzles/file_parsing.pwp");
  const State& initial_state = puzzle.getInitialState();

  std::vector<int> all_object_indices(initial_state.size());
  for (int i = 0; i < initial_state.size(); i++) {
    all_object_indices[i] = i;
  }
  const RelativeState initial_relative_state{initial_state,
                                             std::move(all_object_indices)};

  StateSet visited_states;
  visited_states.insert(initial_state);
  
  std::deque<RelativeState> frontier;
  frontier.push_back(initial_relative_state);

  NoveltyHeuristic heuristic(initial_state.size());
  BaselineNoveltyHeuristic baseline_heuristic(initial_state.size());

  std::vector<int> cost_counts{0, 0, 0, 0};

  // Explore states breadth-first, and check that 10,000 states all have the
  // same heuristic values.
  const int num_test_states = 1000;

  for (int i = 0; i < num_test_states; i++) {
    const RelativeState relative_state = frontier.front();
    frontier.pop_front();

    const auto cost = heuristic.estimate_cost_to_goal(relative_state);
    const auto baseline_cost =
        baseline_heuristic.estimate_cost_to_goal(relative_state);

    BOOST_TEST(cost == baseline_cost);
    cost_counts[cost]++;

    for (int action = 0; action < NUM_ACTIONS; action++) {
      RelativeState next_relative_state =
          puzzle.getNextState(relative_state.state, action);
      if (visited_states.find(next_relative_state.state) ==
          visited_states.end()) {
        // This state has not yet been visited.
        visited_states.insert(next_relative_state.state);
        frontier.push_back(std::move(next_relative_state));
      }
    }
  }

  BOOST_TEST(cost_counts[1] == 75);
  BOOST_TEST(cost_counts[2] == 546);
  BOOST_TEST(cost_counts[3] == 379);

  // Check that all states were counted during this test.
  BOOST_TEST(std::accumulate(cost_counts.begin(), cost_counts.end(), 0) ==
             num_test_states);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace heuristic
}  // namespace pushworld
