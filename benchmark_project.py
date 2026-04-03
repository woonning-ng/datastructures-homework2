#!/usr/bin/env python3
import argparse
import csv
import json
import platform
import re
import subprocess
import sys
import time
from pathlib import Path

DISP_RE = re.compile(r"Total areal displacement:\s*([0-9eE+.\-]+)")
AREA_IN_RE = re.compile(r"Total signed area in input:\s*([0-9eE+.\-]+)")
AREA_OUT_RE = re.compile(r"Total signed area in output:\s*([0-9eE+.\-]+)")

DEFAULT_STRESS_CASES = [
    "test_hole_grazes_exterior.csv",
    "test_narrow_corridor.csv",
    "test_two_close_holes.csv",
]

def count_input_vertices(csv_path: Path) -> int:
    count = 0
    with csv_path.open("r", newline="") as f:
        reader = csv.reader(f)
        next(reader, None)
        for row in reader:
            if row and len(row) >= 4:
                count += 1
    return count

def parse_output(stdout: str):
    disp = DISP_RE.search(stdout)
    area_in = AREA_IN_RE.search(stdout)
    area_out = AREA_OUT_RE.search(stdout)

    vertex_lines = 0
    started = False
    for line in stdout.splitlines():
        if line.strip() == "ring_id,vertex_id,x,y":
            started = True
            continue
        if not started:
            continue
        if line.startswith("Total signed area in input:"):
            break
        if line.strip():
            vertex_lines += 1

    return {
        "output_vertices": vertex_lines,
        "areal_displacement": float(disp.group(1)) if disp else None,
        "input_area": float(area_in.group(1)) if area_in else None,
        "output_area": float(area_out.group(1)) if area_out else None,
    }

def run_with_peak_memory(executable: str, input_file: Path, target_vertices: int):
    time_bin = "/usr/bin/time"
    if platform.system() in ("Linux", "Darwin") and Path(time_bin).exists():
        start = time.perf_counter()
        proc = subprocess.run(
            [time_bin, "-v", executable, str(input_file), str(target_vertices)],
            capture_output=True,
            text=True
        )
        elapsed_ms = (time.perf_counter() - start) * 1000.0
        peak_memory_kb = None
        if platform.system() == "Linux":
            m = re.search(r"Maximum resident set size \(kbytes\):\s*(\d+)", proc.stderr)
            if m:
                peak_memory_kb = int(m.group(1))
        result = parse_output(proc.stdout)
        result.update({
            "returncode": proc.returncode,
            "runtime_ms": elapsed_ms,
            "peak_memory_kb": peak_memory_kb,
            "stderr": proc.stderr,
            "stdout": proc.stdout,
        })
        return result

    start = time.perf_counter()
    proc = subprocess.run([executable, str(input_file), str(target_vertices)], capture_output=True, text=True)
    elapsed_ms = (time.perf_counter() - start) * 1000.0
    result = parse_output(proc.stdout)
    result.update({
        "returncode": proc.returncode,
        "runtime_ms": elapsed_ms,
        "peak_memory_kb": None,
        "stderr": proc.stderr,
        "stdout": proc.stdout,
    })
    return result

def average_runs(exe, input_file, target, repeats):
    runs = []
    for _ in range(repeats):
        result = run_with_peak_memory(exe, input_file, target)
        if result["returncode"] != 0:
            raise RuntimeError(f"Run failed for {input_file} target={target}\n{result['stderr']}")
        runs.append(result)
    avg = dict(runs[-1])
    avg["runtime_ms"] = sum(r["runtime_ms"] for r in runs) / len(runs)
    mems = [r["peak_memory_kb"] for r in runs if r["peak_memory_kb"] is not None]
    avg["peak_memory_kb"] = (sum(mems) / len(mems)) if mems else None
    avg["repeats_used"] = len(runs)
    return avg

def fixed_target(input_vertices: int, ratio: float):
    target = int(round(input_vertices * ratio))
    return max(3, min(input_vertices - 1, target))

def build_line_series(rows, x_key, y_key, group_key="dataset"):
    groups = {}
    for row in rows:
        groups.setdefault(row[group_key], []).append(row)
    out = []
    for name, items in groups.items():
        items = [i for i in items if i.get(y_key) is not None]
        items.sort(key=lambda r: r[x_key])
        out.append({"name": name, "x": [i[x_key] for i in items], "y": [i[y_key] for i in items]})
    return out

