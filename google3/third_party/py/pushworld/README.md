# PushWorld: A benchmark for planning with physical tools and movable obstacles

This repository contains the PushWorld environment, a collection of benchmark puzzles
to compare the performance of different solvers, and a state-of-the-art planner for
this environment.

Benchmark puzzles and reference solutions are available in the `benchmark` directory.
Puzzles are stored in a human-readable format that is convenient to edit in any
text editor with a fixed-width font. To interactively play any of the puzzles in this
benchmark, visit https://deepmind-pushworld.github.io/play/.

The `cpp` directory contains the code of the Recursive Graph Distance (RGD) planner,
which as of September 2022 is the state-of-the-art in solving the most puzzles within
1, 5, and 30 minutes. Implementing the RGD planner in *C++* ensures a fair comparison to
other classical planners used in our experiments. To build and run the RGD planner
(*run_planner*), follow the instructions in [cpp/README.md](cpp/README.md).

The `python3` directory contains a suite of scripts to benchmark the RGD planner,
plot videos of puzzle solutions, plot images of all puzzles, convert puzzles to PDDL,
and convert puzzles to SAS. To set up the Python environment required to run code in the `python3` directory, follow the instructions in
[python3/README.md](python3/README.md).
To support reinforcement learning research with PushWorld, it also contains wrappers compatible with the [OpenAI Gym environment](https://github.com/openai/gym) API and [DeepMind RL environment](https://github.com/deepmind/dm_env) API. 
To see how to use these, see [python3/scripts/demo_gym_env.py](python3/scripts/demo_gym_env.py) and  [python3/scripts/demo_dm_env.py](python3/scripts/demo_dm_env.py) respectively.
.

# Glossary

* PDDL: A predicate logic language for representing classical planning tasks.

* SAS: A state-variable representation for classical planning tasks.

* RGD: Recursive graph distance - fast distance computation on domain transition graph.

* OpenAI Gym: An environment for developing and testing learning agents.

* DeepMind env: An environment for developing and testing learning agents.
