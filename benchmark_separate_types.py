#!/usr/bin/env python3
import argparse
import csv
import json
import math
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
    "hole_graze": [
        "our_test_cases/test_hole_graze_20.csv",
        "our_test_cases/test_hole_graze_40.csv",
        "our_test_cases/test_hole_graze_80.csv",
        "our_test_cases/test_hole_graze_160.csv",
        "our_test_cases/test_hole_graze_320.csv",
    ],
    "corridor": [
        "our_test_cases/test_corridor_20.csv",
        "our_test_cases/test_corridor_40.csv",
        "our_test_cases/test_corridor_80.csv",
        "our_test_cases/test_corridor_160.csv",
        "our_test_cases/test_corridor_320.csv",
    ],
    "two_close_holes": [
        "our_test_cases/test_twoholes_20.csv",
        "our_test_cases/test_twoholes_40.csv",
        "our_test_cases/test_twoholes_80.csv",
        "our_test_cases/test_twoholes_160.csv",
        "our_test_cases/test_twoholes_320.csv",
    ],
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
    proc = subprocess.run(
        [executable, str(input_file), str(target_vertices)],
        capture_output=True,
        text=True
    )
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
            raise RuntimeError(
                f"Run failed for {input_file} target={target}\n"
                f"STDERR:\n{result['stderr']}\n"
                f"STDOUT:\n{result['stdout']}"
            )
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
        "text": [f"{r['group']}:{r['dataset']}" for r in items],
    }]

def fit_linear_through_origin(xs, ys):
    denom = sum(x * x for x in xs)
    if denom == 0:
        return None
    c = sum(x * y for x, y in zip(xs, ys)) / denom
    preds = [c * x for x in xs]
    return {"model": "c*n", "c": c, "predictions": preds}

def fit_nlogn_through_origin(xs, ys):
    basis = [x * math.log2(max(x, 2)) for x in xs]
    denom = sum(b * b for b in basis)
    if denom == 0:
        return None
    c = sum(b * y for b, y in zip(basis, ys)) / denom
    preds = [c * b for b in basis]
    return {"model": "c*nlogn", "c": c, "predictions": preds}

def fit_power_law(xs, ys):
    pairs = [(x, y) for x, y in zip(xs, ys) if x > 0 and y is not None and y > 0]
    if len(pairs) < 2:
        return None

    lx = [math.log(x) for x, _ in pairs]
    ly = [math.log(y) for _, y in pairs]
    n = len(pairs)

    sx = sum(lx)
    sy = sum(ly)
    sxx = sum(v * v for v in lx)
    sxy = sum(a * b for a, b in zip(lx, ly))
    denom = n * sxx - sx * sx
    if denom == 0:
        return None

    k = (n * sxy - sx * sy) / denom
    a = (sy - k * sx) / n
    c = math.exp(a)

    xs_used = [x for x, _ in pairs]
    ys_used = [y for _, y in pairs]
    preds = [c * (x ** k) for x in xs_used]

    return {
        "model": "c*n^k",
        "c": c,
        "k": k,
        "xs_used": xs_used,
        "ys_used": ys_used,
        "predictions": preds,
    }

def r_squared(actual, predicted):
    if not actual or len(actual) != len(predicted):
        return None
    mean_y = sum(actual) / len(actual)
    ss_tot = sum((y - mean_y) ** 2 for y in actual)
    ss_res = sum((y - yp) ** 2 for y, yp in zip(actual, predicted))
    if ss_tot == 0:
        return 1.0
    return 1.0 - ss_res / ss_tot

def fit_models(rows, y_key):
    items = [r for r in rows if r.get(y_key) is not None]
    items.sort(key=lambda r: r["input_vertices"])

    xs = [r["input_vertices"] for r in items]
    ys = [r[y_key] for r in items]

    if len(xs) < 2:
        return []

    fits = []

    lin = fit_linear_through_origin(xs, ys)
    if lin:
        fits.append({
            "model": lin["model"],
            "c": lin["c"],
            "k": 1.0,
            "r2": r_squared(ys, lin["predictions"]),
        })

    nlogn = fit_nlogn_through_origin(xs, ys)
    if nlogn:
        fits.append({
            "model": nlogn["model"],
            "c": nlogn["c"],
            "k": None,
            "r2": r_squared(ys, nlogn["predictions"]),
        })

    power = fit_power_law(xs, ys)
    if power:
        fits.append({
            "model": power["model"],
            "c": power["c"],
            "k": power["k"],
            "r2": r_squared(power["ys_used"], power["predictions"]),
        })

    fits.sort(key=lambda f: float("-inf") if f["r2"] is None else -f["r2"])
    return fits

