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

import random
from typing import Optional


import dm_env
from dm_env import specs
import numpy as np

from pushworld.config import PUZZLE_EXTENSION
from pushworld.puzzle import (
    DEFAULT_BORDER_WIDTH,
    DEFAULT_PIXELS_PER_CELL,
    NUM_ACTIONS,
    PushWorldPuzzle,
    State,
)
from pushworld.utils.env_utils import get_max_puzzle_dimensions, render_observation_padded
from pushworld.utils.filesystem import iter_files_with_extension


class PushWorldEnv(dm_env.Environment):
    """A dm_env environment for PushWorld puzzles.

    Rewards are calculated according to Appendix D of
    https://arxiv.org/pdf/1707.06203.pdf with one change: The negative reward
    per step is reduced to 0.01, since PushWorld puzzles tend to have longer
    solutions than Sokoban puzzles.

    Args:
        puzzle_path: The path of a PushWorld puzzle file or of a directory that
          contains puzzle files, possibly nested in subdirectories. All discovered
          puzzles are loaded, and the `reset` method randomly selects a new puzzle
          each time it is called.
        max_steps: If not None, the `step` method will return step type dm_env.StepType.LAST
          after calling it `max_steps` times since the most recent call of `reset`.
        border_width: The pixel width of the border drawn to indicate object
          boundaries. Must be >= 1.
        pixels_per_cell: The pixel width and height of a discrete position in the
          environment. Must be >= 1 + 2 * border_width.
        standard_padding: If True, all puzzles are padded to the maximum width and
          height of the puzzles in the `pushworld.config.BENCHMARK_PUZZLES_PATH`
          directory. If False, puzzles are padded to the maximum dimensions of all
          puzzles found in the `puzzle_path`.
    """

    def __init__(
        self,
        puzzle_path: str,
        max_steps: Optional[int] = None,
        border_width: int = DEFAULT_BORDER_WIDTH,
        pixels_per_cell: int = DEFAULT_PIXELS_PER_CELL,
        standard_padding: bool = False,
    ) -> None:
        self._puzzles = []
        for puzzle_file_path in iter_files_with_extension(
            puzzle_path, PUZZLE_EXTENSION
        ):
          self._puzzles.append(PushWorldPuzzle(puzzle_file_path))

        if len(self._puzzles) == 0:
          raise ValueError(f"No PushWorld puzzles found in: {puzzle_path}")
        if border_width < 1:
          raise ValueError("border_width must be >= 1")
        if pixels_per_cell < 3:
          raise ValueError("pixels_per_cell must be >= 3")

        self._max_steps = max_steps
        self._pixels_per_cell = pixels_per_cell
        self._border_width = border_width

        widths, heights = zip(*[puzzle.dimensions for puzzle in self._puzzles])
        self._max_cell_width = max(widths)
        self._max_cell_height = max(heights)

        if standard_padding:
          standard_cell_height, standard_cell_width = get_max_puzzle_dimensions()

          if standard_cell_height < self._max_cell_height:
            raise ValueError(
                "`standard_padding` is True, but the maximum puzzle height in "
                "BENCHMARK_PUZZLES_PATH is less than the height of the puzzle(s) "
                "in the given `puzzle_path`."
            )
          else:
            self._max_cell_height = standard_cell_height

          if standard_cell_width < self._max_cell_width:
            raise ValueError(
                "`standard_padding` is True, but the maximum puzzle width in "
                "BENCHMARK_PUZZLES_PATH is less than the width of the puzzle(s) "
                "in the given `puzzle_path`."
            )
          else:
            self._max_cell_width = standard_cell_width

        # Use a fixed arbitrary seed for reproducibility of results and for
        # deterministic tests.
        self._random_generator = random.Random(123)

        self._current_puzzle = None
        self._current_state = None

        self._action_space = specs.DiscreteArray(
            num_values=NUM_ACTIONS, dtype=int, name="action"
        )

        self._observation_space = specs.BoundedArray(
            shape=render_observation_padded(
                self._puzzles[0], self._puzzles[0].initial_state, self._max_cell_height, self._max_cell_width, self._pixels_per_cell, self._border_width,
            ).shape,
            dtype=np.float32,
            name="board",
            minimum=0.0,
            maximum=1.0,
        )

    def observation_spec(self) -> specs.BoundedArray:
        """Implements `dm_env.Environment.observation_spec`."""
        return self._observation_space

    def action_spec(self) -> specs.DiscreteArray:
        """Implements `dm_env.Environment.action_spec`."""
        return self._action_space

    @property
    def current_puzzle(self) -> PushWorldPuzzle or None:
        """The current puzzle, or `None` if `reset` has not yet been called."""
        return self._current_puzzle

    @property
    def current_state(self) -> State or None:
        """The current state, or `None` if the environment has just been initialized.
        """
        return self._current_state

    def reset(
        self,
        seed: Optional[int] = None,
    ) -> dm_env.TimeStep:
        """Implements `dm_env.Environment.reset`.

        This function randomly selects a puzzle from those provided to the
        constructor and resets the environment to the initial state of the puzzle.

        Args:
            seed: If not None, the random number generator in this environment is
              reset with this seed.

        Returns:
            A tuple of (observation, info). The observation contains the initial
            observation of the environment after the reset, and it is formatted as
            an RGB image with shape (height, width, 3) with `float32` type and
            values ranging from [0, 1]. The info dictionary is unused.
        """
        if seed is not None:
          self._random_generator = random.Random(seed)

        self._current_puzzle = self._random_generator.choice(self._puzzles)
        self._current_state = self._current_puzzle.initial_state
        self._current_achieved_goals = self._current_puzzle.count_achieved_goals(
            self._current_state
        )
        self._steps = 0

        observation = render_observation_padded(
            self._current_puzzle, self._current_state, self._max_cell_height, self._max_cell_width, self._pixels_per_cell, self._border_width,
        )

        return dm_env.restart(observation)

    def step(self, action: int) -> dm_env.TimeStep:
        """Implements `dm_env.Environment.step`.

        Args:
          action: Index corresponding to the categorical action

        Returns:
          TimeStep object bundling the reward, discount and observation. The returned observation is an RGB image of the new state of the
          environment, formatted as a `float32` array with shape (height, width, 3)
          and values ranging from [0, 1].
        """
        try:
          self._action_space.validate(action)
        except ValueError:
          raise ValueError("The provided action is not in the action space.")

        if self._current_state is None:
          raise RuntimeError("reset() must be called before step() can be called.")

        self._steps += 1
        previous_state = self._current_state
        self._current_state = self._current_puzzle.get_next_state(
            self._current_state, action
        )
        observation = render_observation_padded(
            self._current_puzzle, self._current_state, self._max_cell_height, self._max_cell_width, self._pixels_per_cell, self._border_width,
        )

        terminated = self._current_puzzle.is_goal_state(self._current_state)

        if terminated:
          reward = 10.0
        else:
          previous_achieved_goals = self._current_puzzle.count_achieved_goals(
              previous_state
          )
          current_achieved_goals = self._current_puzzle.count_achieved_goals(
              self._current_state
          )
          reward = current_achieved_goals - previous_achieved_goals - 0.01

        truncated = (
            False if self._max_steps is None else self._steps >= self._max_steps
        )

        done = terminated or truncated
        if done:
          return dm_env.termination(reward, observation)
        else:
          return dm_env.transition(reward, observation)

    def render(self, mode="rgb_array") -> np.ndarray:
        """Render an observation from the environment state.

        Returns:
            An RGB image of the current state of the environment, formatted as a
            `np.float32` array with shape (height, width, 3).
        """
        assert mode == "rgb_array", "mode must be rgb_array."
        return (
            self._current_puzzle.render(
                self._current_state,
                border_width=self._border_width,
                pixels_per_cell=self._pixels_per_cell,
            ).astype(np.float32)
            / 255
        )

