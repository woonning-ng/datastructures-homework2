#!/usr/bin/env python3

from __future__ import annotations

import csv
import math
import statistics
import subprocess
import sys
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable


REPO_ROOT = Path(__file__).resolve().parent.parent
TEST_CASES_DIR = REPO_ROOT / "test_cases"
OUTPUT_DATA_DIR = REPO_ROOT / "analysis" / "graph_data"
OUTPUT_GRAPH_DIR = REPO_ROOT / "analysis" / "graphs"
SIMPLIFY_PATH = REPO_ROOT / "simplify"
TIME_BINARY = "/usr/bin/time"
RUN_TIMEOUT_SECONDS = 120
RUNTIME_REPEATS = 3


RUNTIME_MEMORY_CASES = [
    "input_hole_grazes_exterior.csv",
    "input_narrow_corridor.csv",
    "input_rectangle_with_two_holes.csv",
    "input_two_close_holes.csv",
    "input_comb_exterior.csv",
    "input_cushion_with_hexagonal_hole.csv",
    "input_blob_with_two_holes.csv",
    "input_wavy_with_three_holes.csv",
    "input_lake_with_two_islands.csv",
    "input_original_01.csv",
    "input_original_02.csv",
    "input_original_04.csv",
    "input_original_05.csv",
    "input_original_07.csv",
    "input_original_03.csv",
    "input_original_06.csv",
    "input_original_08.csv",
    "input_original_10.csv",
    "input_original_09.csv",
]


DISPLACEMENT_SWEEPS = {
    "input_comb_exterior.csv": [21, 18, 14, 10, 6, 4],
    "input_blob_with_two_holes.csv": [35, 32, 28, 24, 20, 18],
    "input_wavy_with_three_holes.csv": [42, 38, 34, 30, 26, 22],
    "input_lake_with_two_islands.csv": [80, 65, 50, 35, 24, 18],
}


COLORS = ["#0f766e", "#dc2626", "#2563eb", "#d97706", "#7c3aed", "#059669"]


@dataclass
class InputSummary:
    input_file: str
    input_vertices: int
    ring_count: int


def read_manifest_inputs() -> list[str]:
    manifest = TEST_CASES_DIR / "test_manifest.tsv"
    rows: list[str] = []
    with manifest.open(newline="", encoding="utf-8") as handle:
        next(handle)
        for line in handle:
            if not line.strip():
                continue
            input_file, _, _ = line.rstrip("\n").split("\t")
            rows.append(input_file)
    return rows


def summarize_input(input_file: str) -> InputSummary:
    counts: dict[int, int] = {}
    with (TEST_CASES_DIR / input_file).open(newline="", encoding="utf-8") as handle:
        reader = csv.DictReader(handle)
        for row in reader:
            ring_id = int(row["ring_id"])
            counts[ring_id] = counts.get(ring_id, 0) + 1
    return InputSummary(
        input_file=input_file,
        input_vertices=sum(counts.values()),
        ring_count=len(counts),
    )


def run_simplify(input_file: str, target_vertices: int) -> tuple[float, int, float]:
    command = [
        TIME_BINARY,
        "-f",
        "MAXRSS_KB=%M",
        str(SIMPLIFY_PATH),
        str(TEST_CASES_DIR / input_file),
        str(target_vertices),
    ]

    start = time.perf_counter()
    completed = subprocess.run(
        command,
        cwd=REPO_ROOT,
        capture_output=True,
        text=True,
        timeout=RUN_TIMEOUT_SECONDS,
        check=False,
    )
    elapsed_seconds = time.perf_counter() - start

    if completed.returncode != 0:
        raise RuntimeError(
            f"simplify failed for {input_file} target {target_vertices} with code "
            f"{completed.returncode}: {completed.stderr.strip()}"
        )

    maxrss_kb = None
    for line in completed.stderr.splitlines():
        if line.startswith("MAXRSS_KB="):
            maxrss_kb = int(line.split("=", 1)[1])
            break

    if maxrss_kb is None:
        raise RuntimeError(f"Could not parse max RSS for {input_file}")

    displacement = None
    for line in completed.stdout.splitlines():
        if line.startswith("Total areal displacement:"):
            displacement = float(line.split(":", 1)[1].strip())
            break

    if displacement is None:
        raise RuntimeError(f"Could not parse displacement for {input_file}")

    return elapsed_seconds, maxrss_kb, displacement


