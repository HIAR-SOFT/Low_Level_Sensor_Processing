#!/usr/bin/env python3
"""Convert IMS bearing text files into a float64 .bin for the C/ASM code.

Files are tab-separated, one row per sample, one column per channel.
Consecutive files (10 min apart) are concatenated in name order, so the signal
has a time gap every 20,480 samples. That does not matter for benchmarking.
"""
import argparse
import json
from pathlib import Path

import numpy as np


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("folder", type=Path, help="folder with IMS files (e.g. 2nd_test)")
    ap.add_argument("--channel", type=int, default=0, help="column index (0-based)")
    ap.add_argument("--samples", type=int, default=1_000_000)
    ap.add_argument("--start", type=int, default=0, help="index of first file to use")
    ap.add_argument("--out", type=Path, default=None)
    a = ap.parse_args()

    files = sorted(p for p in a.folder.iterdir() if p.is_file())[a.start:]
    if not files:
        raise SystemExit("no files found")
    parts, total, used = [], 0, 0
    for p in files:
        col = np.loadtxt(p, delimiter="\t", usecols=a.channel)
        parts.append(col)
        total += col.size
        used += 1
        if total >= a.samples:
            break
    x = np.concatenate(parts)[: a.samples].astype("<f8")
    if x.size < a.samples:
        print(f"warning: only {x.size} samples available")

    out = a.out or Path("data/generated") / f"ims_ch{a.channel}_start{a.start}_{x.size}.bin"
    out.parent.mkdir(parents=True, exist_ok=True)
    x.tofile(out)
    meta = dict(source=str(a.folder), channel=a.channel, start_file=a.start,
                files_used=used, first_file=files[0].name, samples=int(x.size), dtype="<f8")
    out.with_suffix(".json").write_text(json.dumps(meta, indent=2))
    print(f"wrote {out}: {x.size} samples from {used} files "
          f"(min={x.min():.4f} max={x.max():.4f} rms={np.sqrt(np.mean(x**2)):.4f})")


if __name__ == "__main__":
    main()