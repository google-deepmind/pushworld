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

from pushworld.config import PUZZLE_EXTENSION
from pushworld.puzzle import AGENT_IDX, Actions, PushWorldPuzzle


def _get_test_puzzle_file_path(puzzle_name: str) -> str:
    """Returns the file path of the puzzle with the given name."""
    return os.path.join(
        os.path.split(__file__)[0], "puzzles", puzzle_name + PUZZLE_EXTENSION
    )


def test_agent_movement() -> None:
    """Tests that the agent object moves correctly in free space and near walls."""
    puzzle = PushWorldPuzzle(_get_test_puzzle_file_path("agent_movement"))

    next_state = puzzle.get_next_state(puzzle.initial_state, Actions.LEFT)
    assert next_state[0] == (1, 2)

    next_state = puzzle.get_next_state(puzzle.initial_state, Actions.RIGHT)
    assert next_state[0] == (3, 2)

    next_state = puzzle.get_next_state(puzzle.initial_state, Actions.UP)
    assert next_state[0] == (2, 1)

    next_state = puzzle.get_next_state(puzzle.initial_state, Actions.DOWN)
    assert next_state[0] == (2, 3)

    # Add a left agent wall
    puzzle._agent_collision_map[Actions.LEFT].add((2, 2))

    next_state = puzzle.get_next_state(puzzle.initial_state, Actions.LEFT)
    assert next_state[0] == (2, 2)

    next_state = puzzle.get_next_state(puzzle.initial_state, Actions.RIGHT)
    assert next_state[0] == (3, 2)

    # Add a right agent wall
    puzzle._agent_collision_map[Actions.RIGHT].add((2, 2))
    next_state = puzzle.get_next_state(puzzle.initial_state, Actions.RIGHT)
    assert next_state[0] == (2, 2)

    # Add a top agent wall
    puzzle._agent_collision_map[Actions.UP].add((2, 2))
    next_state = puzzle.get_next_state(puzzle.initial_state, Actions.UP)
    assert next_state[0] == (2, 2)

    # Add a bottom agent wall
    puzzle._agent_collision_map[Actions.DOWN].add((2, 2))
    next_state = puzzle.get_next_state(puzzle.initial_state, Actions.DOWN)
    assert next_state[0] == (2, 2)


def test_pushing() -> None:
    """Tests directly pushing an object in contact with the agent."""
    puzzle = PushWorldPuzzle(_get_test_puzzle_file_path("pushing"))

    next_state = puzzle.get_next_state(puzzle.initial_state, Actions.DOWN)
    assert next_state == ((1, 2), (2, 1))
    next_state = puzzle.get_next_state(puzzle.initial_state, Actions.RIGHT)
    assert next_state == ((2, 1), (3, 1))
    next_state = puzzle.get_next_state(next_state, Actions.RIGHT)
    assert next_state == ((3, 1), (4, 1))
    next_state = puzzle.get_next_state(next_state, Actions.RIGHT)
    assert next_state == ((3, 1), (4, 1))  # transitive stopping


def test_transitive_pushing() -> None:
    """Tests indirectly pushing an object using another object as a tool."""
    puzzle = PushWorldPuzzle(_get_test_puzzle_file_path("transitive_pushing"))

    next_state = puzzle.get_next_state(puzzle.initial_state, Actions.DOWN)
    assert next_state == ((1, 2), (5, 1), (3, 1))
    next_state = puzzle.get_next_state(puzzle.initial_state, Actions.RIGHT)
    assert next_state == ((2, 1), (5, 1), (3, 1))
    next_state = puzzle.get_next_state(next_state, Actions.RIGHT)
    assert next_state == ((3, 1), (5, 1), (4, 1))
    next_state = puzzle.get_next_state(next_state, Actions.RIGHT)
    assert next_state == ((4, 1), (6, 1), (5, 1))
    next_state = puzzle.get_next_state(next_state, Actions.RIGHT)  # transitive stopping
    assert next_state == ((4, 1), (6, 1), (5, 1))
    next_state = puzzle.get_next_state(next_state, Actions.DOWN)
    assert next_state == ((4, 2), (6, 1), (5, 1))


