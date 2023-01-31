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

#include "pushworld_puzzle.h"

#include <algorithm>  // min, max
#include <boost/algorithm/string.hpp>
#include <fstream>
#include <map>
#include <string>
#include <unordered_set>
#include <vector>

namespace pushworld {

// A private namespace to not expose this code
namespace {

/**
 * This struct assists with processing object pixels after loading them from a
 * PushWorld puzzle file.
 */
struct Point {
  int x, y;

  bool operator==(const Point& other) const {
    return this->x == other.x && this->y == other.y;
  }
  Point operator+(const Point& other) const {
    return Point{this->x + other.x, this->y + other.y};
  }
  Point operator-(const Point& other) const {
    return Point{this->x - other.x, this->y - other.y};
  }
  // unary version: negation operator
  Point operator-() const { return Point{-this->x, -this->y}; }
};

struct PointHash {
  std::size_t operator()(const Point& p) const {
    std::size_t seed = 0;
    boost::hash_combine(seed, p.x);
    boost::hash_combine(seed, p.y);
    return seed;
  }
};

using PointSet = std::unordered_set<Point, PointHash>;

int point_to_position(const Point& p) { return p.x * POSITION_LIMIT + p.y; };

static const Point POINT_DISPLACEMENTS[] = {
    // (0,0) is top left
    Point{-1, 0},  // LEFT
    Point{1, 0},   // RIGHT
    Point{0, -1},  // UP
    Point{0, 1}    // DOWN
};

Point getObjectPosition(const PointSet& pixels) {
  Point position{INT_MAX, INT_MAX};
  for (const auto& pixel : pixels) {
    position.x = std::min(pixel.x, position.x);
    position.y = std::min(pixel.y, position.y);
  }
  return position;
};

Point getObjectSize(const PointSet& pixels) {
  Point size{0, 0};
  for (const auto& pixel : pixels) {
    size.x = std::max(pixel.x + 1, size.x);
    size.y = std::max(pixel.y + 1, size.y);
  }
  return size;
};

/* Subtracts the position from the pixels. */
PointSet offsetObjectPixels(PointSet& pixels, Point position) {
  PointSet offset;
  for (const auto& pixel : pixels) {
    offset.insert(pixel - position);
  }
  return offset;
};

/**
 * After adding the `offset` to all points in `s1`, returns whether any of the
 * resulting points occur in `s2`.
 */
bool pointsOverlap(const PointSet& s1, const PointSet& s2,
                   const Point& offset) {
  for (const auto& p : s1)
    if (s2.find(offset + p) != s2.end()) return true;
  return false;
};

/**
 * Computes all positions of a "pusher" object relative to a "pushee" object
 * such that moving the pusher according to the `action` results in a collision
 * with the pushee.
 *
 * @param[out] collisions This set is updated to include the positions of the
 * pusher relative to the pushee that result in collisions.
 * @param[in] action The action that determines the direction of movement of the
 * pusher.
 * @param[in] pusher_pixels The set of all pixels in the pusher object. These
 * pixel positions are measured in the frame of the pusher.
 * @param[in] pushee_pixels The set of all pixels in the pushee object. These
 * pixel positions are measured in the frame of the pushee.
 */
void populateCollisions(std::unordered_set<Position2D>& collisions,
                        const int action, const PointSet& pusher_pixels,
                        const PointSet& pushee_pixels) {
  const Point displacement = POINT_DISPLACEMENTS[action];
  PointSet relative_positions;

  for (const auto& pusher_px : pusher_pixels) {
    for (const auto& pushee_px : pushee_pixels)
      relative_positions.insert(pushee_px - (pusher_px + displacement));
  }

  for (const auto& relative_position : relative_positions) {
    if (!pointsOverlap(pusher_pixels, pushee_pixels, relative_position))
      collisions.insert(point_to_position(relative_position));
  }
};

/**
 * This function is identical to `populateCollisions` except for applying an
 * additional constraint that all of the pusher's pixels must satisfy `0 <= x <
 * width` and `0 <= y < height` when x, y correspond to the position of the
 * pusher relative to the pushee after the pusher moves according to the
 * `action`.
 */
void populateBoundedCollisions(std::unordered_set<Position2D>& collisions,
                               const int action, const PointSet& pusher_pixels,
                               const PointSet& pushee_pixels, const int width,
                               const int height) {
  const Point displacement = POINT_DISPLACEMENTS[action];
  PointSet relative_positions;

  // Note that if the pusher has size 1, `max_x` will be `width - 1`, so the
  // inequality `x <= max_x` is equivalent to `x < width` for integer values.
  const Point pusher_size = getObjectSize(pusher_pixels);
  const auto max_x = width - pusher_size.x;
  const auto max_y = height - pusher_size.y;

  for (const auto& pusher_px : pusher_pixels) {
    for (const auto& pushee_px : pushee_pixels)
      relative_positions.insert(pushee_px - (pusher_px + displacement));
  }

  for (const auto& relative_position : relative_positions) {
    if (relative_position.x >= 0 && relative_position.y >= 0 &&
        relative_position.x <= max_x && relative_position.y <= max_y &&
        !pointsOverlap(pusher_pixels, pushee_pixels, relative_position)) {
      collisions.insert(point_to_position(relative_position));
    }
  }
};

}  // namespace

Position2D xy_to_position(const int x, const int y) {
  return x * POSITION_LIMIT + y;
};

void position_to_xy(const Position2D p, int& x, int& y) {
  x = p / POSITION_LIMIT;
  y = p % POSITION_LIMIT;
};

/*
- Parse the file, loading all objects and their pixels.
- Compute the initial position of every object.
- Compute the boundaries of every object.
- Compute the collisions between every pair of objects.
*/
PushWorldPuzzle::PushWorldPuzzle(const std::string& filename) {
  std::string line, elem_id;
  int line_idx = 0;
  int x, y;
  int elems_per_row;

  std::map<std::string, PointSet> obj_pixels;
  std::vector<std::string> line_elems, cell_elems;

  // Parse the file, loading all objects and their pixels.
  std::ifstream pw_file(filename);

  if (pw_file) {  // opened successfully
    while (getline(pw_file, line)) {
      y = line_idx + 1;

      boost::trim(line);
      boost::split(line_elems, line, boost::is_any_of(" "),
                   boost::token_compress_on);

      if (y == 1) {
        elems_per_row = line_elems.size();
      } else if (line_elems.size() == 0) {
        continue;  // ignore linebreaks
      } else if (elems_per_row != line_elems.size()) {
        throw std::invalid_argument(
            "Rows do not contain the same number of elements.");
      }

      for (x = 1; x <= line_elems.size(); x++) {
        boost::split(cell_elems, line_elems[x - 1], boost::is_any_of("+"),
                     boost::token_compress_on);

        for (int k = 0; k < cell_elems.size(); k++) {
          elem_id = cell_elems[k];
          boost::to_lower(elem_id);
          if (elem_id != ".") {
            obj_pixels[elem_id].insert(Point{x, y});
          }
        }
      }
      line_idx++;
    }
    pw_file.close();
  } else {
    throw std::invalid_argument("Unable to open file");
  }

  if (obj_pixels.find("a") == obj_pixels.end())
    throw std::invalid_argument(
        "Every puzzle must have an agent object whose pixels are "
        "indicated by 'a'.");

  int width = x + 1;
  int height = y + 2;

  if (width >= POSITION_LIMIT || height >= POSITION_LIMIT) {
    throw std::domain_error(
        "The maximum width and height of a PushWorld puzzle is " +
        std::to_string(POSITION_LIMIT));
  }

  // Add walls at the boundaries of the puzzle.
  for (int xx = 0; xx < width; xx++) {
    obj_pixels["w"].insert(Point{xx, 0});
    obj_pixels["w"].insert(Point{xx, height - 1});
  }
  for (int yy = 0; yy < height; yy++) {
    obj_pixels["w"].insert(Point{0, yy});
    obj_pixels["w"].insert(Point{width - 1, yy});
  }

  std::vector<std::string> objects;
  objects.push_back("a");

  std::vector<std::string> goals;
  std::string moveable_id;

  // Compute the initial positions and collision boundaries of every object.
  PointSet pixels;
  std::map<std::string, Point> object_positions;
  Point position;

  for (const auto& pixels_in_object : obj_pixels) {
    elem_id = pixels_in_object.first;
    pixels = pixels_in_object.second;

    if (elem_id != "w" && elem_id != "aw") {
      position = getObjectPosition(pixels);
      object_positions[elem_id] = position;
      pixels = offsetObjectPixels(pixels, position);
      obj_pixels[elem_id] = pixels;
    }

    if (elem_id[0] == 'g') {
      goals.push_back(elem_id);
      moveable_id = elem_id;

      // Require that there is an associated movable with the same name.
      moveable_id.replace(0, 1, "m");
      if (obj_pixels.find(moveable_id) == obj_pixels.end()) {
        throw std::invalid_argument("Goal has no associated moveable object: " +
                                    moveable_id);
      }

      objects.push_back(moveable_id);
    }
  }

  const auto num_goal_entities = goals.size();
  m_goal.resize(num_goal_entities);

  // Create the goal state.
  for (int i = 0; i < num_goal_entities; i++) {
    m_goal[i] = point_to_position(object_positions[goals[i]]);
  }

  // Create the initial state.
  for (const auto& pixels_in_object : obj_pixels) {
    elem_id = pixels_in_object.first;
    if (elem_id[0] == 'm' &&
        find(objects.begin(), objects.end(), elem_id) == objects.end()) {
      objects.push_back(elem_id);
    }
  }

  m_num_objects = objects.size();
  m_initial_state.resize(m_num_objects);

  for (int i = 0; i < m_num_objects; i++) {
    m_initial_state[i] = point_to_position(object_positions[objects[i]]);
  }

  // Create all collision data structures.
  m_object_collisions.resize(m_num_objects);

  // Walls for the agent include both "aw" and "w" pixels.
  for (const auto& px : obj_pixels["w"]) {
    obj_pixels["aw"].insert(px);
  }

  // Populate the agent collisions.
  for (int action = 0; action < NUM_ACTIONS; action++) {
    populateBoundedCollisions(
        m_object_collisions.static_collisions[action][AGENT], action,
        obj_pixels["a"], obj_pixels["aw"], width, height);
  }

  // Populate the wall collisions of all objects other than the agent.
  for (int m = 1; m < objects.size(); m++) {
    for (int action = 0; action < NUM_ACTIONS; action++) {
      populateBoundedCollisions(
          m_object_collisions.static_collisions[action][m], action,
          obj_pixels[objects[m]], obj_pixels["w"], width, height);
    }
  }

  // Populate the collisions between all objects. There's no reason to store
  // collisions caused by objects pushing the agent, since the agent is the
  // cause of all movement.
  for (int pusher = 0; pusher < objects.size(); pusher++) {
    for (int pushee = 1; pushee < objects.size(); pushee++) {
      for (int action = 0; action < NUM_ACTIONS; action++) {
        populateCollisions(
            m_object_collisions.dynamic_collisions[action][pusher][pushee],
            action, obj_pixels[objects[pusher]], obj_pixels[objects[pushee]]);
      }
    }
  }

  init();
}

PushWorldPuzzle::PushWorldPuzzle(const State& initial_state, const Goal& goal,
                                 const ObjectCollisions& object_collisions)
    : m_initial_state(initial_state),
      m_num_objects(initial_state.size()),
      m_goal(goal),
      m_object_collisions(object_collisions) {
  init();
}

void PushWorldPuzzle::init() {
  m_pushing_frontier.resize(m_num_objects);

  m_pushed_object_idxs.resize(m_num_objects);
  m_pushed_object_idxs[0] = AGENT;

  m_pushed_objects.resize(m_num_objects);
  m_pushed_objects[AGENT] = true;
  for (int i = 1; i < m_num_objects; i++) {
    m_pushed_objects[i] = false;
  }
}

RelativeState PushWorldPuzzle::getNextState(const State& state,
                                    const Action action) const {
  const int agent_pos = state[AGENT];
  const auto& static_collisions = m_object_collisions.static_collisions[action];
  const auto& static_agent_collisions = static_collisions[AGENT];

  RelativeState relative_next_state;

  if (static_agent_collisions.find(agent_pos) !=
      static_agent_collisions.end()) {
    // The agent cannot move.
    relative_next_state.state = state;
    return relative_next_state;
  }

  // The frontier stores all objects that are moved by this action that have not
  // yet been checked for whether they push other objects. It is a "search
  // frontier" for pushed objects.
  m_pushing_frontier[0] = AGENT;
  int num_pushed_objects = 1;
  int num_frontier_objects = 1;
  const auto& dynamic_collisions =
      m_object_collisions.dynamic_collisions[action];

  while (num_frontier_objects) {
    auto object_idx = m_pushing_frontier[--num_frontier_objects];
    const Position2D object_position = state[object_idx];
    const auto& dynamic_object_collisions = dynamic_collisions[object_idx];

    for (int obstacle_idx = 1; obstacle_idx < m_num_objects; obstacle_idx++) {
      if (m_pushed_objects[obstacle_idx]) continue;  // already pushed

      int obstacle_position = state[obstacle_idx];
      int relative_pos = object_position - obstacle_position;

      // Test whether the obstacle is pushed by the object.
      if (dynamic_object_collisions[obstacle_idx].find(relative_pos) !=
          dynamic_object_collisions[obstacle_idx].end()) {
        if (static_collisions[obstacle_idx].find(obstacle_position) !=
            static_collisions[obstacle_idx].end()) {
          // transitive stopping; nothing can move.
          for (int i = 1; i < num_pushed_objects; i++) {
            m_pushed_objects[m_pushed_object_idxs[i]] = false;
          }

          relative_next_state.state = state;
          return relative_next_state;
        }

        m_pushed_objects[obstacle_idx] = true;
        m_pushed_object_idxs[num_pushed_objects++] = obstacle_idx;
        m_pushing_frontier[num_frontier_objects++] = obstacle_idx;
      }
    }
  }

  relative_next_state.state.resize(m_initial_state.size());
  const auto displacement = ACTION_DISPLACEMENTS[action];

  // minor optimization to keep this out of the loop below
  relative_next_state.state[AGENT] = state[AGENT] + displacement;
  relative_next_state.moved_object_indices.push_back(AGENT);

  for (int i = 1; i < m_num_objects; i++) {
    if (m_pushed_objects[i]) {
      relative_next_state.state[i] = state[i] + displacement;
      relative_next_state.moved_object_indices.push_back(i);
      m_pushed_objects[i] = false;
    } else {
      relative_next_state.state[i] = state[i];
    }
  }

  return relative_next_state;
}

bool PushWorldPuzzle::satisfiesGoal(const State& state) const {
  int goal_pos;
  for (int i = 0; i < m_goal.size();) {
    goal_pos = m_goal[i];
    if (goal_pos != state[++i]) return false;
  }
  return true;
}

bool PushWorldPuzzle::isValidPlan(const Plan& plan) const {
  auto state = m_initial_state;

  for (const auto action : plan) {
    state = getNextState(state, action).state;
  }

  return satisfiesGoal(state);
}

}  // namespace pushworld
