import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "python"))
from generate_sensor_data import generate  # noqa: E402

CLI = ROOT / "build" / "sp_cli"
THRESH = 2.5
WINDOW = 32


def run(impl, cmd, path, *args, check=True):
    p = subprocess.run([str(CLI), impl, cmd, str(path), *map(str, args)],
                       capture_output=True, text=True, check=check)
    if not check:
        return p
    return dict(line.split("=", 1) for line in p.stdout.splitlines() if "=" in line)


class TestAgainstNumpy(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.TemporaryDirectory()
        cls.data = {}
        for sc in ("normal", "wear", "spikes", "mixed"):
            x, _ = generate(samples=20_000, scenario=sc, seed=7)
            p = Path(cls.tmp.name) / f"{sc}.bin"
            x.tofile(p)
            cls.data[sc] = (x, p)

    @classmethod
    def tearDownClass(cls):
        cls.tmp.cleanup()

    def each(self):
        for sc, (x, p) in self.data.items():
            for impl in ("c", "asm"):
                with self.subTest(scenario=sc, impl=impl):
                    yield impl, x, p

    def test_rms(self):
        for impl, x, p in self.each():
            got = float(run(impl, "rms", p)["rms"])
            np.testing.assert_allclose(got, np.sqrt(np.mean(x ** 2)), rtol=1e-9)

    def test_stats(self):
        for impl, x, p in self.each():
            r = run(impl, "stats", p)
            self.assertEqual(float(r["min"]), x.min())
            self.assertEqual(float(r["max"]), x.max())
            np.testing.assert_allclose(float(r["mean"]), x.mean(), rtol=1e-9, atol=1e-12)
            np.testing.assert_allclose(float(r["stddev"]), x.std(), rtol=1e-9)

    def test_peaks(self):
        for impl, x, p in self.each():
            r = run(impl, "peaks", p, THRESH)
            ref = np.flatnonzero(np.abs(x) > THRESH)
            self.assertEqual(int(r["count"]), ref.size)
            got = np.array(r["indices"].split(), dtype=np.int64)
            np.testing.assert_array_equal(got, ref)

    def test_moving_average(self):
        for impl, x, p in self.each():
            out = Path(self.tmp.name) / f"ma_{impl}.bin"
            run(impl, "movavg", p, WINDOW, out)
            got = np.fromfile(out, dtype="<f8")
            ref = np.convolve(x, np.ones(WINDOW) / WINDOW, mode="valid")
            self.assertEqual(got.size, ref.size)
            np.testing.assert_allclose(got, ref, rtol=1e-9, atol=1e-9)

    def test_c_and_asm_agree(self):
        for sc, (x, p) in self.data.items():
            with self.subTest(scenario=sc):
                a, b = run("c", "stats", p), run("asm", "stats", p)
                for k in a:
                    np.testing.assert_allclose(float(a[k]), float(b[k]), rtol=1e-12)

    def test_empty_file_is_rejected(self):
        p = Path(self.tmp.name) / "empty.bin"
        p.write_bytes(b"")
        for impl in ("c", "asm"):
            r = run(impl, "rms", p, check=False)
            self.assertEqual(r.returncode, 2)


if __name__ == "__main__":
    unittest.main()