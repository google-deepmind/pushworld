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
import numpy as np
import matplotlib
import matplotlib.pyplot as plt

from pushworld.puzzle import NUM_ACTIONS
from pushworld.gym_env import PushWorldEnv

from absl import app
from absl import flags

_PATH = flags.DEFINE_string('path', '', 'Puzzle file path.')

matplotlib.use('TkAgg')


def main(argv):
    # Create PushWorldEnv 
    env = PushWorldEnv(_PATH.value)

    # Reset the environment and show observation
    image, info = env.reset()
    plt.figure(figsize=(5, 5))
    plt.imshow(image)
    plt.ion()
    plt.show()

    # Randomly take 10 actions and show observation
    for _ in range(10):
        rets = env.step(np.random.randint(NUM_ACTIONS))
        image = rets[0]
        plt.imshow(image)
        plt.draw()
        plt.pause(0.5)


if __name__ == '__main__':
    app.run(main)
