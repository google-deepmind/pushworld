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
  };

  int estimate_cost_to_goal(const State& state) {
    int i, j;
    int novelty = 2;

    // The novelty is 1 if any pair of objects are in a combination of positions
    // that has never occurred in any state previously provided to this method.
    for (i = 0; i < m_state_size; i++) {
      const auto& p_i = state[i];

      for (j = i + 1; j < m_state_size; j++) {
        const auto& p_j = state[j];
        const PositionPair p{p_i, p_j};

        if (m_visited_position_pairs[i][j].insert(p).second) {
          novelty = 1;
        }
      }
    }

    // The novelty is 0 if any object is in a position that has never occurred
    // in any state previously provided to this method.
    for (i = 0; i < m_state_size; i++) {
      if (m_visited_positions[i].insert(state[i]).second) {
        novelty = 0;
      }
    }

    return novelty;
  };
};

}  // namespace

/* Checks `NoveltyHeuristic` on a sequence of states with known costs. */
BOOST_AUTO_TEST_CASE(test_novelty_heuristic) {
  NoveltyHeuristic heuristic(4);
  BOOST_TEST(heuristic.estimate_cost_to_goal({1, 2, 3, 4}) == 0);
  BOOST_TEST(heuristic.estimate_cost_to_goal({2, 3, 4, 5}) == 0);
  BOOST_TEST(heuristic.estimate_cost_to_goal({1, 3, 4, 5}) == 1);
  BOOST_TEST(heuristic.estimate_cost_to_goal({2, 3, 3, 5}) == 1);
  BOOST_TEST(heuristic.estimate_cost_to_goal({1, 3, 3, 5}) == 2);
  BOOST_TEST(heuristic.estimate_cost_to_goal({1, 3, 3, 4}) == 1);
  BOOST_TEST(heuristic.estimate_cost_to_goal({1, 3, 5, 4}) == 0);
  BOOST_TEST(heuristic.estimate_cost_to_goal({1, 3, 5, 4}) == 2);
}

/* Checks that `NoveltyHeuristic` and `BaselineNoveltyHeuristic` return
 * identical costs. */
BOOST_AUTO_TEST_CASE(test_alternative_implementation) {
  PushWorldPuzzle puzzle("puzzles/file_parsing.pwp");

  StateSet visited_states;
  std::deque<State> frontier;

  const auto& initial_state = puzzle.getInitialState();
  frontier.push_back(initial_state);
  visited_states.insert(initial_state);

  NoveltyHeuristic heuristic(initial_state.size());
  BaselineNoveltyHeuristic baseline_heuristic(initial_state.size());

  std::vector<int> cost_counts{0, 0, 0};

  // Explore states breadth-first, and check that 10,000 states all have the
  // same heuristic values.
  const int num_test_states = 1000;

  for (int i = 0; i < num_test_states; i++) {
    const auto state = frontier.front();
    frontier.pop_front();

    const auto cost = heuristic.estimate_cost_to_goal(state);
    const auto baseline_cost = baseline_heuristic.estimate_cost_to_goal(state);

    BOOST_TEST(cost == baseline_cost);
    cost_counts[cost]++;

    for (int action = 0; action < NUM_ACTIONS; action++) {
      const auto next_state = puzzle.getNextState(state, action);
      if (visited_states.find(next_state) == visited_states.end()) {
        // This state has not yet been visited.
        visited_states.insert(next_state);
        frontier.push_back(next_state);
      }
    }
  }

  BOOST_TEST(cost_counts[0] == 75);
  BOOST_TEST(cost_counts[1] == 546);
  BOOST_TEST(cost_counts[2] == 379);

  // Check that all states were counted during this test.
  BOOST_TEST(std::accumulate(cost_counts.begin(), cost_counts.end(), 0) ==
             num_test_states);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace heuristic
}  // namespace pushworld
