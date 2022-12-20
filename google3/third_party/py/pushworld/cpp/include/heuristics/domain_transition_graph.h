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

#ifndef HEURISTICS_DOMAIN_TRANSITION_GRAPH_H_
#define HEURISTICS_DOMAIN_TRANSITION_GRAPH_H_

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "pushworld_puzzle.h"

namespace pushworld {
namespace heuristic {

/**
 * A graph that stores whether an object can potentially move from one position
 * to one or more adjacent positions. Any movement that is not in this graph is
 * proven to be unachievable, but note that movements in this graph are *not*
 * proven to be achievable.
 *
 * Has the form: { start position --> set{end position} }
 *
 * This graph is equivalent to a domain transition graph (DTG) from the Fast
 * Downward planner except that this graph does not store the conditions for
 * each object movement, which can be memory-intensive in PushWorld puzzles that
 * involve objects with large surface areas.
 */
using FeasibleMovementGraph =
    std::unordered_map<Position2D, std::unordered_set<Position2D>>;

/**
 * Returns a map from object IDs to their corresponding feasible movement
 * graphs.
 */
std::unordered_map<int, std::shared_ptr<FeasibleMovementGraph>>
build_feasible_movement_graphs(const PushWorldPuzzle& puzzle);

/**
 * Computes the number of movements on the shortest path from a single starting
 * position to any other position in a `FeasibleMovementGraph`.
 *
 * This class performs an incremental breadth-first expansion of positions that
 * are reachable from the start position.
 */
class SingleSourcePathDistances {
 private:
  std::shared_ptr<FeasibleMovementGraph> m_graph;
  float m_frontier_depth;
  std::unique_ptr<std::vector<Position2D>> m_frontier;
  std::unordered_map<Position2D, float> m_distances;

 public:
  SingleSourcePathDistances(const std::shared_ptr<FeasibleMovementGraph>& graph,
                            const Position2D start);

  /**
   * Returns the number of movements on the shortest path from the `start`
   * position (provided to the constructor) to the `target` position.
   */
  float getDistance(const Position2D target);
};

/**
 * Computes the number of movements on the shortest path between any pair of
 * positions in a `FeasibleMovementGraph`.
 */
class PathDistances {
 private:
  mutable std::unordered_map<Position2D,
                             std::unique_ptr<SingleSourcePathDistances>>
      m_distances;

 public:
  PathDistances(const std::shared_ptr<FeasibleMovementGraph>& graph);

  /**
   * Returns the number of movements on the shortest path from the `start`
   * position to the `target` position.
   */
  float getDistance(const Position2D start, const Position2D target) const;
};

}  // namespace heuristic
}  // namespace pushworld

#endif /* HEURISTICS_DOMAIN_TRANSITION_GRAPH_H_ */
