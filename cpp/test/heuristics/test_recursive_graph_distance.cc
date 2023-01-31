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

#include "heuristics/recursive_graph_distance.h"
#include "pushworld_puzzle.h"

namespace pushworld {
namespace heuristic {

BOOST_AUTO_TEST_SUITE(recursive_graph_distance)

/* Checks `RecursiveGraphDistanceHeuristic` on a simple puzzle with known costs.
 */
BOOST_AUTO_TEST_CASE(test_trivial) {
  const auto puzzle = std::make_shared<PushWorldPuzzle>("puzzles/trivial.pwp");
  RecursiveGraphDistanceHeuristic rgd(puzzle);

  // Repeat each test twice to check that internal caching doesn't change the
  // result.
  RelativeState s;
  s.state = puzzle->getInitialState();
  BOOST_TEST(rgd.estimate_cost_to_goal(s) == 2);
  BOOST_TEST(rgd.estimate_cost_to_goal(s) == 2);

  s = puzzle->getNextState(s.state, RIGHT);
  BOOST_TEST(rgd.estimate_cost_to_goal(s) == 3);
  BOOST_TEST(rgd.estimate_cost_to_goal(s) == 3);

  s = puzzle->getNextState(s.state, UP);
  BOOST_TEST(rgd.estimate_cost_to_goal(s) == 4);
  BOOST_TEST(rgd.estimate_cost_to_goal(s) == 4);
}

/* Checks `RecursiveGraphDistanceHeuristic` on a puzzle with multiple goals. */
BOOST_AUTO_TEST_CASE(test_multiple_goals) {
  const auto puzzle =
      std::make_shared<PushWorldPuzzle>("puzzles/multiple_goals.pwp");
  RecursiveGraphDistanceHeuristic rgd(puzzle);

  RelativeState s0;
  s0.state = puzzle->getInitialState();
  BOOST_TEST(rgd.estimate_cost_to_goal(s0) == 4);

  RelativeState s = puzzle->getNextState(s0.state, LEFT);
  BOOST_TEST(rgd.estimate_cost_to_goal(s) == 4);

  s = puzzle->getNextState(s0.state, RIGHT);
  BOOST_TEST(rgd.estimate_cost_to_goal(s) == 4);

  s = puzzle->getNextState(s0.state, UP);
  BOOST_TEST(rgd.estimate_cost_to_goal(s) == 6);

  s = puzzle->getNextState(s0.state, DOWN);
  BOOST_TEST(rgd.estimate_cost_to_goal(s) == 6);
}

/**
 * Checks `RecursiveGraphDistanceHeuristic` on puzzles that involve pushing a
 * goal object using another object other than the agent.
 */
BOOST_AUTO_TEST_CASE(test_transitive_pushing) {
  auto puzzle =
      std::make_shared<PushWorldPuzzle>("puzzles/transitive_pushing.pwp");
  RelativeState s0;
  s0.state = puzzle->getInitialState();

  RecursiveGraphDistanceHeuristic h0(puzzle, false /* fewest_tools */);
  BOOST_TEST(h0.estimate_cost_to_goal(s0) == 3);
  // Check that internal caching doesn't change the result.
  BOOST_TEST(h0.estimate_cost_to_goal(s0) == 3);

  RecursiveGraphDistanceHeuristic h1(puzzle);
  BOOST_TEST(h1.estimate_cost_to_goal(s0) == 4);
  // Check that internal caching doesn't change the result.
  BOOST_TEST(h1.estimate_cost_to_goal(s0) == 4);

  // Estimated cost = 4 + 3 + 2 = 9
  puzzle = std::make_shared<PushWorldPuzzle>(
      "puzzles/necessary_transitive_pushing1.pwp");
  s0.state = puzzle->getInitialState();
  RecursiveGraphDistanceHeuristic h2(puzzle);
  BOOST_TEST(h2.estimate_cost_to_goal(s0) == 9);
  // Check that internal caching doesn't change the result.
  BOOST_TEST(h2.estimate_cost_to_goal(s0) == 9);

  // Estimated cost = 0 + 0 + 2 = 2
  puzzle = std::make_shared<PushWorldPuzzle>(
      "puzzles/necessary_transitive_pushing2.pwp");
  s0.state = puzzle->getInitialState();
  RecursiveGraphDistanceHeuristic h3(puzzle);
  BOOST_TEST(h3.estimate_cost_to_goal(s0) == 2);

  // Estimated cost = 2 + 0 + 2 = 4
  puzzle = std::make_shared<PushWorldPuzzle>(
      "puzzles/necessary_transitive_pushing3.pwp");
  s0.state = puzzle->getInitialState();
  RecursiveGraphDistanceHeuristic h4(puzzle);
  BOOST_TEST(h4.estimate_cost_to_goal(s0) == 4);

  // Estimated cost = 2 + 0 + 0 = 2
  puzzle = std::make_shared<PushWorldPuzzle>(
      "puzzles/blocked_transitive_pushing1.pwp");
  s0.state = puzzle->getInitialState();
  RecursiveGraphDistanceHeuristic h5(puzzle);
  BOOST_TEST(h5.estimate_cost_to_goal(s0) == 2);

  // Estimated cost = 1 + 2 + 0 = 3
  puzzle = std::make_shared<PushWorldPuzzle>(
      "puzzles/blocked_transitive_pushing2.pwp");
  s0.state = puzzle->getInitialState();
  RecursiveGraphDistanceHeuristic h6(puzzle);
  BOOST_TEST(h6.estimate_cost_to_goal(s0) == 3);

  puzzle = std::make_shared<PushWorldPuzzle>("puzzles/trivial_tool2.pwp");
  s0.state = puzzle->getInitialState();
  RecursiveGraphDistanceHeuristic h7(puzzle);
  BOOST_TEST(h7.estimate_cost_to_goal(s0) == 4);

  puzzle = std::make_shared<PushWorldPuzzle>("puzzles/shortest_path_tool.pwp");
  s0.state = puzzle->getInitialState();
  // Without using tool: 11 + 2 = 13
  RecursiveGraphDistanceHeuristic h8(puzzle);
  BOOST_TEST(h8.estimate_cost_to_goal(s0) == 13);
  // Using tool: 1 + 3 + 2 = 6
  RecursiveGraphDistanceHeuristic h9(puzzle, false);
  BOOST_TEST(h9.estimate_cost_to_goal(s0) == 6);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace heuristic
}  // namespace pushworld
