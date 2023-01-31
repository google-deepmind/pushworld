/*
 * Copyright 2022 DeepMind Technologies Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef PUSH_WORLD_H_
#define PUSH_WORLD_H_

#include <stdlib.h>

#include <boost/functional/hash.hpp>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace pushworld {

// For computational efficiency, each 2D position is stored as an integer in
// which upper bits contain the X value and lower bits contain the Y value.
using Position2D = int;

// For compatibility with `Position2D`, the value of every X and Y value must
// remain below this limit. The value 10000 is convenient for printing
// `Position2D` values in a readable form.
static const int POSITION_LIMIT = 10000;

// A PushWorld state is a vector of the positions of all objects.
using State = std::vector<Position2D>;

// A combination of a state and a vector of the indicies of objects that have
// changed their positions relative to some other state.
struct RelativeState {
  State state;
  std::vector<int> moved_object_indices;
};

// The first object in the `State` vector always corresponds to the object that
// actions can directly control. This object is called the "agent", and its
// index in the `State` vector is given by `AGENT`.
static const int AGENT = 0;

// A PushWorld goal is a vector of the desired positions of one or more objects.
// The kth element in a `Goal` defines the desired position of the (k+1)th
// element in a `State` (i.e. the agent never has a goal position).
using Goal = std::vector<Position2D>;

// This enumeration defines constants for all available PushWorld actions.
enum ACTIONS { LEFT, RIGHT, UP, DOWN };
static const int NUM_ACTIONS = 4;
using Action = int;

// A plan is a sequence of actions to execute in order, usually to transition
// from an initial state to a state that satisfies a goal.
using Plan = std::vector<Action>;

// A plan can be encoded as a string in which each character indicates an
// action. This array maps each action constant above to its corresponding
// character.
static const char ACTION_TO_CHAR[] = {'L', 'R', 'U', 'D'};

// Defines a hash function for `State` instances.
struct StateHash {
  std::size_t operator()(const State& state) const {
    return boost::hash_range(state.begin(), state.end());
  }
};

using StateSet = std::unordered_set<State, StateHash>;

/**
 * Constructs a `Position2D`.
 *
 * This function can also convert displacements. E.g.:
 *     xy_to_position(x, y) + xy_to_position(dx, dy) == xy_to_position(x + dx, y
 * + dy)
 */
Position2D xy_to_position(const int x, const int y);

/**
 * Converts a `Position2D` into separate X and Y values.
 *
 * Unlike `xy_to_position`, this function assumes that X and Y are always
 * non-negative, so this function cannot convert a signed displacement
 * `xy_to_position(dx, dy)` back into the original (dx, dy).
 */
void position_to_xy(const Position2D p, int& x, int& y);

// A map from action IDs to the position displacements they cause.
static const Position2D ACTION_DISPLACEMENTS[] = {
    // (0,0) is top left
    xy_to_position(-1, 0),  // LEFT
    xy_to_position(1, 0),   // RIGHT
    xy_to_position(0, -1),  // UP
    xy_to_position(0, 1)    // DOWN
};

// The inverse of `ACTION_DISPLACEMENTS`.
static const std::unordered_map<Position2D, Action> DISPLACEMENTS_TO_ACTIONS = {
    {xy_to_position(-1, 0), LEFT},
    {xy_to_position(1, 0), RIGHT},
    {xy_to_position(0, -1), UP},
    {xy_to_position(0, 1), DOWN}};

/**
 * Represents how dynamic (i.e. movable) objects collide with static obstacles
 * (e.g. walls) and with each other.
 */
struct ObjectCollisions {
  // Whenever `static_collisions[action][object_index]` contains position `p` of
  // the object with the corresponding index, moving the object in the direction
  // of the action when it has position `p` results in a collision with a static
  // object.
  std::vector<std::vector<std::unordered_set<Position2D>>> static_collisions;

  // Whenever `dynamic_collisions[action][object1_index][object2_index]`
  // contains the relative position `dp = <position of object 1> - <position of
  // object 2>`, moving object 1 in the direction of the action results in a
  // collision with object 2 when the objects have the relative position `dp`.
  // I.e., object 1 would push object 2.
  std::vector<std::vector<std::vector<std::unordered_set<Position2D>>>>
      dynamic_collisions;

  /**
   * Default constructor. Callers are expected to call `resize` later to
   * allocate memory to store collision information.
   */
  ObjectCollisions(){};

  /**
   * This constructor allocates memory for the given number of objects in the
   * `static_collisions` and `dynamic_collisions` member variables.
   */
  ObjectCollisions(const unsigned int num_objects) { resize(num_objects); };

  /**
   * Resizes `static_collisions` and `dynamic_collisions` for the given number
   * of objects.
   */
  void resize(const unsigned int num_objects) {
    static_collisions.resize(NUM_ACTIONS);
    dynamic_collisions.resize(NUM_ACTIONS);

    for (int a = 0; a < NUM_ACTIONS; a++) {
      static_collisions[a].resize(num_objects);
      dynamic_collisions[a].resize(num_objects);

      for (int m = 0; m < num_objects; m++)
        dynamic_collisions[a][m].resize(num_objects);
    }
  };
};

/**
 * A puzzle in the PushWorld environment.
 */
class PushWorldPuzzle {
 private:
  State m_initial_state;
  int m_num_objects;

  // m_goal[i] contains the target value of m_initial_state[i+1].
  Goal m_goal;

  ObjectCollisions m_object_collisions;

  mutable std::vector<int> m_pushing_frontier;
  mutable std::vector<int> m_pushed_object_idxs;
  mutable std::vector<bool> m_pushed_objects;

  void init();

 public:
  /**
   * Loads a PushWorld puzzle from a file.
   */
  PushWorldPuzzle(const std::string& filename);

  /**
   * Constructs a PushWorld puzzle to achieve a `goal` by performing actions
   * starting from an `initial_state`. Object movements are constrained by
   * `entity_collisions`.
   */
  PushWorldPuzzle(const State& initial_state, const Goal& goal,
                  const ObjectCollisions& entity_collisions);

  /**
   * Returns the initial positions of all objects.
   */
  const State& getInitialState() const { return m_initial_state; }

  /**
   * Returns the goal positions of one or more objects.
   */
  const Goal& getGoal() const { return m_goal; }

  /**
   * Returns a data structure that can efficiently evaluate whether performing
   * an action results in a collision between dynamic objects or with a static
   * object.
   */
  const ObjectCollisions& getObjectCollisions() const {
    return m_object_collisions;
  }

  /**
   * Computes the state that results from performing the `action` in the given
   * `state`. The returned `moved_object_indices` in the relative state contain
   * the indices of all objects whose positions differ from their positions in
   * the given `state`.
   */
  RelativeState getNextState(const State& state, const Action action) const;

  /**
   * Returns whether the given state satisfies the goal of this puzzle.
   */
  bool satisfiesGoal(const State& state) const;

  /**
   * Returns whether performing all actions in the `plan`, starting from the
   * initial state, results in a state that satisfies the goal.
   */
  bool isValidPlan(const Plan& plan) const;
};

}  // namespace pushworld

#endif /* PUSH_WORLD_H_ */
