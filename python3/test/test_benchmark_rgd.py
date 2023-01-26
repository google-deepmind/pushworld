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
import tempfile
import pytest
from typing import Any, Dict

import yaml

from pushworld.benchmark_rgd import benchmark_rgd_planner
from pushworld.config import (
    BENCHMARK_PUZZLES_PATH, PUZZLE_EXTENSION, RGD_PLANNER_PATH
)
from pushworld.puzzle import Actions, PushWorldPuzzle


MISSING_PLANNER_EXECUTABLE = not os.path.exists(RGD_PLANNER_PATH)
SKIP_TEST_REASON = (
    "The RGD planner executable was not found. "
    "You may need to update `RGD_PLANNER_PATH` in `src/pushworld/config.py`."
)


def _benchmark_puzzle(puzzle_file_path: str, **kwargs) -> Dict[str, Any]:
    """Runs `benchmark_rgd_planner` on the given puzzle.

    Args:
        puzzle_file_path: The path of the PushWorld puzzle file to benchmark.
        **kwargs: Forwarded to `benchmark_rgd_planner`.

    Returns:
        A dictionary containing the planning result. See `benchmark_rgd_planner`
        for the contents of this result.
    """
    tempdir = tempfile.gettempdir()

    benchmark_rgd_planner(
        results_path=tempdir,
        puzzles_path=puzzle_file_path,
        heuristic="N+RGD",
        **kwargs,
    )
    puzzle_filename = os.path.split(puzzle_file_path)[1]
    planning_result_filename = os.path.splitext(puzzle_filename)[0] + ".yaml"
    planning_result_file_path = os.path.join(tempdir, planning_result_filename)

    with open(planning_result_file_path, "r") as planning_result_file:
        return yaml.safe_load(planning_result_file)


@pytest.mark.skipif(MISSING_PLANNER_EXECUTABLE, reason=SKIP_TEST_REASON)
def test_success():
    """Verifies that a plan is returned when a puzzle is solved."""
    puzzle_name = "Pull Dont Push"
    puzzle_file_path = os.path.join(
        BENCHMARK_PUZZLES_PATH, "level2", puzzle_name + PUZZLE_EXTENSION
    )

    result = _benchmark_puzzle(
        puzzle_file_path, time_limit=None, memory_limit=None
    )

    puzzle = PushWorldPuzzle(puzzle_file_path)
    plan = [Actions.FROM_CHAR[s] for s in result["plan"].upper()]

    assert puzzle.is_valid_plan(plan)
    assert result["puzzle"] == puzzle_name
    assert result["planner"] == "Novelty+RGD"
    assert result["planning_time"] > 0
    assert result.get("failure_reason", None) is None


@pytest.mark.skipif(MISSING_PLANNER_EXECUTABLE, reason=SKIP_TEST_REASON)
def test_time_limit():
    """Verifies that `benchmark_rgd_planner` detects when a puzzle reaches the
    time limit.
    """
    time_limit = 1  # seconds

    result = _benchmark_puzzle(
        os.path.join(
            BENCHMARK_PUZZLES_PATH, "level4", "Four Pistons" + PUZZLE_EXTENSION
        ),
        time_limit=time_limit,
        memory_limit=None,
    )

    assert result["plan"] is None
    assert result["failure_reason"] == "time limit reached"
    assert result["planning_time"] == time_limit


@pytest.mark.skipif(MISSING_PLANNER_EXECUTABLE, reason=SKIP_TEST_REASON)
def test_memory_limit():
    """Verifies that `benchmark_rgd_planner` detects when a puzzle reaches the
    memory limit.
    """
    result = _benchmark_puzzle(
        os.path.join(
            BENCHMARK_PUZZLES_PATH, "level2", "Pull Dont Push" + PUZZLE_EXTENSION
        ),
        time_limit=None,
        memory_limit=0.001,  # gigabytes
    )
    assert result["plan"] is None
    assert result["failure_reason"] == "memory error"
