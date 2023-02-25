# Copyright 2022 DeepMind Technologies Limited
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import os
import random

from pushworld.benchmark_rgd import benchmark_rgd_planner
import tqdm
import yaml


class FailedToGenerateError(Exception):
  """Raised if puzzle generation can't be completed in current state."""
  pass


def place_object(
    puzzle: list[list[str]],
    object_symbol: str,
    shape: list[tuple[int, int]],
):
  """Draws the object into the given image.

  Args:
      puzzle: The partially generated puzzle to place an object in.
      object_symbol: The string to represent the object. e.g. "A", "W".
      shape: Offset indicies describing the object's shape.

  Raises:
      FailedToGenerateError: if the puzzle can't be completed
  """
  puzzle_height = len(puzzle)
  puzzle_width = len(puzzle[0])

  shape_height = 1 + max([y for y, x in shape])
  shape_width = 1 + max([x for y, x in shape])

  attempts = 0
  while True:
    attempts += 1
    if attempts > 100:
      raise FailedToGenerateError()

    # get a random location to attempt to place the object
    x = random.choice(range(puzzle_width + 1 - shape_width))
    y = random.choice(range(puzzle_height + 1 - shape_height))

    # check that that location is clear for the shape
    clear = True
    for offset in shape:
      y_offset, x_offset = offset
      if puzzle[y + y_offset][x + x_offset] != ".":
        clear = False

    # if the location is clear, place the object
    if clear:
      for offset in shape:
        y_offset, x_offset = offset
        puzzle[y + y_offset][x + x_offset] = object_symbol
      break


def generate_puzzle(
    puzzle_width: int,
    puzzle_height: int,
    num_walls: int,
    num_obstacles: int,
    num_goal_objects: int,
    possible_object_shapes: list[list[tuple[int, int]]],
):
  """Attempts to generate a single puzzle.

  Args:
      puzzle_width: The width of the puzzle.
      puzzle_height: The height of the puzzle.
      num_walls: The number of single-pixel walls to place in the puzzle.
      num_obstacles: The number of obstacle objects to place.
      num_goal_objects: The number of goal objects (and goals) to place.
      possible_object_shapes: A list of possible shapes to use for the agent,
        goals, and obstacles.

  Returns:
      A string representation of a puzzle.

  Raises:
      FailedToGenerateError: if the puzzle can't be completed
  """
  assert (
      len(possible_object_shapes) >= num_goal_objects
  ), "need a distinct shape for each goal object"

  puzzle = [["." for _ in range(puzzle_width)] for _ in range(puzzle_height)]

  goal_object_1_shape = random.choice(possible_object_shapes)
  place_object(puzzle, "M1", shape=goal_object_1_shape)
  place_object(puzzle, "G1", shape=goal_object_1_shape)

  if num_goal_objects == 2:
    goal_object_2_shape = None
    while (
        goal_object_2_shape is None
        or goal_object_2_shape == goal_object_1_shape
    ):
      goal_object_2_shape = random.choice(possible_object_shapes)
    place_object(puzzle, "M2", shape=goal_object_2_shape)
    place_object(puzzle, "G2", shape=goal_object_2_shape)

  place_object(
      puzzle, "A", shape=random.choice(possible_object_shapes)
  )

  for i in range(num_obstacles):
    place_object(
        puzzle,
        f"M{1 + i + num_goal_objects}",
        shape=random.choice(possible_object_shapes),
    )

  for _ in range(num_walls):
    place_object(puzzle, "W", shape=[(0, 0)])

  return "\n".join(["  ".join(l) for l in puzzle])


