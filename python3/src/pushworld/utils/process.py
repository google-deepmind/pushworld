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

import resource
import subprocess
import sys
from typing import Callable, Optional, Tuple

# These definitions are standard since 1998. (https://en.wikipedia.org/wiki/Gigabyte)
KILOBYTE = 1000
MEGABYTE = 1000 * KILOBYTE
GIGABYTE = 1000 * MEGABYTE


def get_child_process_cpu_time() -> float:
    """Returns the total CPU time in seconds used by child processes of this process."""
    child_times = resource.getrusage(resource.RUSAGE_CHILDREN)
    return child_times.ru_utime + child_times.ru_stime


def run_process(
    command: list[str],
    time_limit: Optional[int] = None,
    memory_limit: Optional[int] = None,
) -> Tuple[str, int, float]:
    """Runs a child process specified by the given command.

    Args:
        command: A list of strings of command-line arguments.
        time_limit: In seconds, the maximum time the process is allowed to run.
            If `None`, no time limit is enforced.
        memory_limit: In bytes, the maximum memory the process is allowed to use.
            If `None`, no memory limit is enforced.

    Returns:
        A tuple of (stdout, return code, CPU run time) of the child process.
        The measurement of CPU time assumes that no other child processes are
        running in parallel.
    """
    if not isinstance(time_limit, (int, type(None))):
        raise TypeError("time_limit must be an integer or None")
    if time_limit is not None and time_limit <= 0:
        raise ValueError("time_limit must be a positive integer")

    if not isinstance(memory_limit, (int, type(None))):
        raise TypeError("memory_limit must be an integer or None")
    if memory_limit is not None and memory_limit <= 0:
        raise ValueError("memory_limit must be a positive integer")

    def preexec_fn():
        if time_limit is not None:
            # See `https://github.com/aibasel/downward/blob/main/driver/limits.py` for
            # an explanation of this `try..except`.
            try:
                resource.setrlimit(resource.RLIMIT_CPU, (time_limit, time_limit + 1))
            except ValueError:
                resource.setrlimit(resource.RLIMIT_CPU, (time_limit, time_limit))

        if memory_limit is not None:
            resource.setrlimit(resource.RLIMIT_AS, (memory_limit, memory_limit))

    start_cpu_time = get_child_process_cpu_time()
    proc = subprocess.Popen(
        command,
        preexec_fn=preexec_fn,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    out = proc.communicate()[0]  # this waits for the process to terminate
    cpu_run_time = get_child_process_cpu_time() - start_cpu_time

    out = out.strip().decode("utf-8")
    return out, proc.returncode, cpu_run_time
