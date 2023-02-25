Python PushWorld
----------------

Prerequisites:
- Python >= v3.10 must be installed.
- Python3-venv must be installed.

To configure and install all dependencies, run the setup script from this directory:

```
./setup.sh
```

This script creates a virtual Python environment in a `venv` directory. To run Python
scripts and tests within this package, the virtual environment must be activated:

```
source venv/bin/activate
```

To run all unit tests:

```
pytest test
```

Some functions depend on [Fast Downward](https://github.com/aibasel/downward).
Once installed, you may need to update the `FAST_DOWNWARD_PATH` in
`src/pushworld/config.py`.

The RGD benchmarking functions require building the RGD planner in the [../cpp](../cpp)
directory of this repository. For build instructions, see [../cpp/README.md](../cpp/README.md).

## Scripts

The `scripts` directory contains a variety of executable scripts, each of which can
be run with the `--help` option to display usage instructions.

To render images of all benchmark puzzles:

```
./scripts/render_puzzle_previews.py --image_path=puzzle_images
```

To render videos of the solutions to all puzzles, install [ffmpeg](https://ffmpeg.org/)
and run:

```
./scripts/render_plans.py --planning_results_path=../benchmark/solutions --video_path=puzzle_solutions
```

To convert all puzzles to PDDL:

```
./scripts/convert_to_pddl.py --pddl_path=pddl_puzzles
```

To generate level 0 puzzles (use --help flag for puzzle options):

```
./scripts/generate_level0_puzzles.py --save_location_path=training_puzzles --num_puzzles=100
```
