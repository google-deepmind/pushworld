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

#include "heuristics/novelty.h"

namespace pushworld {
namespace heuristic {

NoveltyHeuristic::NoveltyHeuristic(const int state_size)
    : m_state_size(state_size) {
  m_visited_positions.resize(state_size);
  m_visited_position_pairs.resize(state_size);

  for (auto& position_pairs : m_visited_position_pairs) {
    position_pairs.resize(state_size);
  }
}

float NoveltyHeuristic::estimate_cost_to_goal(
    const RelativeState& relative_state) {
  int j;
  float novelty = 3.0f;

  // This loop computes the novelty and updates the set of visited positions.
  // The novelty is 1 if any object is in a position that has never occurred in
  // any state previously provided to this method.
  // The novelty is 2 if any pair of objects are in a combination of positions
  // that has never occurred in any state previously provided to this method.
  for (const int i : relative_state.moved_object_indices) {
    const auto& p_i = relative_state.state[i];

    if (m_visited_positions[i].insert(p_i).second) {
      novelty = 1.0f;
    }

    for (j = 0; j < i; j++) {
      const auto& p_j = relative_state.state[j];

      // Order with smaller indices first. This reduces memory usage by half
      // compared to storing both {p_i, p_j} and {p_j, p_i} in the visited set.
      PositionPair p{p_j, p_i};
      if (m_visited_position_pairs[j][i].insert(std::move(p)).second) {
        if (novelty > 2.0f) {
          novelty = 2.0f;
        }
      }
    }

    j++;  // skip i==j

    for (; j < m_state_size; j++) {
      const auto& p_j = relative_state.state[j];

      // Order with smaller indices first. This reduces memory usage by half
      // compared to storing both {p_i, p_j} and {p_j, p_i} in the visited set.
      PositionPair p{p_i, p_j};
      if (m_visited_position_pairs[i][j].insert(std::move(p)).second) {
        if (novelty > 2.0f) {
          novelty = 2.0f;
        }
      }
    }
  }

  return novelty;
}

}  // namespace heuristic
}  // namespace pushworld