def main():
    parser = argparse.ArgumentParser(
        description="Benchmark separate graph families for each test-case type."
    )
    parser.add_argument("--exe", default="./simplify")
    parser.add_argument(
        "--group",
        action="append",
        default=[],
        help="Case-group spec: name=file1,file2,file3"
    )
    parser.add_argument("--scaling-ratio", type=float, default=0.5)
    parser.add_argument(
        "--displacement-ratios",
        nargs="+",
        type=float,
        default=[0.9, 0.8, 0.7, 0.6, 0.5, 0.4, 0.3]
    )
    parser.add_argument("--repeats", type=int, default=5)
    parser.add_argument("--out-csv", default="benchmark_separate_types_results.csv")
    parser.add_argument("--out-json", default="benchmark_separate_types_results.json")
    args = parser.parse_args()

    groups = DEFAULT_CASE_GROUPS.copy()
    if args.group:
        groups = parse_group_args(args.group)

    all_rows = []
    group_rows = {}
    displacement_by_group = {}

    # Scaling experiments: runtime and memory for every dataset in every group
    for group_name, file_list in groups.items():
        if not file_list:
            continue

        paths = [Path(p) for p in file_list]
        existing = [p for p in paths if p.exists()]

        if not existing:
            print(f"[WARN] skipping group '{group_name}' because no files were found", file=sys.stderr)
            continue

        existing.sort(key=count_input_vertices)
        rows = []

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

            print(
                f"[{group_name}] {row['dataset']} "
                f"vertices={input_vertices} "
                f"target={target} "
                f"runtime={row['runtime_ms']:.3f} ms "
                f"memory={row['peak_memory_kb']}"
            )

        group_rows[group_name] = rows

    # Displacement sweep for every group, using the largest dataset in that group
    for group_name, rows in group_rows.items():
        if not rows:
            continue

        base_row = max(rows, key=lambda r: r["input_vertices"])
        disp_file = Path(base_row["input_file"])
        input_vertices = base_row["input_vertices"]

        displacement_rows = []
        for ratio in args.displacement_ratios:
            target = fixed_target(input_vertices, ratio)
            result = average_runs(args.exe, disp_file, target, args.repeats)

            row = {
                "experiment": "displacement",
                "group": group_name,
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

            print(
                f"[displacement:{group_name}] {disp_file.stem} "
                f"target={target} "
                f"ratio={ratio:.2f} "
                f"displacement={row['areal_displacement']}"
            )

        displacement_by_group[group_name] = displacement_rows

    all_rows.sort(key=lambda r: (r["experiment"], r["group"], r["dataset"], r["target_vertices"]))

    charts = {}
    fits = {}

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

        fits[group_name] = {
            "runtime_ms": fit_models(rows, "runtime_ms"),
            "peak_memory_kb": fit_models(rows, "peak_memory_kb"),
        }

    for group_name, rows in displacement_by_group.items():
        charts[f"{group_name}_displacement"] = {
            "title": f"Areal displacement vs target vertex count ({group_name})",
            "x_label": "Target vertices",
            "y_label": "Areal displacement",
            "series": make_displacement_series(rows),
        }

    payload = {
        "meta": {
            "executable": args.exe,
            "scaling_ratio": args.scaling_ratio,
            "displacement_ratios": args.displacement_ratios,
            "repeats": args.repeats,
            "groups": groups,
        },
        "rows": all_rows,
        "charts": charts,
        "fits": fits,
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

    print("\n=== Scaling fits summary ===")
    for group_name, group_fit in fits.items():
        print(f"\n[{group_name}]")
        for metric, fit_list in group_fit.items():
            if not fit_list:
                print(f"  {metric}: no fit available")
                continue

            best = fit_list[0]
            if best["model"] == "c*n^k":
                print(
                    f"  {metric}: best fit = {best['model']} "
                    f"(c={best['c']:.6g}, k={best['k']:.4f}, R^2={best['r2']:.4f})"
                )
            else:
                print(
                    f"  {metric}: best fit = {best['model']} "
                    f"(c={best['c']:.6g}, R^2={best['r2']:.4f})"
                )

if __name__ == "__main__":
    main()