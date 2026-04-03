#!/usr/bin/env python3
import argparse
import csv
import json
import math
import os
import platform
import re
import subprocess
import sys
import time
from pathlib import Path

DISP_RE = re.compile(r"Total areal displacement:\s*([0-9eE+.\-]+)")
AREA_IN_RE = re.compile(r"Total signed area in input:\s*([0-9eE+.\-]+)")
AREA_OUT_RE = re.compile(r"Total signed area in output:\s*([0-9eE+.\-]+)")

def count_input_vertices(csv_path: Path) -> int:
    count = 0
    with csv_path.open("r", newline="") as f:
        reader = csv.reader(f)
        next(reader, None)  # header
        for row in reader:
            if row and len(row) >= 4:
                count += 1
    return count

def parse_output(stdout: str) -> dict:
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

def run_once(executable: str, input_file: Path, target_vertices: int):
    start = time.perf_counter()
    proc = subprocess.run(
        [executable, str(input_file), str(target_vertices)],
        capture_output=True,
        text=True
    )
    elapsed_ms = (time.perf_counter() - start) * 1000.0
    parsed = parse_output(proc.stdout)
    return {
        "returncode": proc.returncode,
        "runtime_ms": elapsed_ms,
        "stdout": proc.stdout,
        "stderr": proc.stderr,
        **parsed,
    }

def run_with_peak_memory(executable: str, input_file: Path, target_vertices: int):
    if platform.system() not in ("Linux", "Darwin"):
        result = run_once(executable, input_file, target_vertices)
        result["peak_memory_kb"] = None
        result["memory_method"] = "unsupported"
        return result

    time_bin = "/usr/bin/time"
    if not Path(time_bin).exists():
        result = run_once(executable, input_file, target_vertices)
        result["peak_memory_kb"] = None
        result["memory_method"] = "time_not_found"
        return result

    start = time.perf_counter()
    proc = subprocess.run(
        [time_bin, "-v", executable, str(input_file), str(target_vertices)],
        capture_output=True,
        text=True
    )
    elapsed_ms = (time.perf_counter() - start) * 1000.0
    parsed = parse_output(proc.stdout)

    peak_memory_kb = None
    if platform.system() == "Linux":
        m = re.search(r"Maximum resident set size \(kbytes\):\s*(\d+)", proc.stderr)
        if m:
            peak_memory_kb = int(m.group(1))
    elif platform.system() == "Darwin":
        m = re.search(r"(\d+)\s+maximum resident set size", proc.stderr)
        if m:
            peak_memory_kb = int(m.group(1))

    return {
        "returncode": proc.returncode,
        "runtime_ms": elapsed_ms,
        "stdout": proc.stdout,
        "stderr": proc.stderr,
        "peak_memory_kb": peak_memory_kb,
        "memory_method": "/usr/bin/time -v" if platform.system() == "Linux" else "/usr/bin/time -l",
        **parsed,
    }

def loose_targets(input_vertices: int):
    ratios = [0.9, 0.8, 0.7, 0.6, 0.5]
    values = []
    for r in ratios:
        t = max(3, int(round(input_vertices * r)))
        if t < input_vertices:
            values.append(t)
    return sorted(set(values), reverse=True)

def build_series(rows, x_key, y_key, group_key="dataset"):
    grouped = {}
    for row in rows:
        grouped.setdefault(row[group_key], []).append(row)
    series = []
    for name, items in grouped.items():
        clean = [r for r in items if r.get(y_key) is not None]
        clean.sort(key=lambda r: r[x_key])
        series.append({
            "name": name,
            "x": [r[x_key] for r in clean],
            "y": [r[y_key] for r in clean],
        })
    return series