def median(values: Iterable[float]) -> float:
    return statistics.median(list(values))


def power_fit(xs: list[float], ys: list[float]) -> tuple[float, float]:
    log_x = [math.log10(x) for x in xs]
    log_y = [math.log10(y) for y in ys]
    n = len(xs)
    sum_x = sum(log_x)
    sum_y = sum(log_y)
    sum_xx = sum(x * x for x in log_x)
    sum_xy = sum(x * y for x, y in zip(log_x, log_y))
    denominator = n * sum_xx - sum_x * sum_x
    if abs(denominator) < 1e-12:
        raise RuntimeError("Degenerate fit input")
    slope = (n * sum_xy - sum_x * sum_y) / denominator
    intercept = (sum_y - slope * sum_x) / n
    return 10 ** intercept, slope


def nice_linear_ticks(min_value: float, max_value: float, tick_count: int = 5) -> list[float]:
    if math.isclose(min_value, max_value):
        return [min_value]
    span = max_value - min_value
    raw_step = span / max(1, tick_count)
    magnitude = 10 ** math.floor(math.log10(abs(raw_step)))
    for factor in (1, 2, 5, 10):
        step = factor * magnitude
        if span / step <= tick_count + 1:
            break
    start = math.floor(min_value / step) * step
    ticks: list[float] = []
    value = start
    while value <= max_value + 1e-12:
        if value >= min_value - 1e-12:
            ticks.append(value)
        value += step
    return ticks


def format_tick(value: float) -> str:
    if value >= 1000000:
        return f"{value / 1000000:.1f}M"
    if value >= 1000:
        return f"{value / 1000:.0f}k"
    if value >= 1:
        if value >= 100:
            return f"{value:.0f}"
        if value >= 10:
            return f"{value:.1f}".rstrip("0").rstrip(".")
        return f"{value:.2f}".rstrip("0").rstrip(".")
    return f"{value:.3f}".rstrip("0").rstrip(".")


def svg_escape(text: str) -> str:
    return (
        text.replace("&", "&amp;")
        .replace("<", "&lt;")
        .replace(">", "&gt;")
        .replace('"', "&quot;")
    )


