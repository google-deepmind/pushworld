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

#ifndef HEURISTICS_NOVELTY_H_
#define HEURISTICS_NOVELTY_H_

#include <unordered_set>
#include <utility>  // pair
#include <vector>

#include "heuristics/heuristic.h"
#include "pushworld_puzzle.h"

namespace pushworld {
namespace heuristic {

using PositionPair = std::pair<Position2D, Position2D>;

struct PositionPairHash {
  size_t operator()(const PositionPair& pair) const {
    size_t seed = 0;
    boost::hash_combine(seed, pair.first);
    boost::hash_combine(seed, pair.second);
    return seed;
  }
};

/**
 * Implements the novelty heuristic for width-based search as described in:
 *
 *     Lipovetzky, Nir, and Hector Geffner. "Best-first width search:
 * Exploration and exploitation in classical planning." Thirty-First AAAI
 * Conference on Artificial Intelligence. 2017.
 */
class NoveltyHeuristic : public Heuristic<float> {
 private:
  int m_state_size;
  std::vector<std::unordered_set<Position2D>> m_visited_positions;
  std::vector<std::vector<std::unordered_set<PositionPair, PositionPairHash>>>
      m_visited_position_pairs;

 public:
  /**
   * Constructs a heuristic for PushWorld `State` instances that contain the
   * positions of `state_size` objects.
   */
  NoveltyHeuristic(const int state_size);

  /**
   * Measures the novelty of the given `state` by comparing it to previous
   * states provided to this method.
   *
   * Returns:
   *     0: if at least one object is in a position that has not occurred in any
   *        previous state.
   *     1: if at least one pair of objects are in a combination of positions
   * that have not occurred in any previous state. 2: otherwise.
   *
   * Note:
   *     For computational efficiency, the given `state` is not validated to
   *     contain `state_size` elements.
   */
  float estimate_cost_to_goal(const State& state) override;
};

}  // namespace heuristic
}  // namespace pushworld

#endif /* HEURISTICS_NOVELTY_H_ */
