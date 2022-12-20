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

#include "heuristics/domain_transition_graph.h"
#include "pushworld_puzzle.h"

namespace pushworld {
namespace heuristic {

BOOST_AUTO_TEST_SUITE(domain_transition_graph)

/* Checks `build_feasible_movement_graphs` on puzzles with known movement
 * graphs. */
BOOST_AUTO_TEST_CASE(test_build_feasible_movement_graphs) {
  PushWorldPuzzle puzzle("puzzles/trivial.pwp");
  const auto movement_graphs = build_feasible_movement_graphs(puzzle);

  FeasibleMovementGraph agent_movement_graph = {
      {xy_to_position(1, 2), {xy_to_position(2, 2)}},
      {xy_to_position(2, 1), {xy_to_position(2, 2), xy_to_position(3, 1)}},
      {xy_to_position(2, 2),
       {xy_to_position(1, 2), xy_to_position(3, 2), xy_to_position(2, 1),
        xy_to_position(2, 3)}},
      {xy_to_position(2, 3), {xy_to_position(2, 2), xy_to_position(3, 3)}},
      {xy_to_position(3, 1), {xy_to_position(2, 1), xy_to_position(3, 2)}},
      {xy_to_position(3, 2),
       {xy_to_position(3, 1), xy_to_position(3, 3), xy_to_position(2, 2)}},
      {xy_to_position(3, 3), {xy_to_position(2, 3), xy_to_position(3, 2)}},
  };
  BOOST_TEST(*(movement_graphs.at(AGENT)) == agent_movement_graph);

  FeasibleMovementGraph m0_movement_graph = {
      {xy_to_position(1, 2), {}},
      {xy_to_position(1, 3), {}},
      {xy_to_position(2, 1), {}},
      {xy_to_position(2, 2),
       {xy_to_position(1, 2), xy_to_position(3, 2), xy_to_position(2, 1),
        xy_to_position(2, 3)}},
      {xy_to_position(2, 3), {xy_to_position(1, 3)}},
      {xy_to_position(3, 1), {}},
      {xy_to_position(3, 2), {xy_to_position(3, 1), xy_to_position(3, 3)}},
      {xy_to_position(3, 3), {}},
  };
  BOOST_TEST(*(movement_graphs.at(1)) == m0_movement_graph);

  PushWorldPuzzle tool_puzzle("puzzles/trivial_tool.pwp");
  const auto tool_movement_graphs = build_feasible_movement_graphs(tool_puzzle);

  FeasibleMovementGraph target_graph = {
      {xy_to_position(4, 1), {}},
      {xy_to_position(4, 2), {xy_to_position(4, 1)}},
      {xy_to_position(4, 3), {xy_to_position(4, 2), xy_to_position(4, 4)}},
      {xy_to_position(4, 4), {}}};
  BOOST_TEST(tool_movement_graphs.at(AGENT)->size() == 15);
  BOOST_TEST(*(tool_movement_graphs.at(1)) == target_graph);
  BOOST_TEST(tool_movement_graphs.at(2)->size() == 12);
}

/* Checks `PathDistances` on movement graphs with known distances. */
BOOST_AUTO_TEST_CASE(test_path_distances) {
  PushWorldPuzzle puzzle("puzzles/trivial.pwp");
  const auto movement_graphs = build_feasible_movement_graphs(puzzle);

  const auto agent_distances = PathDistances(movement_graphs.at(AGENT));
  const auto object_distances = PathDistances(movement_graphs.at(1));

  // Run every test twice to check cached distances.
  for (int i = 0; i < 2; i++) {
    float d;

    // Agent distances:
    d = agent_distances.getDistance(xy_to_position(1, 2), xy_to_position(1, 2));
    BOOST_TEST(d == 0);

    d = agent_distances.getDistance(xy_to_position(1, 2), xy_to_position(2, 2));
    BOOST_TEST(d == 1);

    d = agent_distances.getDistance(xy_to_position(1, 2), xy_to_position(3, 3));
    BOOST_TEST(d == 3);

    d = agent_distances.getDistance(xy_to_position(1, 2), xy_to_position(3, 1));
    BOOST_TEST(d == 3);

    d = agent_distances.getDistance(xy_to_position(2, 3), xy_to_position(3, 1));
    BOOST_TEST(d == 3);

    d = agent_distances.getDistance(xy_to_position(2, 3), xy_to_position(2, 2));
    BOOST_TEST(d == 1);

    d = agent_distances.getDistance(xy_to_position(2, 3), xy_to_position(2, 3));
    BOOST_TEST(d == 0);

    d = agent_distances.getDistance(xy_to_position(1, 1), xy_to_position(2, 3));
    BOOST_TEST(d == std::numeric_limits<float>::infinity());

    d = agent_distances.getDistance(xy_to_position(2, 2), xy_to_position(1, 1));
    BOOST_TEST(d == std::numeric_limits<float>::infinity());

    d = agent_distances.getDistance(xy_to_position(3, 1), xy_to_position(1, 3));
    BOOST_TEST(d == std::numeric_limits<float>::infinity());

    // Goal object distances:
    d = object_distances.getDistance(xy_to_position(2, 2),
                                     xy_to_position(3, 1));
    BOOST_TEST(d == 2);

    d = object_distances.getDistance(xy_to_position(2, 2),
                                     xy_to_position(1, 3));
    BOOST_TEST(d == 2);

    d = object_distances.getDistance(xy_to_position(2, 2),
                                     xy_to_position(3, 3));
    BOOST_TEST(d == 2);

    d = object_distances.getDistance(xy_to_position(2, 2),
                                     xy_to_position(2, 3));
    BOOST_TEST(d == 1);

    d = object_distances.getDistance(xy_to_position(3, 2),
                                     xy_to_position(3, 1));
    BOOST_TEST(d == 1);

    d = object_distances.getDistance(xy_to_position(3, 1),
                                     xy_to_position(3, 1));
    BOOST_TEST(d == 0);

    d = object_distances.getDistance(xy_to_position(2, 1),
                                     xy_to_position(3, 1));
    BOOST_TEST(d == std::numeric_limits<float>::infinity());

    d = object_distances.getDistance(xy_to_position(1, 2),
                                     xy_to_position(1, 3));
    BOOST_TEST(d == std::numeric_limits<float>::infinity());

    d = object_distances.getDistance(xy_to_position(3, 1),
                                     xy_to_position(2, 2));
    BOOST_TEST(d == std::numeric_limits<float>::infinity());
  }
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace heuristic
}  // namespace pushworld
