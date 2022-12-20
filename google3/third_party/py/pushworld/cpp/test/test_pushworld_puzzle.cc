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

#include <stdlib.h>  // srand, rand

#include <boost/test/unit_test.hpp>
#include <string>
#include <unordered_set>

#include "pushworld_puzzle.h"

namespace pushworld {

BOOST_AUTO_TEST_SUITE(pushworld)

/* Checks `position_to_xy` and `xy_to_position`. */
BOOST_AUTO_TEST_CASE(test_position_conversions) {
  int x, y;
  int target_x, target_y;

  std::srand(0);

  position_to_xy(xy_to_position(1, 1), x, y);
  BOOST_TEST(x == 1);
  BOOST_TEST(y == 1);

  for (int i = 0; i < 100; i++) {
    // This isn't a uniform distribution, but it doesn't matter here.
    target_x = rand() % 10000;
    target_y = rand() % 10000;
    position_to_xy(xy_to_position(target_x, target_y), x, y);
    BOOST_TEST(x == target_x);
    BOOST_TEST(y == target_y);
  }
}

/* Checks that `Position2D` instances can be added to compute relative
 * positions. */
BOOST_AUTO_TEST_CASE(test_position_arithmetic) {
  int x, y;
  int target_x, target_y;
  int dx, dy;

  std::srand(0);

  position_to_xy(xy_to_position(1, 1) + xy_to_position(2, 2), x, y);
  BOOST_TEST(x == 3);
  BOOST_TEST(y == 3);

  position_to_xy(xy_to_position(-1, -1) + xy_to_position(2, 2), x, y);
  BOOST_TEST(x == 1);
  BOOST_TEST(y == 1);

  position_to_xy(xy_to_position(3, -7) + xy_to_position(10, 11), x, y);
  BOOST_TEST(x == 13);
  BOOST_TEST(y == 4);

  for (int i = 0; i < 100; i++) {
    // This isn't a uniform distribution, but it doesn't matter here.
    target_x = rand() % 5000 + 2500;
    target_y = rand() % 5000 + 2500;
    dx = rand() % 5000 - 2500;
    dy = rand() % 5000 - 2500;
    position_to_xy(
        xy_to_position(target_x - dx, target_y - dy) + xy_to_position(dx, dy),
        x, y);
    BOOST_TEST(x == target_x);
    BOOST_TEST(y == target_y);
  }
}

/* Checks that the agent moves as expected with each action. */
BOOST_AUTO_TEST_CASE(test_agent_movement) {
  State next_state;
  const State initial_state = {xy_to_position(1, 1)};
  int num_moveables = 1;
  int num_goal_entities = 0;
  const Goal goal = {};
  int x, y;

  ObjectCollisions object_collisions(num_moveables);
  PushWorldPuzzle puzzle(initial_state, goal, object_collisions);

  // Verify that the agent can move in all 4 directions
  next_state = puzzle.getNextState(initial_state, LEFT);
  position_to_xy(next_state[0], x, y);
  BOOST_TEST(x == 0);
  BOOST_TEST(y == 1);

  next_state = puzzle.getNextState(initial_state, RIGHT);
  position_to_xy(next_state[0], x, y);
  BOOST_TEST(x == 2);
  BOOST_TEST(y == 1);

  next_state = puzzle.getNextState(initial_state, UP);
  position_to_xy(next_state[0], x, y);
  BOOST_TEST(x == 1);
  BOOST_TEST(y == 0);

  next_state = puzzle.getNextState(initial_state, DOWN);
  position_to_xy(next_state[0], x, y);
  BOOST_TEST(x == 1);
  BOOST_TEST(y == 2);

  // Add a left agent wall
  object_collisions.static_collisions[LEFT][AGENT].insert(xy_to_position(1, 1));
  PushWorldPuzzle puzzle2(initial_state, goal, object_collisions);

  next_state = puzzle2.getNextState(initial_state, LEFT);
  BOOST_TEST(next_state[0] == xy_to_position(1, 1));

  next_state = puzzle2.getNextState(initial_state, RIGHT);
  BOOST_TEST(next_state[0] == xy_to_position(2, 1));

  // Add a right agent wall
  object_collisions.static_collisions[RIGHT][AGENT].insert(
      xy_to_position(1, 1));
  PushWorldPuzzle puzzle3(initial_state, goal, object_collisions);

  next_state = puzzle3.getNextState(initial_state, RIGHT);
  BOOST_TEST(next_state[0] == xy_to_position(1, 1));

  // Add a top agent wall
  object_collisions.static_collisions[UP][AGENT].insert(xy_to_position(1, 1));
  PushWorldPuzzle puzzle4(initial_state, goal, object_collisions);

  next_state = puzzle4.getNextState(initial_state, UP);
  BOOST_TEST(next_state[0] == xy_to_position(1, 1));

  // Add a bottom agent wall
  object_collisions.static_collisions[DOWN][AGENT].insert(xy_to_position(1, 1));
  PushWorldPuzzle puzzle5(initial_state, goal, object_collisions);

  next_state = puzzle5.getNextState(initial_state, DOWN);
  BOOST_TEST(next_state[0] == xy_to_position(1, 1));
}

/* Checks that the agent can push an object. */
BOOST_AUTO_TEST_CASE(test_pushing) {
  State initial_state = {xy_to_position(1, 1), xy_to_position(2, 1)};

  ObjectCollisions object_collisions(initial_state.size());
  object_collisions.dynamic_collisions[RIGHT][0][1].insert(
      xy_to_position(-1, 0));

  PushWorldPuzzle puzzle(initial_state, Goal(), object_collisions);

  State next_state = puzzle.getNextState(initial_state, DOWN);
  BOOST_TEST(next_state[0] == xy_to_position(1, 2));
  BOOST_TEST(next_state[1] == xy_to_position(2, 1));

  next_state = puzzle.getNextState(initial_state, RIGHT);
  BOOST_TEST(next_state[0] == xy_to_position(2, 1));
  BOOST_TEST(next_state[1] == xy_to_position(3, 1));

  next_state = puzzle.getNextState(next_state, RIGHT);
  BOOST_TEST(next_state[0] == xy_to_position(3, 1));
  BOOST_TEST(next_state[1] == xy_to_position(4, 1));
}

/* Checks the agent can push an object by using another "tool" object in
 * between. */
BOOST_AUTO_TEST_CASE(test_transitive_pushing) {
  State initial_state = {xy_to_position(1, 1), xy_to_position(3, 1),
                         xy_to_position(5, 1)};
  State s1, s2;

  ObjectCollisions object_collisions(initial_state.size());
  object_collisions.dynamic_collisions[RIGHT][0][1].insert(
      xy_to_position(-1, 0));
  object_collisions.dynamic_collisions[RIGHT][1][2].insert(
      xy_to_position(-1, 0));

  PushWorldPuzzle puzzle(initial_state, Goal(), object_collisions);

  s1 = puzzle.getNextState(initial_state, DOWN);
  BOOST_TEST(s1[0] == xy_to_position(1, 2));
  BOOST_TEST(s1[1] == xy_to_position(3, 1));
  BOOST_TEST(s1[2] == xy_to_position(5, 1));

  s1 = puzzle.getNextState(initial_state, RIGHT);
  BOOST_TEST(s1[0] == xy_to_position(2, 1));
  BOOST_TEST(s1[1] == xy_to_position(3, 1));
  BOOST_TEST(s1[2] == xy_to_position(5, 1));

  s2 = puzzle.getNextState(s1, RIGHT);
  BOOST_TEST(s2[0] == xy_to_position(3, 1));
  BOOST_TEST(s2[1] == xy_to_position(4, 1));
  BOOST_TEST(s2[2] == xy_to_position(5, 1));

  s1 = puzzle.getNextState(s2, RIGHT);
  BOOST_TEST(s1[0] == xy_to_position(4, 1));
  BOOST_TEST(s1[1] == xy_to_position(5, 1));
  BOOST_TEST(s1[2] == xy_to_position(6, 1));

  s2 = puzzle.getNextState(s1, UP);
  BOOST_TEST(s2[0] == xy_to_position(4, 0));
  BOOST_TEST(s2[1] == xy_to_position(5, 1));
  BOOST_TEST(s2[2] == xy_to_position(6, 1));
}

/* Checks `Pushpuzzle.satisfiesGoal` */
BOOST_AUTO_TEST_CASE(test_goal_checking) {
  State initial_state = {xy_to_position(1, 1), xy_to_position(2, 2),
                         xy_to_position(3, 3)};
  Goal goal = {xy_to_position(2, 5)};

  PushWorldPuzzle puzzle(initial_state, goal, ObjectCollisions());

  State s1 = {xy_to_position(1, 1), xy_to_position(2, 5), xy_to_position(3, 3)};
  BOOST_TEST(puzzle.satisfiesGoal(s1));
  State s2 = {xy_to_position(2, 1), xy_to_position(2, 5), xy_to_position(3, 5)};
  BOOST_TEST(puzzle.satisfiesGoal(s2));
  State s3 = {xy_to_position(1, 1), xy_to_position(3, 5), xy_to_position(3, 3)};
  BOOST_TEST(!puzzle.satisfiesGoal(s3));
  State s4 = {xy_to_position(2, 1), xy_to_position(2, 2), xy_to_position(3, 6)};
  BOOST_TEST(!puzzle.satisfiesGoal(s4));

  Goal goal2 = {xy_to_position(2, 5), xy_to_position(3, 6)};
  PushWorldPuzzle puzzle2(initial_state, goal2, ObjectCollisions());

  State s5 = {xy_to_position(5, 1), xy_to_position(2, 5), xy_to_position(3, 6)};
  BOOST_TEST(puzzle2.satisfiesGoal(s5));
  State s6 = {xy_to_position(2, 8), xy_to_position(2, 5), xy_to_position(3, 6)};
  BOOST_TEST(puzzle2.satisfiesGoal(s6));
  State s7 = {xy_to_position(1, 1), xy_to_position(2, 5), xy_to_position(3, 3)};
  BOOST_TEST(!puzzle2.satisfiesGoal(s7));
  State s8 = {xy_to_position(1, 1), xy_to_position(2, 2), xy_to_position(3, 6)};
  BOOST_TEST(!puzzle2.satisfiesGoal(s8));
}

/* Checks that a small PushWorld puzzle file is correctly parsed. */
BOOST_AUTO_TEST_CASE(test_trivial_file_parsing) {
  std::string filename = "puzzles/trivial.pwp";
  PushWorldPuzzle puzzle(filename);

  const auto& goal = puzzle.getGoal();
  BOOST_TEST_REQUIRE(goal.size() == 1);
  BOOST_TEST(goal[0] == xy_to_position(3, 1));

  const auto& initial_state = puzzle.getInitialState();
  BOOST_TEST_REQUIRE(initial_state.size() == 2);
  BOOST_TEST(initial_state[0] == xy_to_position(1, 2));  // S
  BOOST_TEST(initial_state[1] == xy_to_position(2, 2));  // M0

  // Verify the collisions
  const auto& object_collisions = puzzle.getObjectCollisions();
  const auto& static_collisions = object_collisions.static_collisions;
  const auto& dynamic_collisions = object_collisions.dynamic_collisions;

  BOOST_TEST(static_collisions[LEFT][AGENT].size() == 3);
  BOOST_TEST(static_collisions[RIGHT][AGENT].size() == 3);
  BOOST_TEST(static_collisions[UP][AGENT].size() == 3);
  BOOST_TEST(static_collisions[DOWN][AGENT].size() == 3);

  BOOST_TEST((static_collisions[LEFT][AGENT].find(xy_to_position(2, 1)) !=
              static_collisions[LEFT][AGENT].end()));
  BOOST_TEST((static_collisions[LEFT][AGENT].find(xy_to_position(1, 2)) !=
              static_collisions[LEFT][AGENT].end()));
  BOOST_TEST((static_collisions[LEFT][AGENT].find(xy_to_position(2, 3)) !=
              static_collisions[LEFT][AGENT].end()));

  BOOST_TEST((static_collisions[UP][AGENT].find(xy_to_position(1, 2)) !=
              static_collisions[UP][AGENT].end()));
  BOOST_TEST((static_collisions[UP][AGENT].find(xy_to_position(2, 1)) !=
              static_collisions[UP][AGENT].end()));
  BOOST_TEST((static_collisions[UP][AGENT].find(xy_to_position(3, 1)) !=
              static_collisions[UP][AGENT].end()));

  BOOST_TEST((static_collisions[RIGHT][AGENT].find(xy_to_position(3, 1)) !=
              static_collisions[RIGHT][AGENT].end()));
  BOOST_TEST((static_collisions[RIGHT][AGENT].find(xy_to_position(3, 2)) !=
              static_collisions[RIGHT][AGENT].end()));
  BOOST_TEST((static_collisions[RIGHT][AGENT].find(xy_to_position(3, 3)) !=
              static_collisions[RIGHT][AGENT].end()));

  BOOST_TEST((static_collisions[DOWN][AGENT].find(xy_to_position(1, 2)) !=
              static_collisions[DOWN][AGENT].end()));
  BOOST_TEST((static_collisions[DOWN][AGENT].find(xy_to_position(2, 3)) !=
              static_collisions[DOWN][AGENT].end()));
  BOOST_TEST((static_collisions[DOWN][AGENT].find(xy_to_position(3, 3)) !=
              static_collisions[DOWN][AGENT].end()));

  BOOST_TEST(dynamic_collisions[LEFT][0][1].size() == 1);
  BOOST_TEST(dynamic_collisions[RIGHT][0][1].size() == 1);
  BOOST_TEST(dynamic_collisions[UP][0][1].size() == 1);
  BOOST_TEST(dynamic_collisions[DOWN][0][1].size() == 1);

  BOOST_TEST((dynamic_collisions[LEFT][0][1].find(xy_to_position(1, 0)) !=
              dynamic_collisions[LEFT][0][1].end()));
  BOOST_TEST((dynamic_collisions[RIGHT][0][1].find(xy_to_position(-1, 0)) !=
              dynamic_collisions[RIGHT][0][1].end()));
  BOOST_TEST((dynamic_collisions[UP][0][1].find(xy_to_position(0, 1)) !=
              dynamic_collisions[UP][0][1].end()));
  BOOST_TEST((dynamic_collisions[DOWN][0][1].find(xy_to_position(0, -1)) !=
              dynamic_collisions[DOWN][0][1].end()));

  // Verify the solution to the puzzle.
  State state(puzzle.getInitialState());
  BOOST_TEST(!puzzle.satisfiesGoal(state));

  state = puzzle.getNextState(state, LEFT);      // Push into a wall. No change.
  BOOST_TEST(state[0] == xy_to_position(1, 2));  // S
  BOOST_TEST(state[1] == xy_to_position(2, 2));  // M0
  BOOST_TEST(!puzzle.satisfiesGoal(state));

  state = puzzle.getNextState(state, UP);        // Push into a wall. No change.
  BOOST_TEST(state[0] == xy_to_position(1, 2));  // S
  BOOST_TEST(state[1] == xy_to_position(2, 2));  // M0
  BOOST_TEST(!puzzle.satisfiesGoal(state));

  state =
      puzzle.getNextState(state, DOWN);  // Push into a agent wall. No change.
  BOOST_TEST(state[0] == xy_to_position(1, 2));  // S
  BOOST_TEST(state[1] == xy_to_position(2, 2));  // M0
  BOOST_TEST(!puzzle.satisfiesGoal(state));

  state = puzzle.getNextState(state, RIGHT);
  BOOST_TEST(state[0] == xy_to_position(2, 2));  // S
  BOOST_TEST(state[1] == xy_to_position(3, 2));  // M0
  BOOST_TEST(!puzzle.satisfiesGoal(state));

  state = puzzle.getNextState(state, RIGHT);  // Transitive stopping. No change.
  BOOST_TEST(state[0] == xy_to_position(2, 2));  // S
  BOOST_TEST(state[1] == xy_to_position(3, 2));  // M0
  BOOST_TEST(!puzzle.satisfiesGoal(state));

  state = puzzle.getNextState(state, DOWN);
  BOOST_TEST(state[0] == xy_to_position(2, 3));  // S
  BOOST_TEST(state[1] == xy_to_position(3, 2));  // M0
  BOOST_TEST(!puzzle.satisfiesGoal(state));

  state = puzzle.getNextState(state, DOWN);      // Push into a wall. No change.
  BOOST_TEST(state[0] == xy_to_position(2, 3));  // S
  BOOST_TEST(state[1] == xy_to_position(3, 2));  // M0
  BOOST_TEST(!puzzle.satisfiesGoal(state));

  state = puzzle.getNextState(state, RIGHT);
  BOOST_TEST(state[0] == xy_to_position(3, 3));  // S
  BOOST_TEST(state[1] == xy_to_position(3, 2));  // M0
  BOOST_TEST(!puzzle.satisfiesGoal(state));

  state = puzzle.getNextState(state, RIGHT);     // Push into a wall. No change.
  BOOST_TEST(state[0] == xy_to_position(3, 3));  // S
  BOOST_TEST(state[1] == xy_to_position(3, 2));  // M0
  BOOST_TEST(!puzzle.satisfiesGoal(state));

  state = puzzle.getNextState(state, UP);
  BOOST_TEST(state[0] == xy_to_position(3, 2));  // S
  BOOST_TEST(state[1] == xy_to_position(3, 1));  // M0
  BOOST_TEST(puzzle.satisfiesGoal(state));

  state = puzzle.getNextState(state, UP);  // Transitive stopping. No change.
  BOOST_TEST(state[0] == xy_to_position(3, 2));  // S
  BOOST_TEST(state[1] == xy_to_position(3, 1));  // M0
  BOOST_TEST(puzzle.satisfiesGoal(state));

  BOOST_TEST(puzzle.isValidPlan({RIGHT, DOWN, RIGHT, UP}));
  BOOST_TEST(puzzle.isValidPlan({RIGHT, DOWN, RIGHT, DOWN, RIGHT, UP}));
  BOOST_TEST(!puzzle.isValidPlan({RIGHT, DOWN, LEFT, UP}));
}

/**
 * Checks a PushWorld puzzle file can be parsed when it contains entities with
 * overlapping pixels. E.g., goals and objects can overlap.
 */
BOOST_AUTO_TEST_CASE(test_trivial_overlap_parsing) {
  std::string filename = "puzzles/trivial_overlap.pwp";
  PushWorldPuzzle puzzle(filename);

  const auto& goal = puzzle.getGoal();
  BOOST_TEST_REQUIRE(goal.size() == 1);
  BOOST_TEST(goal[0] == xy_to_position(2, 1));

  const auto& initial_state = puzzle.getInitialState();
  BOOST_TEST_REQUIRE(initial_state.size() == 3);
  BOOST_TEST(initial_state[0] == xy_to_position(2, 1));  // S
  BOOST_TEST(initial_state[1] == xy_to_position(1, 1));  // M0
  BOOST_TEST(initial_state[2] == xy_to_position(2, 2));  // M1

  // Verify the collisions
  const auto& object_collisions = puzzle.getObjectCollisions();
  const auto& static_collisions = object_collisions.static_collisions;
  const auto& dynamic_collisions = object_collisions.dynamic_collisions;

  BOOST_TEST(static_collisions[LEFT][AGENT].size() == 2);
  BOOST_TEST(static_collisions[RIGHT][AGENT].size() == 2);
  BOOST_TEST(static_collisions[UP][AGENT].size() == 2);
  BOOST_TEST(static_collisions[DOWN][AGENT].size() == 2);

  BOOST_TEST((static_collisions[LEFT][AGENT].find(xy_to_position(1, 1)) !=
              static_collisions[LEFT][AGENT].end()));
  BOOST_TEST((static_collisions[LEFT][AGENT].find(xy_to_position(1, 2)) !=
              static_collisions[LEFT][AGENT].end()));

  BOOST_TEST((static_collisions[UP][AGENT].find(xy_to_position(1, 1)) !=
              static_collisions[UP][AGENT].end()));
  BOOST_TEST((static_collisions[UP][AGENT].find(xy_to_position(2, 1)) !=
              static_collisions[UP][AGENT].end()));

  BOOST_TEST((static_collisions[RIGHT][AGENT].find(xy_to_position(2, 1)) !=
              static_collisions[RIGHT][AGENT].end()));
  BOOST_TEST((static_collisions[RIGHT][AGENT].find(xy_to_position(1, 2)) !=
              static_collisions[RIGHT][AGENT].end()));

  BOOST_TEST((static_collisions[DOWN][AGENT].find(xy_to_position(2, 1)) !=
              static_collisions[DOWN][AGENT].end()));
  BOOST_TEST((static_collisions[DOWN][AGENT].find(xy_to_position(1, 2)) !=
              static_collisions[DOWN][AGENT].end()));

  BOOST_TEST(dynamic_collisions[LEFT][0][1].size() == 2);
  BOOST_TEST(dynamic_collisions[RIGHT][0][1].size() == 2);
  BOOST_TEST(dynamic_collisions[UP][0][1].size() == 1);
  BOOST_TEST(dynamic_collisions[DOWN][0][1].size() == 1);

  BOOST_TEST(dynamic_collisions[LEFT][0][2].size() == 1);
  BOOST_TEST(dynamic_collisions[RIGHT][0][2].size() == 1);
  BOOST_TEST(dynamic_collisions[UP][0][2].size() == 1);
  BOOST_TEST(dynamic_collisions[DOWN][0][2].size() == 1);

  BOOST_TEST(dynamic_collisions[LEFT][2][1].size() == 2);
  BOOST_TEST(dynamic_collisions[RIGHT][2][1].size() == 2);
  BOOST_TEST(dynamic_collisions[UP][2][1].size() == 1);
  BOOST_TEST(dynamic_collisions[DOWN][2][1].size() == 1);
}

/* Checks a PushWorld puzzle file that includes multi-pixel objects. */
BOOST_AUTO_TEST_CASE(test_file_parsing) {
  std::string filename = "puzzles/file_parsing.pwp";
  PushWorldPuzzle puzzle(filename);

  const auto& goal = puzzle.getGoal();
  BOOST_TEST_REQUIRE(goal.size() == 2);
  BOOST_TEST(goal[0] == xy_to_position(3, 4));
  BOOST_TEST(goal[1] == xy_to_position(6, 5));

  const auto& initial_state = puzzle.getInitialState();
  BOOST_TEST_REQUIRE(initial_state.size() == 6);
  BOOST_TEST(initial_state[0] == xy_to_position(1, 12));  // S
  BOOST_TEST(initial_state[1] == xy_to_position(1, 3));   // M1
  BOOST_TEST(initial_state[2] == xy_to_position(6, 14));  // M4
  BOOST_TEST(initial_state[3] == xy_to_position(4, 1));   // M0
  BOOST_TEST(initial_state[4] == xy_to_position(2, 7));   // M2
  BOOST_TEST(initial_state[5] == xy_to_position(3, 8));   // M3

  const auto& object_collisions = puzzle.getObjectCollisions();
  const auto& static_collisions = object_collisions.static_collisions;
  const auto& dynamic_collisions = object_collisions.dynamic_collisions;

  BOOST_TEST(static_collisions[LEFT][0].size() == 16);  // S
  BOOST_TEST(static_collisions[LEFT][1].size() == 16);  // M1
  BOOST_TEST(static_collisions[LEFT][2].size() == 15);  // M4
  BOOST_TEST(static_collisions[LEFT][3].size() == 15);  // M0
  BOOST_TEST(static_collisions[LEFT][4].size() == 14);  // M2
  BOOST_TEST(static_collisions[LEFT][5].size() == 16);  // M3

  BOOST_TEST(static_collisions[RIGHT][0].size() == 16);  // S
  BOOST_TEST(static_collisions[RIGHT][1].size() == 16);  // M1
  BOOST_TEST(static_collisions[RIGHT][2].size() == 15);  // M4
  BOOST_TEST(static_collisions[RIGHT][3].size() == 15);  // M0
  BOOST_TEST(static_collisions[RIGHT][4].size() == 14);  // M2
  BOOST_TEST(static_collisions[RIGHT][5].size() == 16);  // M3

  BOOST_TEST(static_collisions[UP][0].size() == 9);   // S
  BOOST_TEST(static_collisions[UP][1].size() == 10);  // M1
  BOOST_TEST(static_collisions[UP][2].size() == 9);   // M4
  BOOST_TEST(static_collisions[UP][3].size() == 9);   // M0
  BOOST_TEST(static_collisions[UP][4].size() == 8);   // M2
  BOOST_TEST(static_collisions[UP][5].size() == 10);  // M3

  BOOST_TEST(dynamic_collisions[DOWN][0][4].size() == 5);   // S, M4
  BOOST_TEST(dynamic_collisions[DOWN][0][3].size() == 4);   // S, M3
  BOOST_TEST(dynamic_collisions[DOWN][1][2].size() == 2);   // M1, M4
  BOOST_TEST(dynamic_collisions[DOWN][1][4].size() == 4);   // M1, M2
  BOOST_TEST(dynamic_collisions[LEFT][1][2].size() == 2);   // M1, M4
  BOOST_TEST(dynamic_collisions[LEFT][1][4].size() == 4);   // M1, M2
  BOOST_TEST(dynamic_collisions[RIGHT][1][2].size() == 2);  // M1, M4
  BOOST_TEST(dynamic_collisions[RIGHT][1][4].size() == 4);  // M1, M2
  BOOST_TEST(dynamic_collisions[UP][1][2].size() == 2);     // M1, M4
  BOOST_TEST(dynamic_collisions[UP][1][4].size() == 4);     // M1, M2
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace pushworld