def test_goal_states() -> None:
    """Checks `PushWorldPuzzle.is_goal_state` and `count_achieved_goals`."""
    puzzle = PushWorldPuzzle(_get_test_puzzle_file_path("is_goal_state"))

    state = ((5, 1), (3, 6), (2, 5))
    assert puzzle.is_goal_state(state)
    assert puzzle.count_achieved_goals(state) == 2

    state = ((2, 8), (3, 6), (2, 5))
    assert puzzle.is_goal_state(state)
    assert puzzle.count_achieved_goals(state) == 2

    state = ((1, 1), (3, 3), (2, 5))
    assert not puzzle.is_goal_state(state)
    assert puzzle.count_achieved_goals(state) == 1

    state = ((1, 1), (3, 6), (2, 2))
    assert not puzzle.is_goal_state(state)
    assert puzzle.count_achieved_goals(state) == 1

    state = ((1, 1), (3, 4), (1, 5))
    assert not puzzle.is_goal_state(state)
    assert puzzle.count_achieved_goals(state) == 0


def test_trivial_file_loading() -> None:
    """Verifies that a trivial puzzle can step through actions and detect goal states."""
    puzzle = PushWorldPuzzle(_get_test_puzzle_file_path("trivial"))

    assert puzzle.goal_state == ((3, 1),)
    assert puzzle.initial_state == ((1, 2), (2, 2))

    # Verify the collision maps
    assert len(puzzle._agent_collision_map[Actions.LEFT]) == 3
    assert len(puzzle._agent_collision_map[Actions.RIGHT]) == 3
    assert len(puzzle._agent_collision_map[Actions.UP]) == 3
    assert len(puzzle._agent_collision_map[Actions.DOWN]) == 3

    assert puzzle._agent_collision_map[Actions.LEFT] == set([(2, 1), (1, 2), (2, 3)])
    assert puzzle._agent_collision_map[Actions.UP] == set([(1, 2), (2, 1), (3, 1)])
    assert puzzle._agent_collision_map[Actions.RIGHT] == set([(3, 1), (3, 2), (3, 3)])
    assert puzzle._agent_collision_map[Actions.DOWN] == set([(1, 2), (2, 3), (3, 3)])

    # Verify the solution to the puzzle
    state = puzzle.initial_state
    assert not puzzle.is_goal_state(state)

    state = puzzle.get_next_state(state, Actions.LEFT)  # Push into a wall. No change.
    assert state == ((1, 2), (2, 2))
    assert not puzzle.is_goal_state(state)

    state = puzzle.get_next_state(state, Actions.UP)  # Push into a wall. No change.
    assert state == ((1, 2), (2, 2))
    assert not puzzle.is_goal_state(state)

    state = puzzle.get_next_state(
        state, Actions.DOWN
    )  # Push into a agent wall. No change.
    assert state == ((1, 2), (2, 2))
    assert not puzzle.is_goal_state(state)

    state = puzzle.get_next_state(state, Actions.RIGHT)
    assert state == ((2, 2), (3, 2))
    assert not puzzle.is_goal_state(state)

    state = puzzle.get_next_state(
        state, Actions.RIGHT
    )  # Transitive stopping. No change.
    assert state == ((2, 2), (3, 2))
    assert not puzzle.is_goal_state(state)

    state = puzzle.get_next_state(state, Actions.DOWN)
    assert state == ((2, 3), (3, 2))
    assert not puzzle.is_goal_state(state)

    state = puzzle.get_next_state(state, Actions.DOWN)  # Push into a wall. No change.
    assert state == ((2, 3), (3, 2))
    assert not puzzle.is_goal_state(state)

    state = puzzle.get_next_state(state, Actions.RIGHT)
    assert state == ((3, 3), (3, 2))
    assert not puzzle.is_goal_state(state)

    state = puzzle.get_next_state(state, Actions.RIGHT)  # Push into a wall. No change.
    assert state == ((3, 3), (3, 2))
    assert not puzzle.is_goal_state(state)

    state = puzzle.get_next_state(state, Actions.UP)
    assert state == ((3, 2), (3, 1))
    assert puzzle.is_goal_state(state)

    state = puzzle.get_next_state(state, Actions.UP)  # Transitive stopping. No change.
    assert state == ((3, 2), (3, 1))
    assert puzzle.is_goal_state(state)


