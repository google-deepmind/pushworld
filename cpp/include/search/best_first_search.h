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

#ifndef SEARCH_BEST_FIRST_SEARCH_H_
#define SEARCH_BEST_FIRST_SEARCH_H_

#include <memory>
#include <optional>

#include "heuristics/heuristic.h"
#include "pushworld_puzzle.h"
#include "search/priority_queue.h"
#include "search/random_action_iterator.h"
#include "search/search.h"

namespace pushworld {
namespace search {

/**
 * Searches for a solution to the given `puzzle` by prioritizing the exploration
 * of states that the `heuristic` estimates to have the minimum estimated cost
 * to reach the goal. Returns `std::nullopt` if no solution exists.
 *
 * The `frontier` priority queue is used to track which unexplored states have
 * the minimum estimated cost. In some cases, the type of this priority queue
 * may be chosen to optimize for the `Cost` type (e.g. if costs are discrete or
 * continuous). The `frontier` is cleared when the search begins.
 *
 * `visited` stores all states that are encountered during the search. It is
 * cleared when the search begins.
 */
template <typename Cost>
std::optional<Plan> best_first_search(
    const PushWorldPuzzle& puzzle, heuristic::Heuristic<Cost>& heuristic,
    priority_queue::PriorityQueue<std::shared_ptr<SearchNode>, Cost>& frontier,
    StateSet& visited) {
  const auto& initial_state = puzzle.getInitialState();

  if (puzzle.satisfiesGoal(initial_state)) {
    return Plan();  // The plan to reach the goal has no actions.
  }

  RandomActionIterator action_iterator;

  visited.clear();
  visited.insert(initial_state);

  frontier.clear();
  frontier.push(std::make_shared<SearchNode>(nullptr, initial_state),
                heuristic.estimate_cost_to_goal(initial_state));

  while (!frontier.empty()) {
    const auto parent_node = frontier.top();
    frontier.pop();

    for (const auto& action : action_iterator.next()) {
      const auto state = puzzle.getNextState(parent_node->state, action);

      // Ignore the state if it was already visited.
      if (visited.find(state) == visited.end()) {
        const auto node = std::make_shared<SearchNode>(parent_node, state);

        if (puzzle.satisfiesGoal(state)) {
          // Return the first solution found.
          return backtrackPlan(puzzle, node);
        }

        frontier.push(node, heuristic.estimate_cost_to_goal(state));
        visited.insert(state);
      }
    }
  }

  // No solution found
  return std::nullopt;
}

/* Identical to `best_first_search` above, but without the `visited` argument.
 */
template <typename Cost>
std::optional<Plan> best_first_search(
    const PushWorldPuzzle& puzzle, heuristic::Heuristic<Cost>& heuristic,
    priority_queue::PriorityQueue<std::shared_ptr<SearchNode>, Cost>&
        frontier) {
  StateSet visited;
  return best_first_search<Cost>(puzzle, heuristic, frontier, visited);
}

}  // namespace search
}  // namespace pushworld

#endif /* SEARCH_BEST_FIRST_SEARCH_H_ */
