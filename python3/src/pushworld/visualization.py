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
from PIL import Image

from pushworld.config import BENCHMARK_PUZZLES_PATH, PUZZLE_EXTENSION
from pushworld.puzzle import Actions, PushWorldPuzzle
from pushworld.utils.filesystem import get_puzzle_file_paths, map_files_with_extension
from pushworld.utils.images2mp4 import images2mp4


def render_puzzle_previews(
    image_path: str,
    puzzle_path: str = BENCHMARK_PUZZLES_PATH,
    image_extension: str = ".png",
) -> None:
    """Iterates over all PushWorld puzzles found in the given `puzzle_path` and
    saves them in the `image_path` directory.

    This function recursively descends into all subdirectories in the puzzle path and
    recreates the subdirectory structure in the directory of saved images.

    Args:
        image_path: The path to a directory in which to save the resulting puzzle
            preview images. This directory is created if it does not exist.
        puzzle_path: The path to a PushWorld puzzle file (.pwp) or to a directory
            that contains puzzle files, possibly nested in subdirectories.
        image_extension: The file extension of the saved images. This determines the
            image format.
    """
    for puzzle_file_path, image_file_path in map_files_with_extension(
        input_file_or_directory_path=puzzle_path,
        input_extension=PUZZLE_EXTENSION,
        output_directory_path=image_path,
        output_extension=image_extension,
    ):
        puzzle = PushWorldPuzzle(puzzle_file_path)
        image = puzzle.render(puzzle.initial_state)
        Image.fromarray(image).save(image_file_path)


def render_plans(
    planning_results_path: str,
    video_path: str,
    puzzle_path: str = BENCHMARK_PUZZLES_PATH,
    fps: float = 6.0,
) -> None:
    """Iterates over all planning result YAML files contained within the
    `planning_results_path` directory, and then renders MP4 videos of all plans
    found within the result files.

    Each planning result YAML file must have the following structure:
    ```
    planner: <name of the planner>
    puzzle: <name of the puzzle that the planner attempted to solve>
    plan: <a string of 'UDLR' characters, or `null` if no plan found>
    ```

    Args:
        planning_results_path: The path of a directory that contains planning result
            YAML files.
        video_path: The path of a directory in which videos of the plans are saved.
        puzzle_path: The path of a directory that contains all puzzles that are
            mentioned in the planning result files.
        fps: Frames Per Second in the generated mp4 video files.
    """
    puzzle_file_paths = get_puzzle_file_paths(puzzle_path)

    for planning_result_file_path, video_file_path in map_files_with_extension(
        input_file_or_directory_path=planning_results_path,
        input_extension=".yaml",
        output_directory_path=video_path,
        output_extension=".mp4",
    ):
        with open(planning_result_file_path) as result_file:
            planning_result = yaml.safe_load(result_file)

        plan = planning_result["plan"]
        if plan is None:
            continue

        puzzle_name = planning_result["puzzle"]
        if puzzle_name not in puzzle_file_paths:
            raise ValueError(
                f'No puzzle is named "{puzzle_name}" in the directory: {puzzle_path}'
            )

        puzzle = PushWorldPuzzle(puzzle_file_paths[puzzle_name])
        images = puzzle.render_plan([Actions.FROM_CHAR[ch] for ch in plan.upper()])
        images2mp4(video_file_path, images=images, fps=fps)
