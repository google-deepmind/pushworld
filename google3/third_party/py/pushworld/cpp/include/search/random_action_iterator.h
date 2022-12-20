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

#ifndef SEARCH_RANDOM_ACTION_ITERATOR_H_
#define SEARCH_RANDOM_ACTION_ITERATOR_H_

#include <vector>

#include "pushworld_puzzle.h"

namespace pushworld {
namespace search {

/**
 * Iterates over vectors that contain all PushWorld actions in randomized
 * orders.
 *
 * Typically used to avoid bias from evaluating some actions before others. For
 * example:
 *
 *     action_iter = RandomActionIterator();
 *     for (const auto action : action_iter.next()) {
 *         const auto next_state = pushworld_puzzle.getNextState(state, action);
 *         ...
 *     }
 */
class RandomActionIterator {
 private:
  std::vector<std::vector<pushworld::Action>> m_action_groups;
  std::vector<std::vector<pushworld::Action>>::iterator m_next_action_group;

 public:
  /**
   * For computational efficiency, a finite number of groups of all `PushWorld`
   * actions are constructed when this iterator is initialized, and the `next`
   * method loops through each of the groups without repeatedly performing
   * random shuffles after initialization.
   */
  RandomActionIterator(const int num_action_groups = 1000);

  /* Returns a vector that contains all `PushWorld` actions in a random order.
   */
  const std::vector<pushworld::Action>& next();
};

}  // namespace search
}  // namespace pushworld

#endif /* SEARCH_RANDOM_ACTION_ITERATOR_H_ */
