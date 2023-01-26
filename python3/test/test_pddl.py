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
import pytest
import subprocess
import tempfile
from typing import List

from pushworld.config import (
    DOMAIN_SUFFIX,
    FAST_DOWNWARD_PATH,
    PROBLEM_SUFFIX,
    PUZZLE_EXTENSION,
)
from pushworld.pddl import puzzle_to_pddl
from pushworld.puzzle import Actions, PushWorldPuzzle


MISSING_PLANNER_EXECUTABLE = not os.path.exists(FAST_DOWNWARD_PATH)
SKIP_TEST_REASON = (
    "The Fast Downward executable was not found. "
    "You may need to update `FAST_DOWNWARD_PATH` in `src/pushworld/config.py`."
)


def run_fast_downward(
    domain_file_path: str, problem_file_path: str
) -> List[str] | None:
    """Runs the Fast Downward planner on the given PDDL domain + problem.

    Args:
        domain_file_path: The path of the PDDL domain file.
        problem_file_path: The path of the PDDL problem file.

    Returns:
        The resulting plan, stored as a list of PDDL actions, or None if a plan was
        not found.

    Raises:
        AssertionError: If Fast Downward detects that the plan does not achieve
            the problem goal.
    """
    plan_file_path = f"{tempfile.gettempdir()}/plan"

    proc = subprocess.Popen(
        [
            FAST_DOWNWARD_PATH,
            "--alias",
            "seq-sat-lama-2011",
            "--overall-time-limit",
            "100s",
            "--overall-memory-limit",
            "4G",
            "--plan-file",
            plan_file_path,
            domain_file_path,
            problem_file_path,
        ],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )

    stdout = proc.communicate()[0].decode("utf-8")

    if "Search stopped without finding a solution." in stdout:
        return None

    assert "Solution found." in stdout

    plan = []

    with open(f"{plan_file_path}.1", "r") as plan_file:
        for line in plan_file.readlines():
            if not line.startswith(";"):
                plan.append(line.strip())

    return plan


def check_pddl(puzzle_name: str, for_bfws: bool, solution_exists: bool) -> None:
    """Verifies that a planner is able to find a solution in the given puzzle whenever
    a solution exists.

    Args:
        puzzle_name: The name of a puzzle in the `puzzles` directory that is in the
            same directory as this file.
        for_bfws: Forwarded to `puzzle_to_pddl`.
        solution_exists: True iff the puzzle has at least one solution.

    Raises:
        AssertionError: If
            - a plan was found when a solution does not exist.
            - a plan was not found when a solution exists.
            - a discovered plan does not achieve the goal.
    """
    puzzle = PushWorldPuzzle(
        os.path.join(
            os.path.dirname(__file__), "puzzles", puzzle_name + PUZZLE_EXTENSION
        )
    )
    domain, problem = puzzle_to_pddl(puzzle_name, puzzle, for_bfws=for_bfws)

    tempdir = tempfile.gettempdir()
    domain_file_path = f"{tempdir}/{puzzle_name}{DOMAIN_SUFFIX}"
    problem_file_path = f"{tempdir}/{puzzle_name}{PROBLEM_SUFFIX}"

    with open(domain_file_path, "w") as domain_file:
        domain_file.write(domain)

    with open(problem_file_path, "w") as problem_file:
        problem_file.write(problem)

    plan = run_fast_downward(domain_file_path, problem_file_path)

    # A map from PDDL agent-movement actions to PushWorld actions.
    agent_action_map = {
        "(move-agent left)": Actions.LEFT,
        "(move-agent right)": Actions.RIGHT,
        "(move-agent up)": Actions.UP,
        "(move-agent down)": Actions.DOWN,
    }

    if solution_exists:
        assert plan is not None

        # Verify that the solution achieves the goal.
        state = puzzle.initial_state

        while len(plan) > 0:
            # This action should always be "move-agent".
            next_state = puzzle.get_next_state(state, agent_action_map[plan.pop(0)])

            # The next action should always be "push agent".
            assert plan.pop(0).startswith("(push agent")

            # The remaining actions should push the correct number of objects.
            num_moved_objects = sum(a != b for a, b in zip(state, next_state))
            for _ in range(num_moved_objects - 1):
                assert plan.pop(0).startswith("(push m")

            state = next_state

        assert puzzle.is_goal_state(state)

    else:
        assert plan is None


@pytest.mark.skipif(MISSING_PLANNER_EXECUTABLE, reason=SKIP_TEST_REASON)
def test_conversion() -> None:
    """Checks that `puzzle_to_pddl` generates a PDDL description that correctly
    represents the dynamics of the PushWorld environment.

    This test uses Fast Downward (https://www.fast-downward.org/) to find and
    validate plans on the generated PDDL descriptions.
    """
    puzzles_with_solutions = [
        "trivial",
        "trivial_tool",
        "multiple_goals",
        "trivial_obstacle",
        "transitive_pushing",
    ]
    puzzles_without_solutions = ["is_goal_state"]

    for for_bfws in [True, False]:
        for puzzle_name in puzzles_with_solutions:
            check_pddl(puzzle_name, for_bfws, solution_exists=True)
        for puzzle_name in puzzles_without_solutions:
            check_pddl(puzzle_name, for_bfws, solution_exists=False)
