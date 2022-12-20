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

# The following assumes gym.__version__ == '0.19.0'

import os

import numpy as np
import pytest

from pushworld.gym_env import PushWorldEnv
from pushworld.puzzle import Actions

TEST_PUZZLES_PATH = os.path.join(os.path.split(__file__)[0], "puzzles")


def test_observations_and_renderings():
    """Checks that `PushWorldEnv.reset` and `step` return correct observations,
    and checks that these observations are consistent with `render`."""
    puzzle_file_path = os.path.join(TEST_PUZZLES_PATH, "trivial.pwp")
    env = PushWorldEnv(puzzle_file_path)

    observation, info = env.reset()
    assert observation in env.observation_space
    image = env.current_puzzle.render(info["puzzle_state"])
    assert (image == observation * 255).all()
    assert (image == env.render()).all()

    observation, _, _, _, info = env.step(Actions.RIGHT)
    assert observation in env.observation_space
    image = env.current_puzzle.render(info["puzzle_state"])
    assert (image == observation * 255).all()
    assert (image == env.render()).all()


def test_standard_padding():
    """Checks setting `standard_padding = True` in a `PushWorldEnv`."""
    puzzle_file_path = os.path.join(TEST_PUZZLES_PATH, "trivial.pwp")

    env = PushWorldEnv(puzzle_file_path, standard_padding=False)
    o1, _ = env.reset()
    assert o1 in env.observation_space
    o1 = o1.sum(axis=0)
    nz1 = np.count_nonzero(o1)

    env = PushWorldEnv(puzzle_file_path, standard_padding=True)
    o2, _ = env.reset()
    assert o2 in env.observation_space
    o2 = o2.sum(axis=0)
    nz2 = np.count_nonzero(o2)

    assert o1.shape != o2.shape
    assert nz1 == nz2


def test_reward():
    """Checks that `PushWorldEnv.step` returns expected rewards."""
    puzzle_file_path = os.path.join(TEST_PUZZLES_PATH, "multiple_goals.pwp")
    cost_per_step = -0.01

    env = PushWorldEnv(puzzle_file_path)
    env.reset()
    assert env.step(Actions.RIGHT)[1] == cost_per_step
    assert env.step(Actions.RIGHT)[1] == 1 + cost_per_step
    assert env.step(Actions.RIGHT)[1] == -1 + cost_per_step

    env.reset()
    env.step(Actions.RIGHT)
    env.step(Actions.RIGHT)
    env.step(Actions.LEFT)
    env.step(Actions.LEFT)
    env.step(Actions.LEFT)
    assert env.step(Actions.LEFT)[1] == 10


@pytest.mark.parametrize("standard_padding", [True, False])
def test_all_goals_achieved(standard_padding: bool):
    """Checks that `PushWorldEnv.step` returns expected values when all goals in the
    puzzle are achieved."""
    puzzle_file_path = os.path.join(TEST_PUZZLES_PATH, "trivial.pwp")
    env = PushWorldEnv(puzzle_file_path, standard_padding=standard_padding)
    env.reset()
    env.step(Actions.RIGHT)
    env.step(Actions.DOWN)
    env.step(Actions.RIGHT)
    obs, reward, terminated, truncated, _ = env.step(Actions.UP)

    assert obs in env.observation_space
    assert reward == 10
    assert terminated == True
    assert truncated == False


def test_truncation():
    """Checks that `PushWorldEnv.step` correctly returns whether an episode is
    truncated by reaching the `max_steps` limit."""
    puzzle_file_path = os.path.join(TEST_PUZZLES_PATH, "transitive_pushing.pwp")
    env = PushWorldEnv(puzzle_file_path, max_steps=3)
    env.reset()
    assert env.step(Actions.LEFT)[3] == False
    assert env.step(Actions.LEFT)[3] == False
    assert env.step(Actions.LEFT)[3] == True


def test_termination():
    """Checks that `PushWorldEnv.step` currectly returns whether an episode is
    terminated by achieving all goals in the puzzle."""
    puzzle_file_path = os.path.join(TEST_PUZZLES_PATH, "transitive_pushing.pwp")
    env = PushWorldEnv(puzzle_file_path)
    env.reset()
    env.step(Actions.RIGHT)
    env.step(Actions.RIGHT)
    _, reward, terminated, truncated, _ = env.step(Actions.RIGHT)

    assert reward == 10
    assert terminated == True
    assert truncated == False

    puzzle_file_path = os.path.join(TEST_PUZZLES_PATH, "multiple_goals.pwp")
    env = PushWorldEnv(puzzle_file_path)

    for action in [Actions.LEFT, Actions.RIGHT]:
        env.reset()
        env.step(action)
        _, reward, terminated, _, _ = env.step(action)

        # Goal achieved, but not terminated because another goal remains.
        assert reward > 0
        assert terminated == False

    env.reset()
    env.step(Actions.RIGHT)
    env.step(Actions.RIGHT)
    env.step(Actions.LEFT)
    env.step(Actions.LEFT)
    env.step(Actions.LEFT)
    _, reward, terminated, _, _ = env.step(Actions.LEFT)

    # Both goals achieved.
    assert reward == 10
    assert terminated == True


def test_reset():
    """Checks `PushWorldEnv.reset`."""
    puzzle_file_path = os.path.join(TEST_PUZZLES_PATH, "trivial.pwp")
    env = PushWorldEnv(puzzle_file_path)

    o1, _ = env.reset()
    o2 = env.step(Actions.RIGHT)[0]
    o3, _ = env.reset()
    assert not (o1 == o2).all()
    assert (o1 == o3).all()
    assert o1 in env.observation_space

    env = PushWorldEnv(TEST_PUZZLES_PATH)
    initial_states = set(tuple(env.reset()[0].flat) for _ in range(100))
    assert len(initial_states) > 1
