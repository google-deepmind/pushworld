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

#ifndef HEURISTICS_RECURSIVE_GRAPH_DISTANCE_H_
#define HEURISTICS_RECURSIVE_GRAPH_DISTANCE_H_

#include <boost/functional/hash.hpp>
#include <memory>
#include <unordered_map>

#include "heuristics/domain_transition_graph.h"
#include "heuristics/heuristic.h"
#include "pushworld_puzzle.h"

namespace pushworld {
namespace heuristic {

/**
 * Stores all arguments to `RecursiveGraphDistanceHeuristic::get_pushing_costs`
 * to assist with memoization.
 */
struct PushingCostCacheKey {
  int pusher_id;
  Position2D pusher_position;
  int pushee_id;
  Position2D pushee_start_position;
  Position2D pushee_end_position;

  bool operator==(const PushingCostCacheKey& other) const {
    return (pusher_id == other.pusher_id and
            pusher_position == other.pusher_position and
            pushee_id == other.pushee_id and
            pushee_start_position == other.pushee_start_position and
            pushee_end_position == other.pushee_end_position);
  };
};

/* A hash function for `PushingCostCacheKey`. */
struct PushingCostCacheKeyHash {
  std::size_t operator()(const PushingCostCacheKey& t) const {
    std::size_t seed = 0;
    boost::hash_combine(seed, t.pusher_id);
    boost::hash_combine(seed, t.pusher_position);
    boost::hash_combine(seed, t.pushee_id);
    boost::hash_combine(seed, t.pushee_start_position);
    boost::hash_combine(seed, t.pushee_end_position);
    return seed;
  }
};

/**
 * This Recursive Graph Distance (RGD) heuristic is based on the Fast Downward
 * (FD) heuristic, but with modifications to improve both its speed and the
 * accuracy of estimated costs in the PushWorld domain:
 *
 *   - FD estimates the cost of achieving every condition on every transition in
 * a path in a domain transition graph. In PushWorld this can result in greatly
 *     overestimated costs to push an object to a desired position. To reduce
 * this overestimation and improve computational efficiency, RGD only estimates
 * the costs of the conditions for the first transition in the path, and all
 * other transitions in the path are assumed to have a cost of 1, regardless of
 * their conditions.
 *   - The cost of simultaneously pushing several objects at once (e.g. if
 * multiple objects are in a chain of contact) is computed more accurately than
 * in STRIPS representations of the PushWorld domain, since STRIPS cannot model
 * actions that simultaneously move different numbers of objects depending on
 * whether they are in contact.
 */
class RecursiveGraphDistanceHeuristic : public Heuristic<float> {
 private:
  bool m_fewest_tools;
  std::shared_ptr<PushWorldPuzzle> m_puzzle;
  std::unordered_map<int, std::shared_ptr<FeasibleMovementGraph>>
      m_movement_graphs;
  std::unordered_map<int, PathDistances> m_path_distances;

  // A cache of values returned from `get_pushing_costs`.
  std::unordered_map<PushingCostCacheKey,
                     std::shared_ptr<std::unordered_map<Position2D, float>>,
                     PushingCostCacheKeyHash>
      m_pushing_cost_cache;

  /**
   * Returns the estimated cost to move the object with the given `object_id`
   * from its position in the given `state` to the given `goal_position`,
   * subject to the constraint that the agent can push at most `pushing_depth`
   * other objects to indirectly push the given object.
   */
  float get_goal_cost(const State& state, const int object_id,
                      const Position2D goal_position, const int pushing_depth);

  /**
   * Returns the estimated cost to move the object with the given `object_id`
   * from its position in the given `state` to the given `goal_position`,
   * subject to the constraint that the agent should use as few other objects as
   * possible to indirectly push the given object. I.e., the agent should use
   * the fewest number of "tools" to push the given object to its goal position.
   */
  float get_fewest_tools_goal_cost(const State& state, const int object_id,
                                   const Position2D goal_position);

  /**
   * Returns the estimated cost to move the object with the given `object_id`
   * from its `current_position` to the `effect_position`, which must be
   * adjacent.
   *
   * The positions of all other objects are defined in the given `state`. The
   * provided `current_position` overrides `state[object_id]`.
   *
   * `skipped_object_ids` contains a set of the IDs of all objects that should
   * be ignored when considering how to push the indicated object.
   *
   * This cost calculation is subject to the constraint that the agent can push
   * at most `pushing_depth` other objects to indirectly push the given object.
   *
   * `cost_upper_bound` is a limit on the maximum returned cost. Lower values
   * can make this function return faster.
   */
  float get_recursive_pushing_cost(
      const State& state, const int object_id,
      const Position2D current_position, const Position2D effect_position,
      const std::unordered_set<int>& skipped_object_ids,
      const int pushing_depth, const float cost_upper_bound);

  /**
   * Returns a map from pusher positions, which are constrained to be adjacent
   * to the given `pusher_position`, to the cost of moving the pusher from each
   * adjacent position into a (possibly non-adjacent) position where it can push
   * the "pushee" object from the given start position to the given end
   * position, which must be adjacent.
   *
   * When the pusher's movement from `pusher_position` to an adjacent position
   * directly pushes the pushee from its start position to its end position, the
   * returned cost is zero.
   *
   * This function uses the pusher's `FeasibleMovementGraph` to compute the
   * movement cost, and it searches over all ways that the pusher can make
   * contact with the pushee to determine which has minimum cost.
   */
  std::shared_ptr<std::unordered_map<Position2D, float>> get_pushing_costs(
      const int pusher_id, const Position2D pusher_position,
      const int pushee_id, const Position2D pushee_start_position,
      const Position2D pushee_end_position);

 public:
  /**
   * When `fewest_tools` is false, costs are computed by considering an
   * unbounded number of "tools" to perform a single push, where a tool is any
   * object in between the agent and a target object that allows the agent to
   * push the target object without direct contact. For example, if object X can
   * push object Y, then the agent can push Y by pushing X, using X as a tool.
   *
   * When `fewest_tools` is true, costs are computed using the fewest number of
   * tools that results in a non-infinite cost. For example, if a target object
   * can be pushed using a single tool or using two tools at once, the returned
   * cost will correspond to the single tool, even if this cost is higher than
   * the cost of using two tools.
   *
   * Setting `fewest_tools` to true results in faster cost calculations, since
   * considering all combinations of tools is exponentially expensive in the
   * number of available tools, while setting `fewest_tools` to false results in
   * more accurate estimated costs.
   */
  RecursiveGraphDistanceHeuristic(
      const std::shared_ptr<PushWorldPuzzle>& puzzle,
      const bool fewest_tools = true);

  /**
   * Returns the estimated cost to reach the goal in the puzzle provided to the
   * constructor, starting from the given `relative_state.state`. If the
   * returned cost is infinite, then there is provably no way to reach the goal
   * from the given state.
   */
  float estimate_cost_to_goal(const RelativeState& relative_state) override;
};

}  // namespace heuristic
}  // namespace pushworld

#endif /* HEURISTICS_RECURSIVE_GRAPH_DISTANCE_H_ */
