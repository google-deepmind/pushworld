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

#include "heuristics/recursive_graph_distance.h"

#include <assert.h>

#include <limits>
#include <memory>
#include <unordered_map>
#include <unordered_set>

#include "heuristics/domain_transition_graph.h"
#include "pushworld_puzzle.h"

namespace pushworld {
namespace heuristic {

RecursiveGraphDistanceHeuristic::RecursiveGraphDistanceHeuristic(
    const std::shared_ptr<PushWorldPuzzle>& puzzle, const bool fewest_tools)
    : m_puzzle(puzzle),
      m_movement_graphs(build_feasible_movement_graphs(*puzzle)),
      m_fewest_tools(fewest_tools) {
  for (const auto& pair : m_movement_graphs) {
    m_path_distances.emplace(pair.first, PathDistances(pair.second));
  }

  // Validate an assumption inside `get_recursive_pushing_cost`.
  assert(AGENT == 0);
};

float RecursiveGraphDistanceHeuristic::estimate_cost_to_goal(
    const State& state) {
  float cost = 0.0f;
  const auto& goal = m_puzzle->getGoal();

  for (int object_id = 0; object_id < goal.size();) {
    const auto goal_position = goal[object_id++];

    if (m_fewest_tools) {
      cost += get_fewest_tools_goal_cost(state, object_id, goal_position);
    } else {
      cost += get_goal_cost(state, object_id, goal_position, state.size() - 2);
    }

    // Minor optimization to break early when the cost is infinite.
    if (cost == std::numeric_limits<float>::infinity()) {
      break;
    }
  }

  return cost;
};

float RecursiveGraphDistanceHeuristic::get_goal_cost(
    const State& state, const int object_id, const Position2D goal_position,
    const int pushing_depth) {
  int current_position = state[object_id];

  if (goal_position == current_position) {
    return 0.0f;
  }

  float min_cost = std::numeric_limits<float>::infinity();
  const std::unordered_set<int> skipped_object_ids;  // empty set

  // Consider each feasible movement of the object.
  for (const auto& effect_position :
       m_movement_graphs.at(object_id)->at(current_position)) {
    // Get the distance from the effect position to the goal position.
    const float goal_distance_cost = m_path_distances.at(object_id).getDistance(
        effect_position, goal_position);

    if (goal_distance_cost >= min_cost) {
      continue;
    }

    min_cost = goal_distance_cost +
               get_recursive_pushing_cost(state, object_id, current_position,
                                          effect_position, skipped_object_ids,
                                          pushing_depth,
                                          min_cost - goal_distance_cost);
  }

  return min_cost;
}

float RecursiveGraphDistanceHeuristic::get_fewest_tools_goal_cost(
    const State& state, const int object_id, const Position2D goal_position) {
  for (int pushing_depth = 0; pushing_depth < state.size() - 1;
       pushing_depth++) {
    const auto cost =
        get_goal_cost(state, object_id, goal_position, pushing_depth);
    if (cost != std::numeric_limits<float>::infinity()) {
      return cost;
    }
  }
  return std::numeric_limits<float>::infinity();
}

float RecursiveGraphDistanceHeuristic::get_recursive_pushing_cost(
    const State& state, const int object_id, const Position2D current_position,
    const Position2D effect_position,
    const std::unordered_set<int>& skipped_object_ids, const int pushing_depth,
    const float cost_upper_bound) {
  float min_cost = cost_upper_bound;

  std::unordered_set<int> next_skipped_object_ids(skipped_object_ids);
  next_skipped_object_ids.insert(object_id);

  int start_pusher_id;
  int end_pusher_id;

  // These indices assume that the agent's ID is 0, which is validated in the
  // class constructor.
  if (pushing_depth == 0) {
    start_pusher_id = 0;
    end_pusher_id = 1;
  } else {
    start_pusher_id = 1;
    end_pusher_id = state.size();
  }

  for (int pusher_id = start_pusher_id; pusher_id < end_pusher_id;
       pusher_id++) {
    if (next_skipped_object_ids.find(pusher_id) !=
        next_skipped_object_ids.end()) {
      continue;
    }

    const Position2D pusher_position = state[pusher_id];
    const auto pushing_costs =
        get_pushing_costs(pusher_id, pusher_position, object_id,
                          current_position, effect_position);

    for (const auto pair : *pushing_costs) {
      const float pusher_distance_cost = pair.second;
      if (pusher_distance_cost >= min_cost) {
        continue;
      }

      if (pusher_id == AGENT) {
        // The agent can directly push the object by moving to an adjacent
        // position, which costs `1`.
        const auto total_cost = pusher_distance_cost + 1.0f;
        if (total_cost < min_cost) {
          min_cost = total_cost;
        }
      } else {
        min_cost = pusher_distance_cost +
                   get_recursive_pushing_cost(
                       state, pusher_id, pusher_position,
                       pair.first,  // pusher_effect_position
                       next_skipped_object_ids, pushing_depth - 1,
                       min_cost - pusher_distance_cost);
      }
    }
  }

  return min_cost;
}

std::shared_ptr<std::unordered_map<Position2D, float>>
RecursiveGraphDistanceHeuristic::get_pushing_costs(
    const int pusher_id, const Position2D pusher_position, const int pushee_id,
    const Position2D pushee_start_position,
    const Position2D pushee_end_position) {
  PushingCostCacheKey args{pusher_id, pusher_position, pushee_id,
                           pushee_start_position, pushee_end_position};

  const auto cached_costs = m_pushing_cost_cache.find(args);
  if (cached_costs != m_pushing_cost_cache.end()) {
    return cached_costs->second;
  }

  auto costs = std::make_shared<std::unordered_map<Position2D, float>>();

  const float displacement = pushee_end_position - pushee_start_position;
  const Action action = DISPLACEMENTS_TO_ACTIONS.at(displacement);
  const auto& collisions = m_puzzle->getObjectCollisions();

  const auto& pusher_graph = m_movement_graphs.at(pusher_id);
  const auto& pusher_next_positions = pusher_graph->at(pusher_position);
  const auto& relative_positions =
      collisions.dynamic_collisions.at(action).at(pusher_id).at(pushee_id);

  // Consider every relative position from which the pusher can push the pushee
  // to its end position.
  for (const auto relative_position : relative_positions) {
    const Position2D pushing_start_position =
        pushee_start_position + relative_position;
    const Position2D pushing_end_position =
        pushing_start_position + displacement;

    const auto& pushing_movements = pusher_graph->find(pushing_start_position);

    // Check that the pusher does not collide with a static obstacle while
    // performing the pushing movement.
    if (pushing_movements != pusher_graph->end() and
        pushing_movements->second.find(pushing_end_position) !=
            pushing_movements->second.end()) {
      /*
      For all pusher positions that are adjacent to the pusher's current
      position, compute the graph distance from each adjacent position to the
      position where the pusher makes contact with the pushee.
      */
      for (const auto pusher_next_position : pusher_next_positions) {
        float distance_cost;

        if (pushing_start_position == pusher_position and
            pushing_end_position == pusher_next_position) {
          // This is a simultaneous push, so there is no cost.
          distance_cost = 0;
        } else {
          distance_cost = m_path_distances.at(pusher_id).getDistance(
              pusher_next_position, pushing_start_position);

          if (distance_cost == std::numeric_limits<float>::infinity()) {
            continue;
          }

          // Add 1 for the cost of the transition.
          distance_cost += 1;
        }

        const auto best_cost_pair = costs->find(pusher_next_position);
        if (best_cost_pair == costs->end()) {
          (*costs)[pusher_next_position] = distance_cost;
        } else if (distance_cost < best_cost_pair->second) {
          best_cost_pair->second = distance_cost;
        }
      }
    }
  }

  // Cache the result.
  m_pushing_cost_cache[args] = costs;
  return costs;
}

}  // namespace heuristic
}  // namespace pushworld
