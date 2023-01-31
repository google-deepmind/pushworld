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

#include "search/search.h"

#include <algorithm>  // std::reverse
#include <memory>

#include "pushworld_puzzle.h"

namespace pushworld {
namespace search {

Plan backtrackPlan(const PushWorldPuzzle& puzzle,
                   const std::shared_ptr<SearchNode>& end_node) {
  Plan plan;
  std::shared_ptr<SearchNode> node = end_node;
  Action action;

  while (node->parent != nullptr) {
    // Determine which action produced this state transition. When NUM_ACTIONS
    // is small, it is faster to reconstruct the actions during backtracking
    // rather than store actions with every node during a search.
    for (action = 0; action < NUM_ACTIONS; action++) {
      if (node->state ==
          puzzle.getNextState(node->parent->state, action).state) {
        plan.push_back(action);
        break;
      }
    }

    if (action == NUM_ACTIONS) {
      throw std::invalid_argument(
          "A parent state exists for which no action can transition to "
          "the state of a child search node.");
    }

    node = node->parent;
  }

  std::reverse(plan.begin(), plan.end());
  return plan;
}

}  // namespace search
}  // namespace pushworld