def test_file_parsing() -> None:
    """Tests that all properties of a puzzle are loaded correctly from a file,
    including computed collision maps."""
    puzzle = PushWorldPuzzle(_get_test_puzzle_file_path("file_parsing"))

    assert puzzle.dimensions == (12, 18)
    assert puzzle.goal_state == ((6, 5), (3, 4))
    assert puzzle.initial_state == (
        (1, 12),  # S
        (6, 14),  # M4
        (1, 3),  # M1
        (4, 1),  # M0
        (2, 7),  # M2
        (3, 8),  # M3
    )

    # Object indices the state
    M4, M1, M0, M2, M3 = range(1, 6)

    assert len(puzzle._agent_collision_map[Actions.LEFT]) == 16
    assert len(puzzle._wall_collision_map[Actions.LEFT][M0]) == 15
    assert len(puzzle._wall_collision_map[Actions.LEFT][M1]) == 16
    assert len(puzzle._wall_collision_map[Actions.LEFT][M2]) == 14
    assert len(puzzle._wall_collision_map[Actions.LEFT][M3]) == 16
    assert len(puzzle._wall_collision_map[Actions.LEFT][M4]) == 15

    assert len(puzzle._agent_collision_map[Actions.RIGHT]) == 16
    assert len(puzzle._wall_collision_map[Actions.RIGHT][M0]) == 15
    assert len(puzzle._wall_collision_map[Actions.RIGHT][M1]) == 16
    assert len(puzzle._wall_collision_map[Actions.RIGHT][M2]) == 14
    assert len(puzzle._wall_collision_map[Actions.RIGHT][M3]) == 16
    assert len(puzzle._wall_collision_map[Actions.RIGHT][M4]) == 15

    assert len(puzzle._agent_collision_map[Actions.UP]) == 9
    assert len(puzzle._wall_collision_map[Actions.UP][M0]) == 9
    assert len(puzzle._wall_collision_map[Actions.UP][M1]) == 10
    assert len(puzzle._wall_collision_map[Actions.UP][M2]) == 8
    assert len(puzzle._wall_collision_map[Actions.UP][M3]) == 10
    assert len(puzzle._wall_collision_map[Actions.UP][M4]) == 9

    assert len(puzzle._movable_collision_map[Actions.DOWN][AGENT_IDX][M4]) == 4
    assert len(puzzle._movable_collision_map[Actions.DOWN][AGENT_IDX][M3]) == 3
    assert len(puzzle._movable_collision_map[Actions.DOWN][M1][M4]) == 2
    assert len(puzzle._movable_collision_map[Actions.DOWN][M1][M2]) == 4
    assert len(puzzle._movable_collision_map[Actions.LEFT][M1][M4]) == 2
    assert len(puzzle._movable_collision_map[Actions.LEFT][M1][M2]) == 4
    assert len(puzzle._movable_collision_map[Actions.RIGHT][M1][M4]) == 2
    assert len(puzzle._movable_collision_map[Actions.RIGHT][M1][M2]) == 4
    assert len(puzzle._movable_collision_map[Actions.UP][M1][M4]) == 2
    assert len(puzzle._movable_collision_map[Actions.UP][M1][M2]) == 4


def test_rendering() -> None:
    """Tests `PushWorldPuzzle.render` and `render_plan`."""
    puzzle = PushWorldPuzzle(_get_test_puzzle_file_path("trivial"))

    state = puzzle.initial_state
    initial_image = puzzle.render(state)
    assert initial_image.shape == (100, 100, 3)

    plan = [Actions.RIGHT, Actions.DOWN, Actions.RIGHT, Actions.UP]
    plan_images = puzzle.render_plan(plan)

    assert (initial_image == plan_images[0]).all()

    # Compare hashes of the images so that this file doesn't store all of the image
    # arrays.
    image_hashes = [
        8141256401900123811,
        1770142108181252064,
        4744825492003518882,
        -7463149466192975143,
        -8235536721686713717,
    ]
    assert image_hashes == [hash(tuple(image.flat)) for image in plan_images]
