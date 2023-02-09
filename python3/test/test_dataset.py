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

import yaml

from pushworld.config import BENCHMARK_PUZZLES_PATH, BENCHMARK_SOLUTIONS_PATH
from pushworld.puzzle import Actions, PushWorldPuzzle
from pushworld.utils.filesystem import get_puzzle_file_paths, iter_files_with_extension


def test_dataset() -> None:
    """Checks that every puzzle in the dataset has a solution."""

    puzzle_file_paths = get_puzzle_file_paths(BENCHMARK_PUZZLES_PATH)
    visited_puzzles = set()
    errors = []

    for result_file_path in iter_files_with_extension(
        BENCHMARK_SOLUTIONS_PATH, ".yaml"
    ):
        with open(result_file_path) as file:
            planning_result = yaml.safe_load(file)

        puzzle_name = planning_result["puzzle"]
        if puzzle_name not in puzzle_file_paths:
            errors.append(
                f'No puzzle is named "{puzzle_name}" in the directory: '
                f"{BENCHMARK_PUZZLES_PATH}. Referenced from: {result_file_path}"
            )
            continue

        puzzle = PushWorldPuzzle(puzzle_file_paths[puzzle_name])

        plan_string = planning_result["plan"]
        plan = [Actions.FROM_CHAR[s] for s in plan_string.upper()]

        if puzzle.is_valid_plan(plan):
            visited_puzzles.add(puzzle_name)
        else:
            errors.append(f"Detected invalid plan in {result_file_path}")

    for puzzle_name, file_path in puzzle_file_paths.items():
        if puzzle_name not in visited_puzzles:
            errors.append(f"No solution found for puzzle: {puzzle_name} ({file_path})")

    # Raise all errors in batch, which is easier to debug.
    if len(errors) > 0:
        raise ValueError("\n".join(errors))