def render_chart(
    output_path: Path,
    title: str,
    subtitle: str,
    x_label: str,
    y_label: str,
    series: list[dict],
    x_scale: str = "linear",
    y_scale: str = "linear",
    fit: dict | None = None,
) -> None:
    width = 1120
    height = 720
    margin_left = 110
    margin_right = 40
    margin_top = 90
    margin_bottom = 90
    plot_width = width - margin_left - margin_right
    plot_height = height - margin_top - margin_bottom

    all_points = [point for item in series for point in item["points"]]
    xs = [point[0] for point in all_points]
    ys = [point[1] for point in all_points]

    if x_scale == "log":
        min_x = 10 ** math.floor(math.log10(min(xs)))
        max_x = 10 ** math.ceil(math.log10(max(xs)))
    else:
        min_x = min(xs)
        max_x = max(xs)
        span = max_x - min_x
        min_x -= span * 0.05 if span else 1
        max_x += span * 0.05 if span else 1

    if y_scale == "log":
        min_y = 10 ** math.floor(math.log10(min(ys)))
        max_y = 10 ** math.ceil(math.log10(max(ys)))
    else:
        min_y = 0 if min(ys) >= 0 else min(ys)
        max_y = max(ys)
        span = max_y - min_y
        max_y += span * 0.1 if span else 1

    def map_x(value: float) -> float:
        if x_scale == "log":
            position = (math.log10(value) - math.log10(min_x)) / (math.log10(max_x) - math.log10(min_x))
        else:
            position = (value - min_x) / (max_x - min_x)
        return margin_left + position * plot_width

    def map_y(value: float) -> float:
        if y_scale == "log":
            position = (math.log10(value) - math.log10(min_y)) / (math.log10(max_y) - math.log10(min_y))
        else:
            position = (value - min_y) / (max_y - min_y)
        return margin_top + (1 - position) * plot_height

    if x_scale == "log":
        x_ticks = [10 ** power for power in range(int(math.log10(min_x)), int(math.log10(max_x)) + 1)]
    else:
        x_ticks = nice_linear_ticks(min_x, max_x, 6)
    if y_scale == "log":
        y_ticks = [10 ** power for power in range(int(math.log10(min_y)), int(math.log10(max_y)) + 1)]
    else:
        y_ticks = nice_linear_ticks(min_y, max_y, 6)

    parts = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}">',
        '<rect width="100%" height="100%" fill="#f8fafc"/>',
        f'<text x="{margin_left}" y="42" font-size="28" font-family="Arial, sans-serif" font-weight="700" fill="#0f172a">{svg_escape(title)}</text>',
        f'<text x="{margin_left}" y="68" font-size="14" font-family="Arial, sans-serif" fill="#475569">{svg_escape(subtitle)}</text>',
        f'<rect x="{margin_left}" y="{margin_top}" width="{plot_width}" height="{plot_height}" fill="#ffffff" stroke="#cbd5e1"/>',
    ]

    for tick in x_ticks:
        x = map_x(tick)
        parts.append(f'<line x1="{x:.2f}" y1="{margin_top}" x2="{x:.2f}" y2="{margin_top + plot_height}" stroke="#e2e8f0"/>')
        label = f"10^{int(round(math.log10(tick)))}" if x_scale == "log" else format_tick(tick)
        parts.append(
            f'<text x="{x:.2f}" y="{margin_top + plot_height + 26}" text-anchor="middle" '
            f'font-size="13" font-family="Arial, sans-serif" fill="#334155">{svg_escape(label)}</text>'
        )

    for tick in y_ticks:
        y = map_y(tick)
        parts.append(f'<line x1="{margin_left}" y1="{y:.2f}" x2="{margin_left + plot_width}" y2="{y:.2f}" stroke="#e2e8f0"/>')
        label = f"10^{int(round(math.log10(tick)))}" if y_scale == "log" else format_tick(tick)
        parts.append(
            f'<text x="{margin_left - 14}" y="{y + 4:.2f}" text-anchor="end" '
            f'font-size="13" font-family="Arial, sans-serif" fill="#334155">{svg_escape(label)}</text>'
        )

    parts.append(
        f'<text x="{margin_left + plot_width / 2:.2f}" y="{height - 24}" text-anchor="middle" '
        f'font-size="16" font-family="Arial, sans-serif" fill="#0f172a">{svg_escape(x_label)}</text>'
    )
    parts.append(
        f'<text x="26" y="{margin_top + plot_height / 2:.2f}" transform="rotate(-90 26,{margin_top + plot_height / 2:.2f})" '
        f'text-anchor="middle" font-size="16" font-family="Arial, sans-serif" fill="#0f172a">{svg_escape(y_label)}</text>'
    )

    if fit is not None:
        fit_points = fit["points"]
        path_segments = []
        for index, (x_value, y_value) in enumerate(fit_points):
            command = "M" if index == 0 else "L"
            path_segments.append(f"{command} {map_x(x_value):.2f} {map_y(y_value):.2f}")
        parts.append(
            f'<path d="{" ".join(path_segments)}" fill="none" stroke="{fit["color"]}" '
            f'stroke-width="2.5" stroke-dasharray="8 6"/>'
        )
        parts.append(
            f'<text x="{margin_left + plot_width - 8}" y="{margin_top + 20}" text-anchor="end" '
            f'font-size="13" font-family="Arial, sans-serif" fill="{fit["color"]}">{svg_escape(fit["label"])}</text>'
        )

    legend_x = margin_left + plot_width - 230
    legend_y = margin_top + 44
    parts.append(
        f'<rect x="{legend_x}" y="{legend_y - 24}" width="210" height="{28 * len(series) + 18}" '
        f'fill="#ffffff" stroke="#cbd5e1" rx="8"/>'
    )

    for index, item in enumerate(series):
        points = sorted(item["points"])
        if item.get("line", False):
            path_segments = []
            for point_index, (x_value, y_value) in enumerate(points):
                command = "M" if point_index == 0 else "L"
                path_segments.append(f"{command} {map_x(x_value):.2f} {map_y(y_value):.2f}")
            parts.append(
                f'<path d="{" ".join(path_segments)}" fill="none" stroke="{item["color"]}" stroke-width="2.5"/>'
            )
        for x_value, y_value in points:
            parts.append(
                f'<circle cx="{map_x(x_value):.2f}" cy="{map_y(y_value):.2f}" r="4.5" '
                f'fill="{item["color"]}" stroke="#ffffff" stroke-width="1.5"/>'
            )
        legend_row_y = legend_y + index * 28
        parts.append(
            f'<line x1="{legend_x + 14}" y1="{legend_row_y}" x2="{legend_x + 38}" y2="{legend_row_y}" '
            f'stroke="{item["color"]}" stroke-width="2.5"/>'
        )
        parts.append(
            f'<circle cx="{legend_x + 26}" cy="{legend_row_y}" r="4.5" fill="{item["color"]}" stroke="#ffffff" stroke-width="1.5"/>'
        )
        parts.append(
            f'<text x="{legend_x + 48}" y="{legend_row_y + 5}" font-size="13" '
            f'font-family="Arial, sans-serif" fill="#0f172a">{svg_escape(item["name"])}</text>'
        )

    parts.append("</svg>")
    output_path.write_text("\n".join(parts), encoding="utf-8")


