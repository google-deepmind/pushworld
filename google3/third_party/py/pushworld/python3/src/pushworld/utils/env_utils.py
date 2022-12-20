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

from typing import Dict, Generator, Optional, Tuple
import numpy as np

from pushworld.config import BENCHMARK_PUZZLES_PATH, PUZZLE_EXTENSION
from pushworld.puzzle import (
    PushWorldPuzzle,
    State,
)
from pushworld.utils.filesystem import iter_files_with_extension

def get_max_puzzle_dimensions() -> Tuple[int, int]:
    """Returns the (max height, max width) of PushWorld puzzles in the
    `pushworld.config.BENCHMARK_PUZZLES_PATH` directory."""
    max_height = 0
    max_width = 0

    for puzzle_file_path in iter_files_with_extension(
        BENCHMARK_PUZZLES_PATH, PUZZLE_EXTENSION
    ):
        with open(puzzle_file_path, "r") as puzzle_file:
            lines = puzzle_file.readlines()

        # Add 2 for the outer walls
        max_height = max(max_height, len(lines) + 2)
        max_width = max(max_width, len(lines[0].strip().split()) + 2)

    return max_height, max_width


def render_observation_padded(
    puzzle: PushWorldPuzzle,
    state: State,
    max_cell_height: int,
    max_cell_width: int,
    pixels_per_cell: int,
    border_width: int,
) -> np.ndarray:
    """Renders an observation of the state of a puzzle.

    Args:
        puzzle: The puzzle to observe.
        state: The state of the puzzle.

    Returns:
        An observation of the PushWorld environment. This observation is
        formatted
        as an RGB image with shape (height, width, 3) with `float32` type and
        values
        ranging from [0, 1].
    """
    image = (
        puzzle.render(
            state,
            border_width=border_width,
            pixels_per_cell=pixels_per_cell,
        ).astype(np.float32)
        / 255
    )

    # Pad the image to the correct size.
    height_padding = (
        max_cell_height * pixels_per_cell - image.shape[0]
    )
    width_padding = (
        max_cell_width * pixels_per_cell - image.shape[1]
    )
    half_height_padding = height_padding // 2
    half_width_padding = width_padding // 2

    return np.pad(
        image,
        pad_width=[
            (half_height_padding, height_padding - half_height_padding),
            (half_width_padding, width_padding - half_width_padding),
            (0, 0),
        ],
    )
