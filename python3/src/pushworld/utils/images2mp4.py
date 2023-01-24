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
from typing import List

import numpy as np


def images2mp4(
    video_file_path: str,
    images: List[np.ndarray],
    color_axis: int = 2,
    fps: float = 30.0,
    min_video_size: int = 100,
) -> None:
    """Converts a list of images into an mp4 video.

    Args:
        video_file_path: The path of the .mp4 file where the video is saved.
        images: The list of RGB images to convert into a video. Must all have the same
            shape, which can either be (height, width, 3) or (3, height, width).
            This list must contain at least two images.
        color_axis: If 0, images must have shape (3, height, width). If 2, images
            must have shape (height, width, 3).
        fps: Frames per second in the generated video.
        min_video_size: In pixels, the minimum width or height of the generated
            video. Images are repeatedly upsampled by 2x until their dimensions
            exceed this value.
    """
    if color_axis not in [0, 2]:
        raise ValueError("color_axis must either be 0 or 2")

    if video_file_path[-4:].lower() != ".mp4":
        video_file_path += ".mp4"

    if len(images) < 2:
        raise ValueError("Cannot save a video with only %i frames" % len(images))

    if color_axis == 2:
        h, w, c = images[0].shape
    elif color_axis == 0:
        c, h, w = images[0].shape

    upsample = 1 + max(min_video_size // h, min_video_size // w)

    # Make sure the dimensions are divisible by 2.
    if (w % 2 == 1 or h % 2 == 1) and upsample % 2 == 1:
        upsample += 1

    w *= upsample
    h *= upsample
    size = (w, h)

    ffmpeg_command = (
        "ffmpeg",
        "-nostats",
        "-loglevel",
        "error",  # suppress warnings
        "-y",
        "-r",
        "%d" % fps,
        # input
        "-f",
        "rawvideo",
        "-s:v",
        "{}x{}".format(*size),
        "-pix_fmt",
        "rgb24",
        "-i",
        "-",
        # output
        "-vcodec",
        "libx264",
        "-pix_fmt",
        "yuv420p",
        video_file_path,
    )

    try:
        proc = subprocess.Popen(
            ffmpeg_command, stdin=subprocess.PIPE, preexec_fn=os.setsid
        )
    except FileNotFoundError as error:
        if error.filename == "ffmpeg":
            raise RuntimeError(
                "Converting images into an mp4 video requires `ffmpeg` to be "
                "installed. See https://ffmpeg.org/"
            )

    for frame in images:
        if color_axis == 0:
            # Transpose the color axis to the correct position
            frame = np.transpose(frame, [1, 2, 0])

        if frame.shape[2] != 3:
            raise ValueError(
                f"Expected 3 color channels, but an image has {frame.shape[2]} color "
                "channels."
            )

        frame = frame.repeat(upsample, axis=0).repeat(upsample, axis=1)
        proc.stdin.write(frame[:h, :w].tobytes())

    proc.stdin.close()
