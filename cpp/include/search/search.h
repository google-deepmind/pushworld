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

#ifndef SEARCH_SEARCH_H_
#define SEARCH_SEARCH_H_

#include <memory>

#include "pushworld_puzzle.h"

namespace pushworld {
namespace search {

/**
 * Stores a node in a search tree in which each node corresponds to a puzzle
 * state. Also stores a reference to the parent node that resulted in this
 * node's state.
 *
 * Note: To conserve memory, the action that transitions the `parent->state` to
 * this node's `state` is not stored here.
 */
struct SearchNode {
  // If this is a root node, the parent is `nullptr`.
  const std::shared_ptr<SearchNode> parent;
  const State state;

  // This constructor is required to support `make_shared<SearchNode>(parent,
  // state)`.
  SearchNode(const std::shared_ptr<SearchNode>& parent_, const State& state_)
      : parent(parent_), state(state_){};
};

/**
 * Returns the sequence of actions (i.e. the `Plan`) that advances the puzzle
 * state from from the root ancestor of the `end_node` to the `end_node`.
 */
Plan backtrackPlan(const PushWorldPuzzle& puzzle,
                   const std::shared_ptr<SearchNode>& end_node);

}  // namespace search
}  // namespace pushworld

#endif /* SEARCH_SEARCH_H_ */
