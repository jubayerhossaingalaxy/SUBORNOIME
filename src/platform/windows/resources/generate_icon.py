#!/usr/bin/env python3
"""Generate Suborno's multi-resolution Windows icon using only Python's stdlib."""

import math
import struct
import sys
import zlib
from pathlib import Path


def png_chunk(kind: bytes, data: bytes) -> bytes:
    payload = kind + data
    return struct.pack(">I", len(data)) + payload + struct.pack(">I", zlib.crc32(payload))


def render_png(size: int) -> bytes:
    rows = bytearray()
    scale = 4
    for y in range(size):
        rows.append(0)
        for x in range(size):
            samples = []
            for sy in range(scale):
                for sx in range(scale):
                    px = (x + (sx + 0.5) / scale) / size
                    py = (y + (sy + 0.5) / scale) / size
                    edge = min(px, py, 1 - px, 1 - py)
                    radius = 0.22
                    if edge < radius:
                        cx = radius if px < radius else 1 - radius
                        cy = radius if py < radius else 1 - radius
                        if math.hypot(px - cx, py - cy) > radius:
                            samples.append((0, 0, 0, 0))
                            continue

                    background = (22, 54, 82, 255)
                    # A clean, geometric S mark and a warm gold accent.
                    path = [
                        (0.66, 0.29),
                        (0.57, 0.23),
                        (0.43, 0.23),
                        (0.33, 0.30),
                        (0.32, 0.39),
                        (0.40, 0.46),
                        (0.60, 0.54),
                        (0.68, 0.61),
                        (0.67, 0.70),
                        (0.57, 0.77),
                        (0.42, 0.77),
                        (0.32, 0.70),
                    ]
                    mark = False
                    thickness = max(0.035, 2.2 / size)
                    for start, end in zip(path, path[1:]):
                        dx, dy = end[0] - start[0], end[1] - start[1]
                        length2 = dx * dx + dy * dy
                        t = ((px - start[0]) * dx + (py - start[1]) * dy) / length2
                        t = min(1.0, max(0.0, t))
                        if math.hypot(px - start[0] - t * dx, py - start[1] - t * dy) < thickness:
                            mark = True
                            break
                    if mark:
                        samples.append((246, 248, 250, 255))
                    elif math.hypot(px - 0.72, py - 0.27) < 0.045:
                        samples.append((255, 195, 72, 255))
                    else:
                        samples.append(background)

            rows.extend(
                tuple(sum(sample[channel] for sample in samples) // len(samples) for channel in range(4))
            )

    header = struct.pack(">IIBBBBB", size, size, 8, 6, 0, 0, 0)
    return (
        b"\x89PNG\r\n\x1a\n"
        + png_chunk(b"IHDR", header)
        + png_chunk(b"IDAT", zlib.compress(bytes(rows), 9))
        + png_chunk(b"IEND", b"")
    )


def main() -> None:
    if len(sys.argv) != 2:
        raise SystemExit("usage: generate_icon.py OUTPUT.ico")

    sizes = (16, 24, 32, 48, 64, 128, 256)
    images = [render_png(size) for size in sizes]
    directory_size = 6 + 16 * len(images)
    entries = bytearray()
    offset = directory_size
    for size, image in zip(sizes, images):
        entries.extend(
            struct.pack(
                "<BBBBHHII",
                0 if size == 256 else size,
                0 if size == 256 else size,
                0,
                0,
                1,
                32,
                len(image),
                offset,
            )
        )
        offset += len(image)

    output = struct.pack("<HHH", 0, 1, len(images)) + entries + b"".join(images)
    destination = Path(sys.argv[1])
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_bytes(output)


if __name__ == "__main__":
    main()