def main():
    parser = argparse.ArgumentParser(description="Benchmark project with separate scaling and stress experiments.")
    parser.add_argument("--exe", default="./simplify")
    parser.add_argument("--scaling-inputs", nargs="+", required=True, help="Comparable datasets for runtime/memory scaling")
    parser.add_argument("--stress-inputs", nargs="*", default=DEFAULT_STRESS_CASES, help="Non-comparable edge cases for separate reporting")
    parser.add_argument("--scaling-ratio", type=float, default=0.8, help="Fixed target ratio for scaling experiments")
    parser.add_argument("--stress-ratio", type=float, default=0.8, help="Fixed target ratio for stress-case comparison")
    parser.add_argument("--displacement-input", help="Dataset used for areal displacement vs target plot; defaults to largest scaling input")
    parser.add_argument("--displacement-ratios", nargs="+", type=float, default=[0.9, 0.8, 0.7, 0.6, 0.5])
    parser.add_argument("--repeats", type=int, default=5)
    parser.add_argument("--out-csv", default="benchmark_project_results.csv")
    parser.add_argument("--out-json", default="benchmark_project_results.json")
    args = parser.parse_args()

    exe = args.exe
    all_rows = []
    scaling_rows = []
    stress_rows = []
    displacement_rows = []

    scaling_inputs = [Path(p) for p in args.scaling_inputs]
    scaling_inputs_sorted = sorted(scaling_inputs, key=count_input_vertices)

    for input_path in scaling_inputs_sorted:
        input_vertices = count_input_vertices(input_path)
        target = fixed_target(input_vertices, args.scaling_ratio)
        result = average_runs(exe, input_path, target, args.repeats)
        row = {
            "experiment": "scaling",
            "dataset": input_path.stem,
            "input_file": str(input_path),
            "input_vertices": input_vertices,
            "target_vertices": target,
            "target_ratio": round(target / input_vertices, 6),
            "output_vertices": result["output_vertices"],
            "runtime_ms": round(result["runtime_ms"], 6),
            "peak_memory_kb": round(result["peak_memory_kb"], 3) if result["peak_memory_kb"] is not None else None,
            "areal_displacement": result["areal_displacement"],
            "input_area": result["input_area"],
            "output_area": result["output_area"],
            "repeats_used": result["repeats_used"],
        }
        scaling_rows.append(row)
        all_rows.append(row)
        print(f"[SCALING] {row['dataset']} vertices={input_vertices} runtime={row['runtime_ms']:.3f} ms")

    for input_name in args.stress_inputs:
        input_path = Path(input_name)
        if not input_path.exists():
            print(f"[WARN] skipping missing stress case {input_path}", file=sys.stderr)
            continue
        input_vertices = count_input_vertices(input_path)
        target = fixed_target(input_vertices, args.stress_ratio)
        result = average_runs(exe, input_path, target, args.repeats)
        row = {
            "experiment": "stress",
            "dataset": input_path.stem,
            "input_file": str(input_path),
            "input_vertices": input_vertices,
            "target_vertices": target,
            "target_ratio": round(target / input_vertices, 6),
            "output_vertices": result["output_vertices"],
            "runtime_ms": round(result["runtime_ms"], 6),
            "peak_memory_kb": round(result["peak_memory_kb"], 3) if result["peak_memory_kb"] is not None else None,
            "areal_displacement": result["areal_displacement"],
            "input_area": result["input_area"],
            "output_area": result["output_area"],
            "repeats_used": result["repeats_used"],
        }
        stress_rows.append(row)
        all_rows.append(row)
        print(f"[STRESS] {row['dataset']} runtime={row['runtime_ms']:.3f} ms")

    displacement_input = Path(args.displacement_input) if args.displacement_input else scaling_inputs_sorted[-1]
    input_vertices = count_input_vertices(displacement_input)
    for ratio in args.displacement_ratios:
        target = fixed_target(input_vertices, ratio)
        result = average_runs(exe, displacement_input, target, args.repeats)
        row = {
            "experiment": "displacement",
            "dataset": displacement_input.stem,
            "input_file": str(displacement_input),
            "input_vertices": input_vertices,
            "target_vertices": target,
            "target_ratio": round(target / input_vertices, 6),
            "output_vertices": result["output_vertices"],
            "runtime_ms": round(result["runtime_ms"], 6),
            "peak_memory_kb": round(result["peak_memory_kb"], 3) if result["peak_memory_kb"] is not None else None,
            "areal_displacement": result["areal_displacement"],
            "input_area": result["input_area"],
            "output_area": result["output_area"],
            "repeats_used": result["repeats_used"],
        }
        displacement_rows.append(row)
        all_rows.append(row)
        print(f"[DISPLACEMENT] target={target} displacement={row['areal_displacement']}")

    all_rows.sort(key=lambda r: (r["experiment"], r["dataset"], r["target_vertices"]))

    out_csv = Path(args.out_csv)
    out_json = Path(args.out_json)
    if all_rows:
        with out_csv.open("w", newline="") as f:
            writer = csv.DictWriter(f, fieldnames=list(all_rows[0].keys()))
            writer.writeheader()
            writer.writerows(all_rows)

    payload = {
        "meta": {
            "executable": exe,
            "scaling_ratio": args.scaling_ratio,
            "stress_ratio": args.stress_ratio,
            "displacement_input": str(displacement_input),
            "repeats": args.repeats,
        },
        "rows": all_rows,
        "charts": {
            "runtime_vs_input": {
                "title": "Running time vs input size (scaling cases only)",
                "x_label": "Input vertices",
                "y_label": "Runtime (ms)",
                "series": [{"name": "Runtime", "x": [r["input_vertices"] for r in scaling_rows], "y": [r["runtime_ms"] for r in scaling_rows]}],
            },
            "memory_vs_input": {
                "title": "Peak memory vs input size (scaling cases only)",
                "x_label": "Input vertices",
                "y_label": "Peak memory (KB)",
                "series": [{"name": "Peak memory", "x": [r["input_vertices"] for r in scaling_rows if r["peak_memory_kb"] is not None], "y": [r["peak_memory_kb"] for r in scaling_rows if r["peak_memory_kb"] is not None]}],
            },
            "areal_displacement_vs_target": {
                "title": f"Areal displacement vs target vertex count ({displacement_input.stem})",
                "x_label": "Target vertices",
                "y_label": "Areal displacement",
                "series": build_line_series(displacement_rows, "target_vertices", "areal_displacement"),
            },
            "stress_runtime": {
                "title": "Stress-case runtime comparison",
                "x_label": "Stress dataset",
                "y_label": "Runtime (ms)",
                "series": [{"name": "Stress runtime", "x": [r["dataset"] for r in stress_rows], "y": [r["runtime_ms"] for r in stress_rows], "type": "bar"}],
            },
        },
    }
    out_json.write_text(json.dumps(payload, indent=2))
    print(f"Wrote {out_csv}")
    print(f"Wrote {out_json}")

if __name__ == "__main__":
    main()
