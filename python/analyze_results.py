#!/usr/bin/env python3
"""Summarise the newest results/raw/bench_*.csv."""
import sys
from pathlib import Path

import pandas as pd

ROOT = Path(__file__).resolve().parents[1]
ORDER = ["c_O0", "c_O2", "c_O3", "asm"]


def latest_csv():
    files = sorted((ROOT / "results" / "raw").glob("bench_*.csv"))
    if not files:
        sys.exit("no results/raw/bench_*.csv found -- run: make bench")
    return files[-1]


def load_raw(path=None):
    return pd.read_csv(path or latest_csv())


def summarize(df):
    g = df.groupby(["algorithm", "n", "impl"])["ns_per_call"]
    s = g.agg(median="median", q1=lambda v: v.quantile(0.25),
              q3=lambda v: v.quantile(0.75), std="std", min="min", runs="count").reset_index()
    s["iqr"] = s["q3"] - s["q1"]
    s["cv_pct"] = 100 * s["std"] / g.mean().values
    s["msamples_per_s"] = s["n"] / s["median"] * 1e3
    tsc = df.groupby(["algorithm", "n", "impl"])["tsc_per_call"].median().values
    s["tsc_per_sample"] = tsc / s["n"]
    return s


def speedups(s):
    p = s.pivot_table(index=["algorithm", "n"], columns="impl", values="median")
    out = pd.DataFrame(index=p.index)
    out["O2_vs_O0"] = p["c_O0"] / p["c_O2"]
    out["asm_vs_O0"] = p["c_O0"] / p["asm"]
    out["asm_vs_O2"] = p["c_O2"] / p["asm"]     # >1 means handwritten ASM is faster than -O2
    return out


if __name__ == "__main__":
    path = Path(sys.argv[1]) if len(sys.argv) > 1 else latest_csv()
    df = load_raw(path)
    s = summarize(df)
    s.to_csv(ROOT / "results" / "summary.csv", index=False)
    pd.set_option("display.width", 200)
    print(f"source: {path.name}")
    print(s.round(3).to_string(index=False))
    print("\nSpeedups (ratio of median times):")
    print(speedups(s).round(3).to_string())
