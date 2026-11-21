#!/usr/bin/env python3
"""Simulated motor vibration data (float64, little-endian .bin)."""
import argparse
import json
from pathlib import Path

import numpy as np

SCENARIOS = ("normal", "wear", "spikes", "mixed")


def generate(samples=100_000, fs=10_000.0, freq=50.0, noise=0.1,
             anomaly=2.0, scenario="mixed", seed=42):
    """Return (signal, metadata).

    normal : fundamental + 2nd harmonic + Gaussian noise
    wear   : amplitude ramps up linearly over the second half (x(1 + anomaly*u))
    spikes : sparse impulses of magnitude 2*anomaly, random sign
    mixed  : wear + spikes
    """
    if scenario not in SCENARIOS:
        raise ValueError(f"scenario must be one of {SCENARIOS}")
    rng = np.random.default_rng(seed)
    t = np.arange(samples) / fs
    base = np.sin(2 * np.pi * freq * t) + 0.3 * np.sin(2 * np.pi * 2 * freq * t + 0.5)

    envelope = np.ones(samples)
    if scenario in ("wear", "mixed"):
        start = samples // 2
        ramp = np.zeros(samples)
        ramp[start:] = np.linspace(0.0, 1.0, samples - start)
        envelope += anomaly * ramp

    x = envelope * base + rng.normal(0.0, noise, samples)

    n_spikes = 0
    if scenario in ("spikes", "mixed") and samples > 0:
        n_spikes = max(1, samples // 5000)
        pos = rng.choice(samples, size=n_spikes, replace=False)
        sign = rng.choice([-1.0, 1.0], size=n_spikes)
        x[pos] += sign * anomaly * 2.0

    meta = dict(samples=samples, fs=fs, freq=freq, noise=noise, anomaly=anomaly,
                scenario=scenario, seed=seed, n_spikes=int(n_spikes), dtype="<f8")
    return x.astype("<f8"), meta


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--samples", type=int, default=100_000)
    ap.add_argument("--fs", type=float, default=10_000.0, help="sampling rate (Hz)")
    ap.add_argument("--freq", type=float, default=50.0, help="fundamental frequency (Hz)")
    ap.add_argument("--noise", type=float, default=0.1, help="noise std deviation")
    ap.add_argument("--anomaly", type=float, default=2.0, help="anomaly intensity")
    ap.add_argument("--scenario", choices=SCENARIOS, default="mixed")
    ap.add_argument("--seed", type=int, default=42)
    ap.add_argument("--out", type=Path, default=None)
    a = ap.parse_args()

    x, meta = generate(a.samples, a.fs, a.freq, a.noise, a.anomaly, a.scenario, a.seed)
    out = a.out or Path("data/generated") / f"vibration_{a.scenario}_{a.samples}.bin"
    out.parent.mkdir(parents=True, exist_ok=True)
    x.tofile(out)
    out.with_suffix(".json").write_text(json.dumps(meta, indent=2))
    print(f"wrote {out} ({x.size} samples)")


if __name__ == "__main__":
    main()