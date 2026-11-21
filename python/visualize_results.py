#!/usr/bin/env python3
"""Figures for the report -> results/graphs/. Usage: visualize_results.py [signals|benchmarks|all]"""
import subprocess
import sys
import tempfile
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "python"))
from analyze_results import ORDER, load_raw, summarize  # noqa: E402
from generate_sensor_data import generate  # noqa: E402

OUT = ROOT / "results" / "graphs"
CLI = ROOT / "build" / "sp_cli"
WARN, CRIT, WINDOW = 1.0, 2.0, 32
plt.rcParams.update({"figure.dpi": 130, "font.size": 10, "axes.grid": True, "grid.alpha": 0.3})


def cli(impl, cmd, path, *args):
    p = subprocess.run([str(CLI), impl, cmd, str(path), *map(str, args)],
                       capture_output=True, text=True, check=True)
    return dict(l.split("=", 1) for l in p.stdout.splitlines() if "=" in l)


def signals():
    OUT.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory() as tmp:
        tmp = Path(tmp)
        fs = 10_000.0
        x, _ = generate(samples=20_000, fs=fs, scenario="mixed", seed=7)
        p = tmp / "mixed.bin"
        x.tofile(p)
        t = np.arange(x.size) / fs

        # Fig 1: original signal (full record + zoom)
        fig, ax = plt.subplots(2, 1, figsize=(8, 5))
        ax[0].plot(t, x, lw=0.4)
        ax[0].set(title="Simulated vibration (mixed: wear + spikes)", ylabel="acceleration (a.u.)")
        ax[1].plot(t[:600], x[:600], lw=0.8)
        ax[1].set(title="Zoom: first 60 ms", xlabel="time (s)", ylabel="acceleration (a.u.)")
        fig.tight_layout(); fig.savefig(OUT / "fig01_original_signal.png"); plt.close(fig)

        # Fig 2: moving average computed by the ASM implementation
        out = tmp / "ma.bin"
        cli("asm", "movavg", p, WINDOW, out)
        y = np.fromfile(out, dtype="<f8")
        ty = t[WINDOW - 1:]                      # window ending at that sample
        a, b = 600, 1400
        fig, ax = plt.subplots(figsize=(8, 3.5))
        ax.plot(t[a:b], x[a:b], lw=0.6, alpha=0.6, label="original")
        ax.plot(ty[a - WINDOW + 1:b - WINDOW + 1], y[a - WINDOW + 1:b - WINDOW + 1],
                lw=1.4, label=f"moving average (w={WINDOW}, ASM)")
        ax.set(title="Noise reduction by moving average", xlabel="time (s)",
               ylabel="acceleration (a.u.)")
        ax.legend(); fig.tight_layout(); fig.savefig(OUT / "fig02_filtered_signal.png"); plt.close(fig)

        # Fig 3: RMS per scenario vs simulated thresholds
        names, rms = [], []
        for sc in ("normal", "wear", "spikes", "mixed"):
            xs, _ = generate(samples=100_000, scenario=sc, anomaly=3.0, seed=7)
            ps = tmp / f"{sc}.bin"
            xs.tofile(ps)
            names.append(sc); rms.append(float(cli("asm", "rms", ps, WARN, CRIT)["rms"]))
        fig, ax = plt.subplots(figsize=(6, 3.8))
        ax.bar(names, rms, color="#4c78a8")
        ax.axhline(WARN, color="orange", ls="--", label=f"warning ({WARN}) - simulated")
        ax.axhline(CRIT, color="red", ls="--", label=f"critical ({CRIT}) - simulated")
        ax.set(title="RMS by scenario (ASM implementation)", ylabel="RMS (a.u.)")
        ax.legend(); fig.tight_layout(); fig.savefig(OUT / "fig03_rms_comparison.png"); plt.close(fig)


def benchmarks():
    OUT.mkdir(parents=True, exist_ok=True)
    s = summarize(load_raw())
    algos = list(dict.fromkeys(s["algorithm"]))
    impls = [i for i in ORDER if i in set(s["impl"])]
    nmax = s["n"].max()

    # Fig 4: execution time at the largest size (median, error bars = IQR)
    fig, ax = plt.subplots(figsize=(8, 4))
    w = 0.8 / len(impls)
    for k, impl in enumerate(impls):
        d = s[(s.n == nmax) & (s.impl == impl)].set_index("algorithm").loc[algos]
        yerr = [d["median"] - d["q1"], d["q3"] - d["median"]]
        ax.bar(np.arange(len(algos)) + k * w, d["median"] / 1e6, w, yerr=[e / 1e6 for e in yerr],
               label=impl, capsize=2)
    ax.set_xticks(np.arange(len(algos)) + w * (len(impls) - 1) / 2, algos)
    ax.set(title=f"Execution time, n = {nmax:,} (median, IQR bars)", ylabel="ms per call")
    ax.legend(); fig.tight_layout(); fig.savefig(OUT / "fig04_execution_time.png"); plt.close(fig)

    # Fig 5: time vs input size
    fig, axes = plt.subplots(1, len(algos), figsize=(3.4 * len(algos), 3.6), sharey=False)
    for ax, algo in zip(np.atleast_1d(axes), algos):
        for impl in impls:
            d = s[(s.algorithm == algo) & (s.impl == impl)].sort_values("n")
            ax.plot(d["n"], d["median"], marker="o", label=impl)
        ax.set(xscale="log", yscale="log", title=algo, xlabel="samples")
    np.atleast_1d(axes)[0].set_ylabel("ns per call")
    np.atleast_1d(axes)[0].legend()
    fig.tight_layout(); fig.savefig(OUT / "fig05_time_vs_size.png"); plt.close(fig)

    # Fig 6: TSC ticks per sample (NOT core cycles; see docs/benchmark_methodology.md)
    fig, ax = plt.subplots(figsize=(8, 4))
    for k, impl in enumerate(impls):
        d = s[(s.n == nmax) & (s.impl == impl)].set_index("algorithm").loc[algos]
        ax.bar(np.arange(len(algos)) + k * w, d["tsc_per_sample"], w, label=impl)
    ax.set_xticks(np.arange(len(algos)) + w * (len(impls) - 1) / 2, algos)
    ax.set(title=f"TSC ticks per sample, n = {nmax:,}", ylabel="ticks / sample")
    ax.legend(); fig.tight_layout(); fig.savefig(OUT / "fig06_tsc_per_sample.png"); plt.close(fig)


if __name__ == "__main__":
    what = sys.argv[1] if len(sys.argv) > 1 else "all"
    if what in ("signals", "all"):
        signals()
    if what in ("benchmarks", "all"):
        benchmarks()
    print(f"graphs written to {OUT}")