def main():
    parser = argparse.ArgumentParser(description="Benchmark the simplify executable and emit CSV + JSON for HTML plotting.")
    parser.add_argument("--exe", default="./simplify", help="Path to simplify executable")
    parser.add_argument("--inputs", nargs="+", required=True, help="List of CSV input files")
    parser.add_argument("--targets", nargs="*", type=int, help="Explicit target vertices to use for every dataset")
    parser.add_argument("--repeats", type=int, default=3, help="Number of runs per case")
    parser.add_argument("--out-csv", default="benchmark_results.csv", help="Output CSV path")
    parser.add_argument("--out-json", default="benchmark_results.json", help="Output JSON path for HTML viewer")
    args = parser.parse_args()

    rows = []
    exe = args.exe

    for input_name in args.inputs:
        input_path = Path(input_name)
        dataset = input_path.stem
        input_vertices = count_input_vertices(input_path)
        targets = args.targets[:] if args.targets else loose_targets(input_vertices)

        for target in targets:
            runtimes = []
            peak_memories = []
            parsed_result = None

            for _ in range(max(1, args.repeats)):
                result = run_with_peak_memory(exe, input_path, target)
                if result["returncode"] != 0:
                    print(f"[WARN] {dataset} target={target} failed with code {result['returncode']}", file=sys.stderr)
                    print(result["stderr"], file=sys.stderr)
                    continue
                runtimes.append(result["runtime_ms"])
                if result.get("peak_memory_kb") is not None:
                    peak_memories.append(result["peak_memory_kb"])
                parsed_result = result

            if not runtimes or parsed_result is None:
                continue

            avg_runtime = sum(runtimes) / len(runtimes)
            avg_peak_memory = (sum(peak_memories) / len(peak_memories)) if peak_memories else None

            row = {
                "dataset": dataset,
                "input_file": str(input_path),
                "input_vertices": input_vertices,
                "target_vertices": target,
                "target_ratio": round(target / input_vertices, 6) if input_vertices else None,
                "output_vertices": parsed_result["output_vertices"],
                "runtime_ms": round(avg_runtime, 6),
                "peak_memory_kb": round(avg_peak_memory, 3) if avg_peak_memory is not None else None,
                "areal_displacement": parsed_result["areal_displacement"],
                "input_area": parsed_result["input_area"],
                "output_area": parsed_result["output_area"],
                "repeats_used": len(runtimes),
            }
            rows.append(row)
            print(f"[OK] {dataset} target={target} runtime={avg_runtime:.3f} ms")

    rows.sort(key=lambda r: (r["dataset"], r["target_vertices"]))

    out_csv = Path(args.out_csv)
    out_json = Path(args.out_json)
    if rows:
        with out_csv.open("w", newline="") as f:
            writer = csv.DictWriter(f, fieldnames=list(rows[0].keys()))
            writer.writeheader()
            writer.writerows(rows)

    runtime_vs_input_rows = []
    memory_vs_input_rows = []
    best_by_dataset = {}
    for row in rows:
        current = best_by_dataset.get(row["dataset"])
        if current is None or abs(row["target_ratio"] - 0.8) < abs(current["target_ratio"] - 0.8):
            best_by_dataset[row["dataset"]] = row
    runtime_vs_input_rows = list(best_by_dataset.values())
    memory_vs_input_rows = list(best_by_dataset.values())

    payload = {
        "meta": {
            "executable": exe,
            "generated_from": [str(Path(p)) for p in args.inputs],
            "repeats": args.repeats,
        },
        "rows": rows,
        "charts": {
            "runtime_vs_input": {
                "title": "Running time vs input size",
                "x_label": "Input vertices",
                "y_label": "Runtime (ms)",
                "series": [{
                    "name": "Runtime",
                    "x": [r["input_vertices"] for r in sorted(runtime_vs_input_rows, key=lambda x: x["input_vertices"])],
                    "y": [r["runtime_ms"] for r in sorted(runtime_vs_input_rows, key=lambda x: x["input_vertices"])],
                    "text": [f'{r["dataset"]} @ target {r["target_vertices"]}' for r in sorted(runtime_vs_input_rows, key=lambda x: x["input_vertices"])],
                }],
            },
            "memory_vs_input": {
                "title": "Peak memory vs input size",
                "x_label": "Input vertices",
                "y_label": "Peak memory (KB)",
                "series": [{
                    "name": "Peak memory",
                    "x": [r["input_vertices"] for r in sorted(memory_vs_input_rows, key=lambda x: x["input_vertices"]) if r["peak_memory_kb"] is not None],
                    "y": [r["peak_memory_kb"] for r in sorted(memory_vs_input_rows, key=lambda x: x["input_vertices"]) if r["peak_memory_kb"] is not None],
                    "text": [f'{r["dataset"]} @ target {r["target_vertices"]}' for r in sorted(memory_vs_input_rows, key=lambda x: x["input_vertices"]) if r["peak_memory_kb"] is not None],
                }],
            },
            "areal_displacement_vs_target": {
                "title": "Areal displacement vs target vertex count",
                "x_label": "Target vertices",
                "y_label": "Areal displacement",
                "series": build_series(rows, "target_vertices", "areal_displacement"),
            },
        },
    }

    with out_json.open("w") as f:
        json.dump(payload, f, indent=2)

    print(f"\nWrote {out_csv}")
    print(f"Wrote {out_json}")

if __name__ == "__main__":
    main()
