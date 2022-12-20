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
#include "search/search.h"

namespace pushworld {
namespace search {

BOOST_AUTO_TEST_SUITE(search)

/* Checks that `backtrackPlan` returns expected results. */
BOOST_AUTO_TEST_CASE(test_backtrack_plan) {
  pushworld::PushWorldPuzzle world("puzzles/trivial.pwp");
  const auto initial_state = world.getInitialState();

  auto search_node = std::make_shared<SearchNode>(nullptr, initial_state);
  auto plan = backtrackPlan(world, search_node);
  BOOST_TEST(plan.empty());

  pushworld::Plan expected_plan{pushworld::RIGHT, pushworld::DOWN,
                                pushworld::RIGHT, pushworld::UP};
  pushworld::State state = initial_state;

  for (const auto action : expected_plan) {
    state = world.getNextState(state, action);
    search_node = std::make_shared<SearchNode>(search_node, state);
  }

  plan = backtrackPlan(world, search_node);
  BOOST_TEST(plan == expected_plan);

  plan = backtrackPlan(world, search_node->parent);
  expected_plan = {pushworld::RIGHT, pushworld::DOWN, pushworld::RIGHT};
  BOOST_TEST(plan == expected_plan);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace search
}  // namespace pushworld
