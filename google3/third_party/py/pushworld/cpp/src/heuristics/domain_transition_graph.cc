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

#include "heuristics/domain_transition_graph.h"

#include <boost/functional/hash.hpp>
#include <limits>
#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace pushworld {
namespace heuristic {

namespace {

/**
 * Represents an object's movement from one position to another.
 *
 * Named `Transition` to match the "domain transition graph" terminology in the
 * Fast Downward planner.
 */
struct Transition {
  int object_id;
  Position2D start_position;
  Position2D end_position;

  bool operator==(const Transition& other) const {
    return (object_id == other.object_id and
            start_position == other.start_position and
            end_position == other.end_position);
  };
};

struct TransitionHash {
  std::size_t operator()(const Transition& t) const {
    std::size_t seed = 0;
    boost::hash_combine(seed, t.object_id);
    boost::hash_combine(seed, t.start_position);
    boost::hash_combine(seed, t.end_position);
    return seed;
  }
};

/**
 * A hash function for the search frontier inside
 * `build_feasible_movement_graphs`.
 */
struct FrontierHash {
  std::size_t operator()(const std::pair<int, Position2D>& p) const {
    std::size_t seed = 0;
    boost::hash_combine(seed, p.first);
    boost::hash_combine(seed, p.second);
    return seed;
  }
};

/**
 * A map from transitions to vectors of other transitions that would become
 * feasible if the key transition is proven to be feasible.
 */
using DependentTransitions =
    std::unordered_map<Transition, std::vector<Transition>, TransitionHash>;

/**
 * Adds the transition to its associated feasible movement graph.
 *
 * If the transition is not already in the graph, all dependent transitions are
 * informed that the given `transition` is feasible, and the end position in the
 * `transition` is added to the `frontier`.
 */
void add_transition(
    const Transition& transition,
    std::unordered_set<std::pair<int, Position2D>, FrontierHash>& frontier,
    std::unordered_map<int, std::shared_ptr<FeasibleMovementGraph>>& graphs,
    DependentTransitions& dependent_transitions) {
  const auto& object_graph = graphs[transition.object_id];
  auto& descendants = (*object_graph)[transition.start_position];

  // Check whether this is the first time the transition has been encountered.
  if (descendants.insert(transition.end_position).second) {
    // Notify other transitions that this one has been satisfied.
    for (const auto& dependent : dependent_transitions[transition]) {
      add_transition(dependent, frontier, graphs, dependent_transitions);
    }
    dependent_transitions.erase(transition);

    // If this is the first time a transition has reached the end position,
    // add the end position to the frontier.
    if (object_graph->find(transition.end_position) == object_graph->end()) {
      frontier.insert(
          std::make_pair(transition.object_id, transition.end_position));

      // Create a node at the end position.
      (*object_graph)[transition.end_position];
    }
  }
}

}  // namespace

std::unordered_map<int, std::shared_ptr<FeasibleMovementGraph>>
build_feasible_movement_graphs(const PushWorldPuzzle& puzzle) {
  // A map from object IDs to their movement graphs.
  std::unordered_map<int, std::shared_ptr<FeasibleMovementGraph>> graphs;

  const auto& initial_state = puzzle.getInitialState();
  const auto& collisions = puzzle.getObjectCollisions();
  const int num_objects = initial_state.size();

  std::unordered_set<std::pair<int, Position2D>, FrontierHash> frontier;
  DependentTransitions dependent_transitions;

  for (int i = 0; i < initial_state.size(); i++) {
    const auto position = initial_state[i];

    graphs[i] = std::make_shared<FeasibleMovementGraph>();
    // Populate the initial node in each graph in case there are no outgoing
    // edges.
    (*graphs[i])[position];

    frontier.insert(std::make_pair(i, position));
  }

  // Incrementally expand the set of movements (i.e. transitions) that are
  // potentially reachable from the initial puzzle state.
  while (!frontier.empty()) {
    const auto frontier_iter = frontier.begin();
    int object_id = frontier_iter->first;
    Position2D position = frontier_iter->second;
    frontier.erase(frontier_iter);

    if (object_id == AGENT) {
      for (int action = 0; action < NUM_ACTIONS; action++) {
        const auto& static_collisions =
            collisions.static_collisions.at(action).at(object_id);

        // Add all transitions from this position that do not move into
        // collision with a static object.
        if (static_collisions.find(position) == static_collisions.end()) {
          const Transition transition{AGENT, position,
                                      position + ACTION_DISPLACEMENTS[action]};
          add_transition(transition, frontier, graphs, dependent_transitions);
        }
      }
      continue;
    }

    // Consider all pushing movements from all directions.
    for (int action = 0; action < NUM_ACTIONS; action++) {
      // Omit movements that cause collisions with static objects.
      const auto& static_collisions =
          collisions.static_collisions.at(action).at(object_id);
      if (static_collisions.find(position) != static_collisions.end()) {
        continue;
      }

      const auto& dynamic_collisions = collisions.dynamic_collisions.at(action);
      const auto displacement = ACTION_DISPLACEMENTS[action];
      const Transition transition{object_id, position, position + displacement};

      // Consider all objects that could push this object.
      for (int pusher_id = 0; pusher_id < num_objects; pusher_id++) {
        if (pusher_id == object_id) {
          continue;
        }

        const auto& relative_positions =
            dynamic_collisions.at(pusher_id).at(object_id);
        const auto& pusher_graph = graphs[pusher_id];

        for (const auto relative_position : relative_positions) {
          // Start and end positions of the pusher.
          const Position2D start_position = position + relative_position;
          const Position2D end_position = start_position + displacement;

          // Check whether the pusher's transition is already proven to be
          // feasible.
          const auto& end_positions = pusher_graph->find(start_position);
          if (end_positions == pusher_graph->end() or
              end_positions->second.find(end_position) ==
                  end_positions->second.end()) {
            // Not yet proven to be feasible, so record the pusher's
            // transition as a possible cause of the object's transition.
            const Transition pusher_transition{pusher_id, start_position,
                                               end_position};
            dependent_transitions[pusher_transition].push_back(transition);

          } else {
            // The pushing transition is feasible, so the object transition
            // is also feasible.
            add_transition(transition, frontier, graphs, dependent_transitions);

            // Break from the parent loop over possible pushers.
            pusher_id = num_objects;

            break;
          }
        }
      }
    }
  }

  return graphs;
}

SingleSourcePathDistances::SingleSourcePathDistances(
    const std::shared_ptr<FeasibleMovementGraph>& graph,
    const Position2D start) {
  m_graph = graph;
  m_frontier_depth = 0.0f;
  m_frontier = std::make_unique<std::vector<Position2D>>();
  m_frontier->push_back(start);
  m_distances[start] = 0.0f;
};

float SingleSourcePathDistances::getDistance(const Position2D target) {
  const auto& iterator = m_distances.find(target);
  if (iterator != m_distances.end()) {
    return iterator->second;
  }

  bool target_found = false;
  while (!m_frontier->empty()) {
    // Expand another depth of a breadth-first search.
    m_frontier_depth++;

    std::unique_ptr<std::vector<Position2D>> next_frontier =
        std::make_unique<std::vector<Position2D>>();

    for (const auto& position : *m_frontier) {
      for (const auto& next_position : m_graph->at(position)) {
        if (m_distances.find(next_position) == m_distances.end()) {
          next_frontier->push_back(next_position);
          m_distances[next_position] = m_frontier_depth;

          if (next_position == target) {
            target_found = true;
          }
        }
      }
    }

    m_frontier = std::move(next_frontier);

    if (target_found) {
      return m_frontier_depth;
    }
  }

  // No path exists from the start to the target.
  return std::numeric_limits<float>::infinity();
};

std::shared_ptr<FeasibleMovementGraph> reverseGraph(
    const std::shared_ptr<FeasibleMovementGraph>& graph) {
  auto reversed_graph = std::make_shared<FeasibleMovementGraph>();
  reversed_graph->reserve(graph->size());

  for (const auto& pair : *graph) {
    (*reversed_graph)[pair.first];
    for (const auto& target : pair.second) {
      (*reversed_graph)[target].insert(pair.first);
    }
  }
  return reversed_graph;
}

PathDistances::PathDistances(
    const std::shared_ptr<FeasibleMovementGraph>& graph) {
  auto reversed_graph = reverseGraph(graph);

  for (const auto& pair : *reversed_graph) {
    m_distances[pair.first] =
        std::make_unique<SingleSourcePathDistances>(reversed_graph, pair.first);
  }
};

float PathDistances::getDistance(const Position2D source,
                                 const Position2D target) const {
  const auto& iterator = m_distances.find(target);

  // Return infinity if no path exists.
  if (iterator == m_distances.end()) {
    return std::numeric_limits<float>::infinity();
  }

  return iterator->second->getDistance(source);
};

}  // namespace heuristic
}  // namespace pushworld
