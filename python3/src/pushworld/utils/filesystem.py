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
from typing import Dict, Generator, Optional, Tuple

from pushworld.config import BENCHMARK_PUZZLES_PATH, PUZZLE_EXTENSION


def iter_files_with_extension(
    file_or_directory_path: str, extension: str
) -> Generator[str, None, None]:
    """Yields the paths of all files nested in a given directory that have the given
    extension.

    Args:
        file_or_directory_path: A path to a file or a directory in which to search for
            files with the given `extension`. If this is a path to a file, then it must
            have the correct extension.
        extension: The extension to search for. Case insensitive.

    Yields:
        If `file_or_directory_path` is a file with the given extension, this generator
        only yields the path of this file. Otherwise this generator yields the path
        to every file in `file_or_directory_path` or any of its subdirectories that
        has the given extension.

    Raises:
        ValueError: If `file_or_directory_path` refers to a file but has the wrong
            extension.
    """
    extension = extension.lower()
    file_or_directory_path = file_or_directory_path.rstrip(os.path.sep)

    if os.path.isfile(file_or_directory_path):
        if file_or_directory_path.lower().endswith(extension):
            yield file_or_directory_path
        else:
            raise ValueError(
                "The given file does not have the expected "
                f"extension ({extension}): {file_or_directory_path}"
            )

    else:
        # Not a file, so must be a directory.
        for parent_directory_path, _, filenames in os.walk(file_or_directory_path):
            for filename in filenames:
                if filename.lower().endswith(extension):
                    file_path = os.path.join(parent_directory_path, filename)
                    yield file_path


def map_files_with_extension(
    input_file_or_directory_path: str,
    input_extension: str,
    output_directory_path: str,
    output_extension: Optional[str] = None,
) -> Generator[Tuple[str, str], None, None]:
    """Maps files from one directory to another while replacing their extensions and
    maintaining the structure of subdirectories.

    For example, if an input directory contains these files:

        input_directory/
            sub_directory/
                foo.yaml
            another_sub_directory/
                bar.png
            baz.yaml

    Then when `input_extension` is ".yaml" and `output_extension` is ".gif", this
    function will yield the paths to the following files and create all necessary
    subdirectories to match the structure of `input_directory`:

        output_directory/     <-- this directory is created
            sub_directory/    <-- this directory is created
                foo.gif       <-- this file path is yielded
            baz.gif           <-- this file path is yielded

    Args:
        input_file_or_directory_path: A path to a file or a directory in which to search
            for files with the given `extension`. If this is a path to a file, then it
            must have the correct extension.
        input_extension: The extension to search for in the
            `input_file_or_directory_path`. Case insensitive.
        output_directory_path: The path of the directory where output files are stored.
            This directory is created if it does not already exist, as well as all
            subdirectories needed to match the structure of the input directory.
        output_extension: The extension of output files.

    Yields:
        (input file path, output file path) pairs. The parent directory of the output
        file is guaranteed to exist.
    """
    if isinstance(output_extension, str) and not output_extension.startswith("."):
        output_extension = "." + output_extension

    for input_file_path in iter_files_with_extension(
        input_file_or_directory_path, input_extension
    ):
        input_parent_directory_path, filename = os.path.split(input_file_path)
        output_filename = os.path.splitext(filename)[0]
        if output_extension is not None:
            output_filename += output_extension

        if input_file_path == input_file_or_directory_path:
            output_parent_directory_path = output_directory_path
        else:
            output_parent_directory_path = os.path.join(
                output_directory_path,
                input_parent_directory_path[len(input_file_or_directory_path) + 1 :],
            )

        # Create the output (sub)directory if it does not already exist.
        os.makedirs(output_parent_directory_path, exist_ok=True)

        output_file_path = os.path.join(output_parent_directory_path, output_filename)
        yield input_file_path, output_file_path


def get_puzzle_file_paths(puzzle_path: str = BENCHMARK_PUZZLES_PATH) -> Dict[str, str]:
    """Returns the paths to all PushWorld puzzles in a directory, including
    nested subdirectories.

    Args:
        puzzle_path: The path to the directory containing PushWorld puzzle files with
            the `PUZZLE_EXTENSION` extension.

    Returns:
        A map from puzzle names to filenames.

    Raises:
        ValueError: If two puzzles have the same name.
    """
    puzzles = {}

    for puzzle_file_path in iter_files_with_extension(puzzle_path, PUZZLE_EXTENSION):
        puzzle_name = os.path.splitext(os.path.split(puzzle_file_path)[1])[0]

        if puzzle_name in puzzles:
            raise ValueError(
                f'Two puzzles have the same name "{puzzle_name}": '
                f"{puzzle_file_path} and {puzzles[puzzle_name]}"
            )

        puzzles[puzzle_name] = puzzle_file_path

    return puzzles
