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
#include <memory>

#include "pushworld_puzzle.h"
#include "search/best_first_search.h"
#include "search/priority_queue.h"

namespace pushworld {
namespace search {

BOOST_AUTO_TEST_SUITE(best_first_search_suite)

namespace {

/* Always returns zero cost to the goal. */
class NullHeuristic : public pushworld::heuristic::Heuristic<int> {
 public:
  int estimate_cost_to_goal(const pushworld::State& state) override {
    return 0;
  };
};

/* Computes the sum of Manhattan distances of each object from its goal
 * position. */
class ManhattanDistanceHeuristic : public pushworld::heuristic::Heuristic<int> {
 private:
  pushworld::Goal m_goal;

 public:
  ManhattanDistanceHeuristic(const pushworld::Goal& goal) : m_goal(goal){};

  int estimate_cost_to_goal(const pushworld::State& state) override {
    int cost = 0;
    int goal_x, goal_y, object_x, object_y;

    for (int i = 0; i < m_goal.size();) {
      pushworld::position_to_xy(m_goal[i++], goal_x, goal_y);
      pushworld::position_to_xy(state[i], object_x, object_y);
      cost += abs(goal_x - object_x) + abs(goal_y - object_y);
    }

    return cost;
  };
};

/* A heuristic that negates the `ManhattanDistanceHeuristic` cost. */
class InvertedManhattanDistanceHeuristic
    : public pushworld::heuristic::Heuristic<int> {
 private:
  pushworld::Goal m_goal;

 public:
  InvertedManhattanDistanceHeuristic(const pushworld::Goal& goal)
      : m_goal(goal){};

 public:
  int estimate_cost_to_goal(const pushworld::State& state) override {
    return -ManhattanDistanceHeuristic(m_goal).estimate_cost_to_goal(state);
  };
};

}  // namespace

BOOST_AUTO_TEST_CASE(test_best_first_search) {
  NullHeuristic null_heuristic;
  priority_queue::FibonacciPriorityQueue<std::shared_ptr<SearchNode>, int>
      frontier;
  pushworld::StateSet visited_states;

  pushworld::PushWorldPuzzle easy_search_puzzle("puzzles/easy_search.pwp");
  ManhattanDistanceHeuristic distance_heuristic(easy_search_puzzle.getGoal());
  InvertedManhattanDistanceHeuristic inv_distance_heuristic(
      easy_search_puzzle.getGoal());

  auto plan = best_first_search(easy_search_puzzle, distance_heuristic,
                                frontier, visited_states);
  BOOST_TEST(plan->size() == 3);
  // The number of visited states depends on the order in which actions are
  // expanded.
  BOOST_TEST(visited_states.size() >= 9);
  BOOST_TEST(visited_states.size() <= 12);
  BOOST_TEST(easy_search_puzzle.isValidPlan(*plan));
  BOOST_TEST(!frontier.empty());

  // The inverted distance heuristic performs much worse than the distance
  // heuristic.
  plan = best_first_search(easy_search_puzzle, inv_distance_heuristic, frontier,
                           visited_states);
  BOOST_TEST(visited_states.size() > 100);
  BOOST_CHECK(easy_search_puzzle.isValidPlan(*plan));
  BOOST_CHECK(!frontier.empty());

  // The search should terminate if no solution exists.
  pushworld::PushWorldPuzzle no_solution_puzzle("puzzles/no_solution.pwp");
  plan = best_first_search(no_solution_puzzle, null_heuristic, frontier,
                           visited_states);
  BOOST_CHECK(plan == std::nullopt);
  BOOST_CHECK(frontier.empty());
  BOOST_CHECK(visited_states.size() == 9);

  // Check a puzzle with only one possible solution.
  pushworld::PushWorldPuzzle trivial_puzzle("puzzles/trivial.pwp");
  plan = best_first_search(trivial_puzzle, null_heuristic, frontier);
  pushworld::Plan expected_plan{pushworld::RIGHT, pushworld::DOWN,
                                pushworld::RIGHT, pushworld::UP};
  BOOST_TEST(*plan == expected_plan);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace search
}  // namespace pushworld
