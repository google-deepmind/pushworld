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
import subprocess

from pushworld.config import DOMAIN_SUFFIX, FAST_DOWNWARD_PATH, PROBLEM_SUFFIX


def pddl_to_sas(
    domain_file_path: str,
    problem_file_path: str,
    sas_file_path: str,
    fast_downward_executable: str = FAST_DOWNWARD_PATH,
) -> None:
    """Runs the Fast Downward (https://www.fast-downward.org/) translator on
    the given PDDL domain and problem files to convert them into an SAS file.

    Args:
        domain_file_path: The path of the PDDL domain file.
        problem_file_path: The path of the PDDL problem file.
        sas_file_path: The path of the written SAS file.
        fast_downward_executable: The path of the `fast-downward.py` executable.
    """
    proc = subprocess.Popen(
        [fast_downward_executable, "--translate"]
        + ["--sas-file", sas_file_path]
        + [domain_file_path, problem_file_path],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    out = proc.communicate()[0].decode("utf-8")
    if "translate exit code: 0" not in out:
        raise RuntimeError(
            f"Failed to convert {domain_file_path} and {problem_file_path} "
            "into SAS format."
        )


def convert_all_pddls_to_sas(
    pddl_path: str,
    fast_downward_executable: str = FAST_DOWNWARD_PATH,
    domain_suffix: str = DOMAIN_SUFFIX,
    problem_suffix: str = PROBLEM_SUFFIX,
) -> None:
    """Iterates through the directory of PDDL files and converts them into SAS files.

    This function uses the Fast Downward (https://www.fast-downward.org/) translator
    to convert PDDL into SAS format.

    Args:
        pddl_path: The path of a directory that contains pairs of PDDL domains
            and problems. PDDL files can be nested in subdirectories, but pairs must
            occur in the same subdirectory. For each detected pair, a SAS file is
            written in the same directory.
        fast_downward_executable: The path of the `fast-downward.py` executable.
        domain_suffix: The suffix of PDDL domain files.
        problem_suffix: The suffix of PDDL problem files.
    """
    for pddl_subdir, _, filenames in os.walk(pddl_path):
        for filename in filenames:
            if filename.endswith(domain_suffix):
                domain_file_path = os.path.join(pddl_subdir, filename)
                puzzle_name = domain_file_path[: -len(domain_suffix)]
                problem_file_path = puzzle_name + problem_suffix
                sas_file_path = puzzle_name + ".sas"

                pddl_to_sas(
                    domain_file_path=domain_file_path,
                    problem_file_path=problem_file_path,
                    sas_file_path=sas_file_path,
                    fast_downward_executable=fast_downward_executable,
                )
