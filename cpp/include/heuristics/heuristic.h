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

#ifndef HEURISTICS_HEURISTIC_H_
#define HEURISTICS_HEURISTIC_H_

#include "pushworld_puzzle.h"

namespace pushworld {
namespace heuristic {

/**
 * A generic search heuristic that estimates the remaining cost to reach a goal
 * from a given state.
 *
 * This interface does not constrain how the goal is defined, although the goal
 * is typically provided to the constructor of child classes.
 *
 * This interface also does not constrain the type of a `Cost`. For example, a
 * classical heuristic could return an `int` cost, while a lexicographic
 * heuristic could return `tuple<int, int, int>`.
 */
template <typename Cost>
class Heuristic {
 public:
  /**
   * Notably, this method is not `const`; repeated calls may return different
   * costs. E.g., some heuristics may spend computation to incrementally improve
   * the estimated cost with repeated calls.
   */
  virtual Cost estimate_cost_to_goal(
      const pushworld::RelativeState& relative_state) = 0;
};

}  // namespace heuristic
}  // namespace pushworld

#endif /* HEURISTICS_HEURISTIC_H_ */
