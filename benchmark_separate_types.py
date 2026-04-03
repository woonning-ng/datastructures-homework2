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

DEFAULT_CASE_GROUPS = {
    "comb": [
        "our_test_cases/test_comb_teeth_50.csv",
        "our_test_cases/test_comb_teeth_100.csv",
        "our_test_cases/test_comb_teeth_200.csv",
        "our_test_cases/test_comb_teeth_400.csv",
        "our_test_cases/test_comb_teeth_800.csv",
        "our_test_cases/test_comb_teeth_1600.csv",
    ],
    "hole_graze": [],
    "corridor": [],
    "two_close_holes": [],
}

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

def parse_group_args(group_args):
    groups = {}
    for entry in group_args:
        if "=" not in entry:
            raise ValueError(f"Invalid group spec '{entry}'. Use name=file1,file2,...")
        name, values = entry.split("=", 1)
        files = [v.strip() for v in values.split(",") if v.strip()]
        groups[name.strip()] = files
    return groups

def make_xy_series(rows, y_key):
    items = [r for r in rows if r.get(y_key) is not None]
    items.sort(key=lambda r: r["input_vertices"])
    return [{
        "name": y_key,
        "x": [r["input_vertices"] for r in items],
        "y": [r[y_key] for r in items],
        "text": [r["dataset"] for r in items],
    }]

def make_displacement_series(rows):
    items = [r for r in rows if r.get("areal_displacement") is not None]
    items.sort(key=lambda r: r["target_vertices"])
    return [{
        "name": "areal_displacement",
        "x": [r["target_vertices"] for r in items],
        "y": [r["areal_displacement"] for r in items],
    }]

def main():
    parser = argparse.ArgumentParser(description="Benchmark separate graph families for each test-case type.")
    parser.add_argument("--exe", default="./simplify")
    parser.add_argument("--group", action="append", default=[],
                        help="Case-group spec: name=file1,file2,file3")
    parser.add_argument("--scaling-ratio", type=float, default=0.5)
    parser.add_argument("--displacement-group", default="comb",
                        help="Group name used for displacement-vs-target plots")
    parser.add_argument("--displacement-file", default="",
                        help="Specific file for displacement plot; defaults to largest file in displacement-group")
    parser.add_argument("--displacement-ratios", nargs="+", type=float, default=[0.9, 0.8, 0.7, 0.6, 0.5])
    parser.add_argument("--repeats", type=int, default=5)
    parser.add_argument("--out-csv", default="benchmark_separate_types_results.csv")
    parser.add_argument("--out-json", default="benchmark_separate_types_results.json")
    args = parser.parse_args()

    groups = DEFAULT_CASE_GROUPS.copy()
    if args.group:
        groups = parse_group_args(args.group)

    all_rows = []
    group_rows = {}
    for group_name, file_list in groups.items():
        if not file_list:
            continue
        rows = []
        paths = [Path(p) for p in file_list]
        existing = [p for p in paths if p.exists()]
        if not existing:
            print(f"[WARN] skipping group '{group_name}' because no files were found", file=sys.stderr)
            continue

        existing.sort(key=count_input_vertices)
        for input_path in existing:
            input_vertices = count_input_vertices(input_path)
            target = fixed_target(input_vertices, args.scaling_ratio)
            result = average_runs(args.exe, input_path, target, args.repeats)
            row = {
                "experiment": "scaling",
                "group": group_name,
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
            rows.append(row)
            all_rows.append(row)
            print(f"[{group_name}] {row['dataset']} vertices={input_vertices} runtime={row['runtime_ms']:.3f} ms")
        group_rows[group_name] = rows

    disp_group = args.displacement_group
    displacement_rows = []
    disp_file = Path(args.displacement_file) if args.displacement_file else None
    if disp_file is None:
        candidates = group_rows.get(disp_group, [])
        if candidates:
            disp_row = max(candidates, key=lambda r: r["input_vertices"])
            disp_file = Path(disp_row["input_file"])

    if disp_file and disp_file.exists():
        input_vertices = count_input_vertices(disp_file)
        for ratio in args.displacement_ratios:
            target = fixed_target(input_vertices, ratio)
            result = average_runs(args.exe, disp_file, target, args.repeats)
            row = {
                "experiment": "displacement",
                "group": disp_group,
                "dataset": disp_file.stem,
                "input_file": str(disp_file),
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
            print(f"[displacement] {disp_file.stem} target={target} displacement={row['areal_displacement']}")

    all_rows.sort(key=lambda r: (r["experiment"], r["group"], r["dataset"], r["target_vertices"]))

    charts = {}
    for group_name, rows in group_rows.items():
        charts[f"{group_name}_runtime"] = {
            "title": f"Running time vs input size ({group_name})",
            "x_label": "Input vertices",
            "y_label": "Runtime (ms)",
            "series": make_xy_series(rows, "runtime_ms"),
        }
        charts[f"{group_name}_memory"] = {
            "title": f"Peak memory vs input size ({group_name})",
            "x_label": "Input vertices",
            "y_label": "Peak memory (KB)",
            "series": make_xy_series(rows, "peak_memory_kb"),
        }

    if displacement_rows:
        charts[f"{disp_group}_displacement"] = {
            "title": f"Areal displacement vs target vertex count ({disp_file.stem})",
            "x_label": "Target vertices",
            "y_label": "Areal displacement",
            "series": make_displacement_series(displacement_rows),
        }

    payload = {
        "meta": {
            "executable": args.exe,
            "scaling_ratio": args.scaling_ratio,
            "repeats": args.repeats,
            "groups": groups,
            "displacement_group": disp_group,
            "displacement_file": str(disp_file) if disp_file else "",
        },
        "rows": all_rows,
        "charts": charts,
    }

    out_csv = Path(args.out_csv)
    out_json = Path(args.out_json)
    if all_rows:
        with out_csv.open("w", newline="") as f:
            writer = csv.DictWriter(f, fieldnames=list(all_rows[0].keys()))
            writer.writeheader()
            writer.writerows(all_rows)
    out_json.write_text(json.dumps(payload, indent=2))
    print(f"Wrote {out_csv}")
    print(f"Wrote {out_json}")

if __name__ == "__main__":
    main()
