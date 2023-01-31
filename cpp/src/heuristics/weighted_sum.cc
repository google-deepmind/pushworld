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

#include "heuristics/weighted_sum.h"

#include <memory>
#include <utility>  // pair
#include <vector>

#include "heuristics/heuristic.h"
#include "pushworld_puzzle.h"

namespace pushworld {
namespace heuristic {

WeightedSumHeuristic::WeightedSumHeuristic(
    const HeuristicsAndWeights& heuristics_and_weights)
    : m_heuristics_and_weights(heuristics_and_weights) {
  if (heuristics_and_weights.empty()) {
    throw std::invalid_argument(
        "At least one heuristic must be provided to compute a weighted "
        "sum of costs.");
  }
};

float WeightedSumHeuristic::estimate_cost_to_goal(
    const RelativeState& relative_state) {
  auto it = m_heuristics_and_weights.begin();

  float cost = it->first->estimate_cost_to_goal(relative_state) * it->second;
  while (++it != m_heuristics_and_weights.end()) {
    cost += it->first->estimate_cost_to_goal(relative_state) * it->second;
  }

  return cost;
}

}  // namespace heuristic
}  // namespace pushworld