def generate_level0_puzzles(
    save_location_path: str,
    num_puzzles: int = 5,
    random_seed: int = 0,
    filter_puzzles: bool = True,
    time_limit: int = 2,
    min_puzzle_size: int = 8,
    max_puzzle_size: int = 12,
    min_num_walls: int = 2,
    max_num_walls: int = 4,
    min_num_obstacles: int = 1,
    max_num_obstacles: int = 2,
    min_num_goal_objects: int = 1,
    max_num_goal_objects: int = 1,
    object_shapes: str = "complex",
):
  """Generates puzzles matching given constraints.

  Generation is naive, so if constraints are too tight,
    such as being too small for the number of shapes asked for, generation may
    be slow or impossible to complete.

  Args:
      save_location_path: Name for directory to store generated puzzles. Must be
        empty or not yet exist.
      num_puzzles: The number of puzzles to generate. If puzzles are filtered
        for solvability the number remaining may be lower.
      random_seed: The seed to use for randomization.
      filter_puzzles: If True, run a solver to filter out unsolvable puzzles.
      time_limit: The number of seconds to run the solver on each level, if
        filter_puzzles is True.
      min_puzzle_size: The minimum height and width of puzzles.
      max_puzzle_size: The maximum height and width of puzzles.
      min_num_walls: The minimum number of single-pixel walls to place inside
        puzzles.
      max_num_walls: The maximum number of single-pixel walls to place inside
        puzzles.
      min_num_obstacles: The minimum number of obstacle shapes to add.
      max_num_obstacles: The maximum number of obstacle shapes to add.
      min_num_goal_objects: The minimum number of goals object and goals to add.
      max_num_goal_objects: The maximum number of goal objects and goals to add.
      object_shapes: Can be either "simple" or "complex". If "simple, the agent,
        goals, and obstacle shapes will all be 1x1. If "complex" they will be
        random trominos.
  """
  random.seed(random_seed)

  if not os.path.exists(save_location_path):
    os.makedirs(save_location_path)
  if os.listdir(save_location_path):
    raise ValueError(f"{save_location_path} is not empty!")

  if num_puzzles < 1:
    raise ValueError("num_puzzles must be at least 1")
  if min_puzzle_size < 2 or min_puzzle_size > max_puzzle_size:
    raise ValueError(
        "min_puzzle_size must be >1 and no bigger than max_puzzle_size"
    )
  if min_num_walls < 0 or min_num_walls > max_num_walls:
    raise ValueError(
        "min_num_walls must be >=0 and no bigger than max_num_walls"
    )
  if min_num_obstacles < 0 or min_num_obstacles > max_num_obstacles:
    raise ValueError(
        "min_num_obstacles must be >=0 and no bigger than max_num_obstacles"
    )
  if (
      min_num_goal_objects < 1
      or max_num_goal_objects > 2
      or min_num_goal_objects > max_num_goal_objects
  ):
    raise ValueError(
        "min_num_goal_objects must be >0, max_num_goal_objects must be <3, and"
        " min_num_goal_objects must be no bigger than max_num_goal_objects"
    )

  if object_shapes == "simple":
    possible_shapes = [
        [(0, 0)],
    ]
  elif object_shapes == "complex":
    possible_shapes = [
        [(0, 0)],
        [(0, 0), (0, 1)],
        [(0, 0), (1, 0)],
        [(0, 0), (1, 0), (1, 1)],
        [(0, 0), (0, 1), (1, 1)],
        [(0, 0), (0, 1), (1, 0)],
        [(1, 0), (0, 1), (1, 1)],
        [(0, 0), (0, 1), (0, 2)],
        [(0, 0), (1, 0), (2, 0)],
    ]
  else:
    raise ValueError("object_shapes must be either 'simple' or 'complex'")

  for i in tqdm.tqdm(range(num_puzzles)):
    while True:
      try:
        p_string = generate_puzzle(
            puzzle_width=random.choice(
                range(min_puzzle_size, max_puzzle_size + 1)
            ),
            puzzle_height=random.choice(
                range(min_puzzle_size, max_puzzle_size + 1)
            ),
            num_walls=random.choice(range(min_num_walls, max_num_walls + 1)),
            num_obstacles=random.choice(
                range(min_num_obstacles, max_num_obstacles + 1)
            ),
            num_goal_objects=random.choice(
                range(min_num_goal_objects, max_num_goal_objects + 1)
            ),
            possible_object_shapes=possible_shapes,
        )
        break
      except FailedToGenerateError:
        pass

    fname = f"{save_location_path}/puzzle_{i}.pwp"
    with open(fname, "w+") as f:
      f.write(p_string)

  if filter_puzzles:
    filter_puzzles_by_solvability(save_location_path, time_limit, num_puzzles)


def filter_puzzles_by_solvability(
    path: str, time_limit: int, num_puzzles: int
):
  """Filters out unsolvable puzzles.

  Args:
    path: The location of the puzzles to filter.
    time_limit: The number of seconds to run the solver on each puzzle.
    num_puzzles: The number of puzzles to expect.
  """
  print(
      "Running planner to filter puzzles for solvability, with time_limit:"
      f" {time_limit}"
  )
  benchmark_rgd_planner(
      puzzles_path=path, time_limit=time_limit, results_path=path
  )

  def puzzle_path(index, extension):
    return f"{path}/puzzle_{index}.{extension}"

  solved_map = dict()
  for i in range(num_puzzles):
    with open(puzzle_path(i, "yaml")) as file:
      results = yaml.safe_load(file)
    if results["plan"] is not None:
      solved_map[i] = len(solved_map)

  print(f"{len(solved_map)}/{num_puzzles} were solvable")

  for i in range(num_puzzles):
    if i in solved_map:
      os.rename(puzzle_path(i, "pwp"), puzzle_path(solved_map[i], "pwp"))
    else:
      os.remove(puzzle_path(i, "pwp"))
    os.remove(puzzle_path(i, "yaml"))
