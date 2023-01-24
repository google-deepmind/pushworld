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

float NoveltyHeuristic::estimate_cost_to_goal(const State& state) {
  int i, j;
  float novelty = 2.0f;

  // The novelty is 1 if any pair of objects are in a combination of positions
  // that has never occurred in any state previously provided to this method.
  // While checking whether this state contains a novel pair of object
  // positions, this loop simultaneously updates the set of visited pairs of
  // positions.
  for (i = 0; i < m_state_size; i++) {
    const auto& p_i = state[i];

    for (j = i + 1; j < m_state_size; j++) {
      const auto& p_j = state[j];
      const PositionPair p{p_i, p_j};

      if (m_visited_position_pairs[i][j].insert(p).second) {
        novelty = 1.0f;
        j++;
        break;
      }
    }
    if (novelty == 1.0f) break;
  }

  // Finish updating all visited pairs if the loop above exited early.
  while (i < m_state_size) {
    const auto& p_i = state[i];
    for (; j < m_state_size; j++) {
      const auto& p_j = state[j];
      m_visited_position_pairs[i][j].insert(PositionPair{p_i, p_j});
    }
    i++;
    j = i + 1;
  }

  // The novelty is 0 if any object is in a position that has never occurred in
  // any state previously provided to this method.
  // While checking whether this state contains a novel object position, this
  // loop simultaneously updates the set of visited object positions.
  for (i = 0; i < m_state_size; i++) {
    if (m_visited_positions[i].insert(state[i]).second) {
      novelty = 0.0f;
      i++;
      break;
    }
  }

  // Finish updating all visited positions if the loop above exited early.
  for (; i < m_state_size; i++) {
    m_visited_positions[i].insert(state[i]);
  }

  return novelty;
}

}  // namespace heuristic
}  // namespace pushworld