def write_csv(path: Path, header: list[str], rows: list[list[object]]) -> None:
    with path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.writer(handle)
        writer.writerow(header)
        writer.writerows(rows)


def main() -> int:
    if not SIMPLIFY_PATH.exists():
        print("Could not find built simplify executable.", file=sys.stderr)
        return 1

    OUTPUT_DATA_DIR.mkdir(parents=True, exist_ok=True)
    OUTPUT_GRAPH_DIR.mkdir(parents=True, exist_ok=True)

    manifest_inputs = {summary.input_file: summary for summary in map(summarize_input, read_manifest_inputs())}

    runtime_rows: list[list[object]] = []
    runtime_points: list[tuple[float, float]] = []
    memory_points: list[tuple[float, float]] = []

    print("Collecting runtime and memory measurements...")
    for input_file in RUNTIME_MEMORY_CASES:
        summary = manifest_inputs[input_file]
        target_vertices = summary.input_vertices - 1
        runtimes: list[float] = []
        memories: list[int] = []
        for repeat in range(RUNTIME_REPEATS):
            elapsed_seconds, maxrss_kb, _ = run_simplify(input_file, target_vertices)
            runtimes.append(elapsed_seconds)
            memories.append(maxrss_kb)
            print(
                f"  {input_file} repeat {repeat + 1}/{RUNTIME_REPEATS}: "
                f"{elapsed_seconds:.6f}s, {maxrss_kb} KB"
            )
        median_runtime = median(runtimes)
        median_memory = int(median(memories))
        runtime_rows.append(
            [
                input_file,
                summary.input_vertices,
                summary.ring_count,
                target_vertices,
                f"{median_runtime:.6f}",
                median_memory,
            ]
        )
        runtime_points.append((summary.input_vertices, median_runtime))
        memory_points.append((summary.input_vertices, median_memory))

    runtime_rows.sort(key=lambda row: int(row[1]))
    runtime_points.sort()
    memory_points.sort()

    print("Collecting areal-displacement measurements...")
    displacement_rows: list[list[object]] = []
    displacement_series: list[dict] = []
    for color, (input_file, targets) in zip(COLORS, DISPLACEMENT_SWEEPS.items()):
        summary = manifest_inputs[input_file]
        series_points: list[tuple[float, float]] = []
        for target_vertices in targets:
            _, _, displacement = run_simplify(input_file, target_vertices)
            displacement_rows.append(
                [
                    input_file,
                    summary.input_vertices,
                    summary.ring_count,
                    target_vertices,
                    f"{displacement:.6f}",
                ]
            )
            series_points.append((target_vertices, displacement))
            print(
                f"  {input_file} target {target_vertices}: displacement {displacement:.6f}"
            )
        displacement_series.append(
            {
                "name": input_file.replace("input_", "").replace(".csv", ""),
                "color": color,
                "points": series_points,
                "line": True,
            }
        )

    displacement_rows.sort(key=lambda row: (str(row[0]), int(row[3])))

    write_csv(
        OUTPUT_DATA_DIR / "runtime_memory_vs_input_size.csv",
        [
            "input_file",
            "input_vertices",
            "ring_count",
            "target_vertices",
            "median_runtime_seconds",
            "median_peak_memory_kb",
        ],
        runtime_rows,
    )

    write_csv(
        OUTPUT_DATA_DIR / "areal_displacement_vs_target.csv",
        [
            "input_file",
            "input_vertices",
            "ring_count",
            "target_vertices",
            "total_areal_displacement",
        ],
        displacement_rows,
    )

    runtime_c, runtime_k = power_fit(
        [point[0] for point in runtime_points],
        [point[1] for point in runtime_points],
    )
    memory_c, memory_k = power_fit(
        [point[0] for point in memory_points],
        [point[1] for point in memory_points],
    )

    fit_summary = (
        "Scaling-fit summary\n"
        "===================\n"
        "Runtime graph setup: target = input_vertices - 1 for every dataset so the program\n"
        "performs a real collapse while avoiding the hardcoded golden-output shortcut.\n\n"
        f"Runtime fit (power law): time ~= {runtime_c:.6e} * n^{runtime_k:.3f}\n"
        f"Memory fit (power law): memory ~= {memory_c:.6e} * n^{memory_k:.3f}\n"
    )
    (OUTPUT_DATA_DIR / "fit_summary.txt").write_text(fit_summary, encoding="utf-8")

    runtime_fit_points = []
    runtime_min_n = min(point[0] for point in runtime_points)
    runtime_max_n = max(point[0] for point in runtime_points)
    memory_min_n = min(point[0] for point in memory_points)
    memory_max_n = max(point[0] for point in memory_points)
    for exponent in [i / 30 for i in range(31)]:
        n_value = 10 ** (
            math.log10(runtime_min_n) +
            exponent * (math.log10(runtime_max_n) - math.log10(runtime_min_n))
        )
        runtime_fit_points.append((n_value, runtime_c * (n_value ** runtime_k)))

    memory_fit_points = []
    for exponent in [i / 30 for i in range(31)]:
        n_value = 10 ** (
            math.log10(memory_min_n) +
            exponent * (math.log10(memory_max_n) - math.log10(memory_min_n))
        )
        memory_fit_points.append((n_value, memory_c * (n_value ** memory_k)))

    render_chart(
        OUTPUT_GRAPH_DIR / "running_time_vs_input_size.svg",
        "Running Time vs Input Size",
        "Median of three runs, measured with target = input_vertices - 1 (log-log scale).",
        "Input size (vertices)",
        "Running time (seconds)",
        [
            {
                "name": "Measured datasets",
                "color": "#0f766e",
                "points": runtime_points,
                "line": False,
            }
        ],
        x_scale="log",
        y_scale="log",
        fit={
            "points": runtime_fit_points,
            "color": "#b91c1c",
            "label": f"fit: time ~= {runtime_c:.2e} * n^{runtime_k:.2f}",
        },
    )

    render_chart(
        OUTPUT_GRAPH_DIR / "peak_memory_vs_input_size.svg",
        "Peak Memory Usage vs Input Size",
        "Median of three runs, measured with target = input_vertices - 1 (log-log scale).",
        "Input size (vertices)",
        "Peak memory usage (KB)",
        [
            {
                "name": "Measured datasets",
                "color": "#2563eb",
                "points": memory_points,
                "line": False,
            }
        ],
        x_scale="log",
        y_scale="log",
        fit={
            "points": memory_fit_points,
            "color": "#d97706",
            "label": f"fit: memory ~= {memory_c:.2e} * n^{memory_k:.2f}",
        },
    )

    render_chart(
        OUTPUT_GRAPH_DIR / "areal_displacement_vs_target_vertex_count.svg",
        "Areal Displacement vs Target Vertex Count",
        "Representative simplification sweeps on four datasets (linear scale).",
        "Target vertex count",
        "Total areal displacement",
        displacement_series,
        x_scale="linear",
        y_scale="linear",
        fit=None,
    )

    print("\nDone.")
    print(f"CSV data: {OUTPUT_DATA_DIR}")
    print(f"SVG graphs: {OUTPUT_GRAPH_DIR}")
    print(f"Fit summary: {OUTPUT_DATA_DIR / 'fit_summary.txt'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
