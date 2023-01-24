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
from typing import Dict

import numpy as np


def get_puzzle_transforms(puzzle_string: str) -> Dict[str, str]:
    """Computes all 8 possible combinations of 90 degree rotations and horizontal and
    vertical flips of the given PushWorld puzzle.

    Args:
        puzzle_string: A human-readable representation of a PushWorld puzzle.
            This string uses the format of a `.pwp` file.

    Returns:
        A map from transform names to their associated transformed puzzle strings.
        Always contains 8 elements.
    """
    lines = [l.split() for l in puzzle_string.splitlines()]

    transformed_puzzles = {}

    for flipped in [False, True]:
        for rotation in range(0, 360, 90):
            transformed_puzzle_string = "\n".join(["  ".join(l) for l in lines])
            transformed_puzzle_name = f"r{rotation}{'_flipped' if flipped else ''}"
            transformed_puzzles[transformed_puzzle_name] = transformed_puzzle_string

            lines = np.rot90(lines, axes=(1, 0))

        # flip top to bottom
        lines = lines[::-1]

    return transformed_puzzles


def create_transformed_puzzles(puzzle_path: str, output_path: str) -> None:
    """For every puzzle found in the `puzzle_path`, this function rotates and flips
    the puzzle into all 8 possible transformations and saves the resulting transformed
    puzzles into the `output_path`.

    Args:
        puzzle_path: The path of the directory that contains the PushWorld puzzles
            to transform.
        output_path: The path of the directory in which all transformed puzzles
            are written.
    """
    puzzle_path = puzzle_path.rstrip(os.path.sep)

    for subdir, _, filenames in os.walk(puzzle_path):
        for filename in filenames:
            if filename.endswith(".pwp"):
                puzzle_file_path = os.path.join(subdir, filename)

                with open(puzzle_file_path, "r") as puzzle_file:
                    puzzle_string = puzzle_file.read()

                aug_puzzles = get_puzzle_transforms(puzzle_string)

                for puzzle_name, puzzle_string in aug_puzzles.items():
                    file_prefix, _ = os.path.splitext(
                        puzzle_file_path[len(puzzle_path) + 1 :]
                    )
                    transformed_file_path = os.path.join(
                        output_path, f"{file_prefix}_{puzzle_name}.pwp"
                    )
                    parent_directory, _ = os.path.split(transformed_file_path)
                    os.makedirs(parent_directory, exist_ok=True)

                    with open(transformed_file_path, "w") as fp:
                        fp.write(puzzle_string)
