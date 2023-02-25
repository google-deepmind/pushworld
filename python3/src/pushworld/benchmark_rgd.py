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
from typing import Optional

import tqdm
import yaml

from pushworld.config import BENCHMARK_PUZZLES_PATH, PUZZLE_EXTENSION, RGD_PLANNER_PATH
from pushworld.puzzle import Actions, PushWorldPuzzle
from pushworld.utils.filesystem import get_puzzle_file_paths, map_files_with_extension
from pushworld.utils.process import GIGABYTE, run_process


def benchmark_rgd_planner(
    results_path: str = "nrgd_results",
    puzzles_path: str = BENCHMARK_PUZZLES_PATH,
    planner_path: str = RGD_PLANNER_PATH,
    heuristic: str = "N+RGD",
    time_limit: Optional[int] = 60 * 30,
    memory_limit: Optional[float] = 30,
) -> None:
    """Benchmarks a greedy best-first search using the recursive graph distance
    heuristic on a collection of PushWorld puzzles.

    Args:
        results_path: The path of the directory in which to save the benchmark results,
            which include one YAML file per puzzle. These YAMLs contain the following
            top-level key-value pairs:
                planner: <name of the planner>
                puzzle: <name of the puzzle that the planner attempted to solve>
                plan: <a string of 'UDLR' characters, or `null` if no plan found>
                planning_time: <the time the planner spent searching for a plan>
                failure_reason: <if a plan was not found, this summarizes why>
        puzzles_path: The path of the directory from which to load PushWorld puzzles
            to benchmark. Can also be the path of a single PushWorld puzzle file.
        planner_path: The path of the RGD planner executable.
        heuristic: The heuristic mode of the planner. See the RGD planner executable
            for available options.
        time_limit: In seconds, the maximum CPU time for which the planner is allowed
            to run to solve a single puzzle. If None, there is no limit.
        memory_limit: In gigabytes, the maximum memory that the planner is allowed
            to use to solve a single puzzle. If None, there is no limit.
    """
    heuristic_to_planner_name = {
        "N+RGD": "Novelty+RGD",
        "RGD": "RGD",
    }

    if heuristic not in heuristic_to_planner_name:
        raise ValueError(
            f'Unknown heuristic: "{heuristic}". '
            f"Supported values are {list(heuristic_to_planner_name.keys())}"
        )

    planner_name = heuristic_to_planner_name[heuristic]

    for puzzle_file_path, planning_result_file_path in tqdm.tqdm(
        map_files_with_extension(
            input_file_or_directory_path=puzzles_path,
            input_extension=PUZZLE_EXTENSION,
            output_directory_path=results_path,
            output_extension=".yaml",
        )
    ):
        out, _, planning_time = run_process(
            command=[RGD_PLANNER_PATH, heuristic, puzzle_file_path],
            time_limit=time_limit,
            memory_limit=(
                None if memory_limit is None else int(memory_limit * GIGABYTE)
            ),
        )

        puzzle_name = os.path.splitext(os.path.split(puzzle_file_path)[1])[0]

        planning_result = {
            "planner": planner_name,
            "puzzle": puzzle_name,
            "planning_time": planning_time,
        }

        if out == "":
            planning_result["failure_reason"] = "time limit reached"
            planning_result["plan"] = None
            planning_result["planning_time"] = time_limit

        elif out == "NO SOLUTION":
            planning_result["failure_reason"] = "no solution exists"
            planning_result["plan"] = None

        elif "std::bad_alloc" in out or "failed to map segment" in out:
            planning_result["failure_reason"] = "memory error"
            planning_result["plan"] = None

        elif set(out).issubset("UDLR"):
            puzzle = PushWorldPuzzle(puzzle_file_path)
            if puzzle.is_valid_plan([Actions.FROM_CHAR[ch] for ch in out]):
                planning_result["plan"] = out
            else:
                planning_result["failure_reason"] = "invalid plan"
                planning_result["plan"] = None

        else:
            planning_result["failure_reason"] = "unknown"
            planning_result["plan"] = None

        with open(planning_result_file_path, "w") as planning_result_file:
            yaml.dump(planning_result, planning_result_file)
