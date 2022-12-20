Python PushWorld
----------------

To configure and install all dependencies, run the setup script from this directory:

```
./setup.sh
```

This script creates a virtual Python environment in a `venv` directory. To run Python
scripts and tests within this package, the virtual environment must be activated:

```
source venv/bin/activate
```

To run all unit tests, install [Fast Downward](https://github.com/aibasel/downward),
update the `FAST_DOWNWARD_PATH` in `src/pushworld/config.py`, and run:

```
pytest test
```

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
