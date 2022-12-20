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
import re
import tempfile

from pushworld.config import PUZZLE_EXTENSION
from pushworld.puzzle import Actions, PushWorldPuzzle
from pushworld.transform import get_puzzle_transforms


def test_transformation():
    """Tests `get_puzzle_transforms` by checking that the transformed solutions solve
    the transformed puzzles."""
    test_puzzle = os.path.join(
        os.path.split(__file__)[0], "puzzles", "shortest_path_tool.pwp"
    )
    tempdir = tempfile.gettempdir()

    plan = [
        Actions.LEFT,
        Actions.UP,
        Actions.UP,
        Actions.UP,
        Actions.RIGHT,
        Actions.DOWN,
        Actions.DOWN,
        Actions.DOWN,
        Actions.DOWN,
    ]

    puzzle = PushWorldPuzzle(test_puzzle)
    assert puzzle.is_valid_plan(plan)  # sanity check

    actions_rot90_map = {
        Actions.UP: Actions.RIGHT,
        Actions.RIGHT: Actions.DOWN,
        Actions.DOWN: Actions.LEFT,
        Actions.LEFT: Actions.UP,
    }
    actions_flipped_map = {
        Actions.UP: Actions.DOWN,
        Actions.DOWN: Actions.UP,
        Actions.RIGHT: Actions.RIGHT,
        Actions.LEFT: Actions.LEFT,
    }

    with open(test_puzzle, "r") as fp:
        puzzle_string = fp.read()

    created_puzzles = get_puzzle_transforms(puzzle_string)
    assert len(created_puzzles) == 8

    for transform_name, puzzle_string in created_puzzles.items():
        # only one number in the transform name
        rotation_angle = int(re.findall(r"\d+", transform_name)[0])

        puzzle_file_path = os.path.join(tempdir, transform_name + PUZZLE_EXTENSION)

        with open(puzzle_file_path, "w") as puzzle_file:
            puzzle_file.write(puzzle_string)

        puzzle = PushWorldPuzzle(puzzle_file_path)

        transformed_plan = plan
        if "flipped" in transform_name:
            transformed_plan = [actions_flipped_map[i] for i in transformed_plan]
        for _ in range(rotation_angle // 90):
            transformed_plan = [actions_rot90_map[i] for i in transformed_plan]

        assert puzzle.is_valid_plan(transformed_plan)
