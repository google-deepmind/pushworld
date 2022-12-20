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
from collections import defaultdict

import matplotlib.pyplot as plt
import numpy as np
import yaml

from pushworld.utils.filesystem import iter_files_with_extension


def plot_puzzles_solved_vs_time(
    planner_results_path: str,
    output_file_path: str = "puzzles_solved_vs_time.png",
    planning_timeout: int = 60 * 30,
) -> None:
    """Creates a plot of number of puzzles solved vs. planning time for each planner.
    The plot's right y-axis shows the percentage of puzzles solved.

    Args:
        planner_results_path: The path of a directory containing planner result files
            in `.yaml` format. Each file must contain the following top-level key-value
            pairs:
                planner: <name of the planner>
                puzzle: <name of the puzzle that the planner attempted to solve>
                plan: <a string of 'UDLR' characters, or `null` if no plan found>
                planning_time: <the time the planner spent searching for a plan>
        output_file_path: The path of the file in which to save the plot as a PNG image.
        planning_timeout: In seconds, the maximum time all planners were allowed to
            solve each puzzle. The plot is extended to include this as the last time on
            the x-axis.

    Raises:
        ValueError: If planners did not attempt to solve all of the same puzzles, or
            if there are multiple results for a planner on the same puzzle.
    """
    _, ax1 = plt.subplots()
    ax2 = ax1.twinx()

    # A map from planner names to lists of the times taken to solve puzzles.
    planning_times_per_planner = defaultdict(list)

    # A map from planner names to the puzzles they attempted to solve.
    puzzles_tried_per_planner = defaultdict(set)

    for result_file_path in iter_files_with_extension(planner_results_path, ".yaml"):
        with open(result_file_path, "r") as result_file:
            planning_result = yaml.safe_load(result_file)

        planner_name = planning_result["planner"]
        puzzle_name = planning_result["puzzle"]

        if puzzle_name in puzzles_tried_per_planner[planner_name]:
            raise ValueError(
                f'Planner "{planner_name}" has multiple results for the '
                f'"{puzzle_name}" puzzle'
            )

        puzzles_tried_per_planner[planner_name].add(puzzle_name)

        if planning_result["plan"] is not None:
            planning_times_per_planner[planning_result["planner"]].append(
                planning_result["planning_time"]
            )

    # Verify that all planners solved the same puzzles
    puzzles_tried_per_planner = list(puzzles_tried_per_planner.items())
    for planner_name, puzzles_tried in puzzles_tried_per_planner[1:]:
        if puzzles_tried != puzzles_tried_per_planner[0][1]:
            raise ValueError(
                f'Planners "{planner_name}" and "{puzzles_tried_per_planner[0][0]}" '
                "did not attempt the same puzzles."
            )

    for planner_name, puzzle_solve_times in sorted(planning_times_per_planner.items()):
        # X and Y values to plot
        x = sorted(puzzle_solve_times)
        y = list(range(1, 1 + len(x)))

        # Replace all 0s with the minimum reported time.
        for i, t in enumerate(x):
            if t > 0:
                break
        for j in range(i):
            x[j] = t

        # Extend all plots to the timeout
        x.append(planning_timeout)
        y.append(y[-1])

        ax1.plot(x, y, label=planner_name)

    num_puzzles = len(puzzles_tried_per_planner[0][1])
    max_solved_puzzles = max(map(len, planning_times_per_planner.values()))

    ax1.set_xscale("log")
    ax1.set_xlabel("Planning Time (seconds)")
    ax1.set_ylabel("Number of Puzzles Solved")
    mn, mx = ax1.set_ylim(0, max_solved_puzzles * 1.05)

    ax2.set_ylabel("% of Puzzles Solved")
    ax2.set_ylim(mn * 100 / num_puzzles, mx * 100 / num_puzzles)

    ax1.legend()
    plt.tight_layout()

    plt.savefig(output_file_path)
