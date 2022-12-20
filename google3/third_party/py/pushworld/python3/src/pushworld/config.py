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

"""
This file contains configurable file extensions, data locations, and executable paths.
"""
import os

MODULE_PATH = os.path.split(__file__)[0]

DOMAIN_SUFFIX = "-domain.pddl"
PROBLEM_SUFFIX = "-problem.pddl"
PUZZLE_EXTENSION = ".pwp"

# Data paths
BENCHMARK_PATH = os.path.join(MODULE_PATH, "../../../benchmark")
BENCHMARK_PUZZLES_PATH = os.path.join(BENCHMARK_PATH, "puzzles")
BENCHMARK_SOLUTIONS_PATH = os.path.join(BENCHMARK_PATH, "solutions")

# Paths to planner executables
RGD_PLANNER_PATH = os.path.join(MODULE_PATH, "../../../cpp/build/bin/run_planner")
FAST_DOWNWARD_PATH = os.path.join(MODULE_PATH, "../../../../downward/fast-downward.py")
