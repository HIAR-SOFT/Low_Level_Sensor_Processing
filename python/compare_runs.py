#!/usr/bin/env python3
"""Compare two benchmark CSVs: run-to-run drift and asm vs O3."""
import sys
from pathlib import Path
import pandas as pd

sys.path.insert(0, str(Path(__file__).resolve().parent))
from analyze_results import load_raw, summarize

a, b = (summarize(load_raw(p)).set_index(["algorithm", "n", "impl"])["median"]
        for p in sys.argv[1:3])
pd.set_option("display.width", 200)
drift = (b / a).rename("ratio").reset_index()
print("Median time, run2 / run1 (1.0 = identical):")
print(drift.pivot_table(index=["algorithm", "n"], columns="impl", values="ratio").round(3).to_string())
for name, s in (("run1", a), ("run2", b)):
    q = s.unstack("impl")
    print(f"\n{name}: asm_vs_O3 (>1 means asm faster)")
    print((q["c_O3"] / q["asm"]).round(3).to_string())
