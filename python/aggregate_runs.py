#!/usr/bin/env python3
"""Aggregate several benchmark CSVs: is asm vs C consistent across runs?
usage: aggregate_runs.py run1.csv run2.csv ..."""
import sys
from pathlib import Path

import pandas as pd

sys.path.insert(0, str(Path(__file__).resolve().parent))
from analyze_results import load_raw, summarize

frames = []
for i, path in enumerate(sys.argv[1:]):
    s = summarize(load_raw(path))[["algorithm", "n", "impl", "median"]]
    s["run"] = i
    frames.append(s)
d = pd.concat(frames)
p = d.pivot_table(index=["run", "algorithm", "n"], columns="impl", values="median")

rows = []
for (algo, n), g in p.groupby(level=["algorithm", "n"]):
    row = {"algorithm": algo, "n": n,
           "asm_ns_median": round(g["asm"].median()),
           "asm_spread": round(g["asm"].max() / g["asm"].min(), 2)}
    for ref in ("c_O2", "c_O3"):
        r = g[ref] / g["asm"]          # >1 means asm is faster
        row[f"vs_{ref[2:]}_median"] = round(r.median(), 3)
        row[f"vs_{ref[2:]}_range"] = f"{r.min():.2f}-{r.max():.2f}"
        row[f"vs_{ref[2:]}_asm_wins"] = f"{int((r > 1).sum())}/{len(r)}"
    rows.append(row)

pd.set_option("display.width", 220)
print(f"runs used: {len(sys.argv) - 1}")
print("asm_spread = slowest/fastest asm median across runs (1.0 = perfectly stable)")
print("vs_X: ratio of C time to asm time; >1 means asm faster; wins = runs where asm was faster\n")
print(pd.DataFrame(rows).to_string(index=False))