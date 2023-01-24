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

#include "search/random_action_iterator.h"

#include <algorithm>  // std::shuffle
#include <random>
#include <vector>

#include "pushworld_puzzle.h"

namespace pushworld {
namespace search {

RandomActionIterator::RandomActionIterator(const int num_action_groups) {
  std::default_random_engine random_engine(42);

  m_action_groups.resize(num_action_groups);

  for (auto& action_group : m_action_groups) {
    action_group.resize(pushworld::NUM_ACTIONS);
    for (int action = 0; action < pushworld::NUM_ACTIONS; action++) {
      action_group[action] = action;
    }
    std::shuffle(action_group.begin(), action_group.end(), random_engine);
  }

  m_next_action_group = m_action_groups.begin();
}

const std::vector<pushworld::Action>& RandomActionIterator::next() {
  m_next_action_group++;

  // Cycle through the same groups.
  if (m_next_action_group == m_action_groups.end()) {
    m_next_action_group = m_action_groups.begin();
  }

  return *m_next_action_group;
}

}  // namespace search
}  // namespace pushworld
